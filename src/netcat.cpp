#include "netcat.hpp"
#include "journal.hpp"

#include "btdef/date.hpp"
#include "btdef/util/basic_string.hpp"

#include <mysql.h>
// FIX my_bool was removed in 8.0.1
#ifndef HAVE_TYPE_MY_BOOL
#include <stdbool.h>
typedef bool my_bool;
#endif

#include <cstdlib>

#include <sys/uio.h>

// журнал работы
static const captor::journal j;

namespace captor {

static const ref::string arop(std::cref("["));
static const ref::string aren(std::cref("]"));
static const ref::string pre(std::cref(",\""));
static const ref::string post(std::cref("\""));
static const ref::string meex(std::cref("\",\""));
static const ref::string meexnu(std::cref("\",\"\""));

netcat::netcat(char* ptr) noexcept
    : socket_(static_cast<evutil_socket_t>(
        reinterpret_cast<std::intptr_t>(ptr)))
{
    assert(ptr);
}
    
void netcat::close() noexcept
{
    socket_.close();
}

void netcat::attach(evutil_socket_t fd) noexcept
{
    socket_.attach(fd);
    //socket_.set(btpro::sndbuf::size(1048576));
}

// подключаемся только на локалхост
void netcat::connect(const btpro::ip::addr& dest)
{
    // создаем сокет в блокируемом режиме
    // было очень много раздумий и обсуждений делать сокет
    // блокируемым или нет
    // есть куча доводов и за и против
    // при условии:
    // что мы работаем только через локалхост
    // что принимающий сервер - нереально быстрый
    // dll не скормить данных больше чем она может пропихать в сокет
    // небыло ни одного 100% полезного свойства у неблокируемого сокета
    // было решено сделать сокет блокируемым
    // это проще и выглядит надежнее

    auto fd = ::socket(dest.family(), SOCK_STREAM, 0);
    if (btpro::code::fail == fd)
        throw std::system_error(btpro::net::error_code(), "socket");

    auto res = ::connect(fd, dest.sa(), dest.size());
    if (btpro::code::fail == res)
        throw std::system_error(btpro::net::error_code(), "connect");

    attach(fd);
}

// null exchange
void netcat::send_header(const char* method, unsigned long method_size)
{
    numbuf buf;
    buf.append(arop);
    buf.append(btdef::date::now().time());
    buf.append(pre);
    buf.append(method, method_size);
    buf.append(meexnu);
    send(buf);
}

void netcat::send_header(const char* method, unsigned long method_size,
    const char* exchange, unsigned long exchange_size)
{
    numbuf buf;
    buf.append(arop);
    buf.append(btdef::date::now().time());
    buf.append(pre);
    buf.append(method, method_size);
    buf.append(meex);
    buf.append(exchange, exchange_size);
    buf.append(post);
    send(buf);
}


long long netcat::send(const refbuf& buf) const
{
    // отправлеям
    // для синхронного скоета
    // writev блокирует контекст пока все не отправит
    auto res = ::writev(socket_.fd(),
        buf.data(), static_cast<int>(buf.count()));

    if (btpro::code::fail == res)
        throw std::system_error(btpro::net::error_code(), "send");

    return static_cast<long long>(res);
}

} // namespace captor

extern "C" my_bool netcat_init(UDF_INIT* initid,
    UDF_ARGS* args, char* msg)
{
    captor::netcat netcat;
    bool continue_if_fail = false;
    try
    {
        auto args_count = args->arg_count;
        if ((args_count < 4) ||
            (!(((args->arg_type[0] == INT_RESULT) ||
                (args->arg_type[0] == STRING_RESULT)) &&
               (args->arg_type[1] == STRING_RESULT) &&
               (args->arg_type[2] == STRING_RESULT) &&
               (args->arg_type[3] == STRING_RESULT))))
        {
            strncpy(msg, "bad args type, use "
                    "netcat(address, method, exchange, json-data[, route])",
                    MYSQL_ERRMSG_SIZE);
            return 1;
        }

        // проверяем на количество роутов
        if (args_count > 5)
        {
            strncpy(msg, "to many routes", MYSQL_ERRMSG_SIZE);
            return 1;
        }

        if (args_count == 5)
        {
            auto type = args->arg_type[4];
            if (!((type == INT_RESULT) || (type == STRING_RESULT)))
            {
                strncpy(msg, "route must be int or string", MYSQL_ERRMSG_SIZE);
                return 1;
            }
        }

        auto addr_val = args->args[0];
        auto addr_size = args->lengths[0];
        auto cmd_val = args->args[1];
        auto cmd_size = args->lengths[1];
        if (!(addr_val && addr_size && cmd_val && cmd_size))
        {
            strncpy(msg, "bad args, use "
                    "netcat(address, method, exchange, json-data[, route])",
                    MYSQL_ERRMSG_SIZE);
            return 1;
        }

        // парсим адрес
        btpro::sock_addr dest;
        auto addr_type = args->arg_type[0];
        if (addr_type == INT_RESULT)
        {
            auto p = static_cast<int>(*reinterpret_cast<long long*>(addr_val));
            if (p < 0)
            {
                continue_if_fail = true;
                p = -p;
            }

            dest.assign(btpro::ipv4::loopback(p));
        }
        else
        {
            auto addr = addr_val;
            auto size = addr_size;
            if (addr[0] == '-')
            {
                continue_if_fail = true;
                ++addr;
                --size;
            }
            dest.assign(std::string(addr, size));
        }

        // подключаемся
        netcat.connect(dest);

        auto param_val = args->args[2];
        auto param_size = args->lengths[2];
        // отправляем хидер
        if (!(param_val && param_size))
            netcat.send_header(cmd_val, cmd_size);
        else
            netcat.send_header(cmd_val, cmd_size, param_val, param_size);

        initid->maybe_null = 0;
        initid->const_item = 0;

        // сохраняем сокет
        initid->ptr = netcat.char_fd();

        return 0;
    }
    catch (const std::exception& e)
    {
        if (!continue_if_fail)
            snprintf(msg, MYSQL_ERRMSG_SIZE, "%s", e.what());
    }
    catch (...)
    {
        if (!continue_if_fail)
            strncpy(msg, "netcat_init :*(", MYSQL_ERRMSG_SIZE);
    }

    netcat.close();
    initid->ptr = nullptr;

    return (continue_if_fail) ? 0 : 1;
}

bool make_packet(captor::numbuf& buf, UDF_ARGS* args)
{
    assert(args);

    auto data = args->args[3];
    auto data_size = args->lengths[3];

    if (!(data && data_size))
    {
        j.cout([&]{
            return std::mkstr(std::cref("netcat: no data?"));
        });
        // если нет данных - выходим
        return false;
    }

    static const ref::string arcop(std::cref(",["));
    buf.append(arcop);

    // пишем  данные
    buf.append(data, data_size);

    auto route = args->args[4];
    // отправляем маршрут если он есть
    if (args->arg_count == 5 && route)
    {

        static const ref::string c(std::cref(","));
        // если маршрут целочисленный
        if (args->arg_type[4] == INT_RESULT)
        {
            static const ref::string roen(std::cref("]"));
            auto route_number = *reinterpret_cast<long long*>(route);
            buf.append(c);
            buf.append(route_number);
            buf.append(roen);
        }
        else
        {
            auto route_size = args->lengths[4];
//            auto b = route[0];
//            auto e = route[route_size - 1];
//            // simple json test
//            if (((b == '{') && (e == '}')) || ((b == '[') && (e == ']')))
//            {
//                static const ref::string roen(std::cref("]"));
//                buf.append(c);
//                buf.append(route, route_size);
//                buf.append(roen);
//            }
//            else
//            {
                static const ref::string roen(std::cref("\"]"));
                buf.append(captor::pre);
                buf.append(route, route_size);
                buf.append(roen);
//            }
        }
    }
    else
    {
        static const ref::string ronl(std::cref(",\"\"]"));
        buf.append(ronl);
    }

    return true;
}

extern "C" long long netcat(UDF_INIT* initid,
    UDF_ARGS* args, char* is_null, char* error)
{
    try
    {
        // если сокета нет, выходим без ошибок
        auto ptr = initid->ptr;
        if (!ptr)
        {
            *is_null = 1;
            *error = 0;
            return 0;
        }

        // буфер указателей
        captor::numbuf buf;

        // формируем пакет
        if (make_packet(buf, args))
        {
            // подключаем сокет
            captor::netcat netcat(ptr);

            // отправляем, выдаем ответ
            return netcat.send(buf);
        }
    }
    catch (const std::exception& e)
    {
        j.cerr([&]{
            auto text = std::mkstr(std::cref("netcat: "), 320);
            text += e.what();
            return text;
        });
    }
    catch (...)
    {
        j.cerr([&]{
            return "netcat :*(";
        });
    }

    // если была ошибка, проверяем надо ли ее обрабатывать
    bool continue_if_fail = false;
    auto addr_val = args->args[0];
    if (args->arg_type[0] == INT_RESULT)
    {
       if (*reinterpret_cast<long long*>(addr_val) < 0)
            continue_if_fail = true;
    }
    else
    {
        if (addr_val[0] == '-')
            continue_if_fail = true;
    }

    *is_null = 0;

    if (continue_if_fail)
        *error = 0;
    else
        *error = 1;

    return 0;
}

extern "C" void netcat_deinit(UDF_INIT* initid)
{
    static const ref::string nl(std::cref("]\n"));

    try
    {
        auto ptr = initid->ptr;
        if (ptr)
        {
            captor::refbuf buf;
            captor::netcat netcat(ptr);
            buf.append(nl);
            netcat.send(buf);
            netcat.close();
        }
    }
    catch (const std::exception& e)
    {
        j.cerr([&]{
            auto text = std::mkstr(std::cref("netcat_deinit: "), 320);
            text += e.what();
            return text;
        });
    }
    catch (...)
    {
        j.cerr([&]{
            return "netcat_deinit :*(";
        });
    }
}
