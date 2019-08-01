#include "netcat.hpp"

#include "btdef/date.hpp"

#include <mysql.h>
#include <cstdlib>

#include <sys/uio.h>

namespace captor {

static const ref::string nl(std::cref("\n"));

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
    // небыол ни одного 100% полезного свойства у неблокируемого сокета
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

void netcat::send_header(const char* cmd, unsigned long cmd_size,
    const char* param, unsigned long param_size)
{
    static const ref::string sp(std::cref(" "));

    numbuf buf;
    buf.append(btdef::date::now().time());
    buf.append(sp);
    buf.append(cmd, cmd_size);
    buf.append(sp);
    buf.append(param, param_size);
    buf.append(nl);
    buf.append(nl);
    send(buf);
}

// слишком большие пакеты не отправит
// FIXME
// вообще тут может быть косяк
// если за раз не отправятся все буфера
// но dll используется по локалхосту
// в режиме: покдлючение, отправка, отключение
// dll особо не подходит для больших пакетов
// TODO
// посчитать сумарный размер, сделать досылку
// это гемор, надо хранить общий размер данных и тд
long long netcat::send(const refbuf& buf) const
{
    auto res = ::writev(socket_.fd(), buf.data(), static_cast<int>(buf.size()));
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
                    "netcat(addr, cmd, param, json-data[, route])",
                    MYSQL_ERRMSG_SIZE);
            return 1;
        }

        // проверяем на количество роутов
        if (args_count > 5)
        {
            strncpy(msg, "to many routes", MYSQL_ERRMSG_SIZE);
            return 1;
        }

        if (args_count > 4)
        {
            decltype(args_count) i = 3;
            while (++i < args_count)
            {
                auto type = args->arg_type[i];
                // простые роуты это числа
                if (!((type == INT_RESULT) || (type == STRING_RESULT)))
                {
                    snprintf(msg, MYSQL_ERRMSG_SIZE,
                             "route[%d] must be int or string", i - 3);
                    return 1;
                }
            };
        }

        auto addr_val = args->args[0];
        auto addr_size = args->lengths[0];
        auto cmd_val = args->args[1];
        auto cmd_size = args->lengths[1];
        auto param_val = args->args[2];
        auto param_size = args->lengths[2];

        if (!(addr_val && addr_size &&
              cmd_val && cmd_size && param_val && param_size))
        {
            strncpy(msg, "bad args, use "
                    "netcat(addr, cmd, param, json-data, route...)",
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

        // отправляем хидер
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
    auto size = args->lengths[3];

    if (!(data && size))
    {
        // если нет данных - выходим
        return false;
    }

    // пишем данные
    buf.append(data, size);
    // перевод строки
    buf.append(captor::nl);

    // пишем маршруты
    for (unsigned int i = 4; i < args->arg_count; ++i)
    {
        auto val = args->args[i];
        if (!val)
        {
            // выходим если маршурт null
            return false;
        }

        auto val_size = args->lengths[i];

        // проверяем на простой роут
        if (args->arg_type[i] == INT_RESULT)
        {
            // добавляем ключ если он указан
            auto k = args->attributes[i];
            auto ks = args->attribute_lengths[i];
            if (k && (ks > 0))
            {
                // разделитель маршрута
                static const ref::string sep(std::cref("="));
                // пишем ключ
                buf.append(k, ks);
                // пишем разделитель
                buf.append(sep);
            }
            // добавляем число
            buf.append(*reinterpret_cast<long long*>(val));
        }
        else
        {
            // добавляем строковый роут
            buf.append(val, val_size);
        }

        // окончание маршрута
        buf.append(captor::nl);
    }

    // завершаем все переводом строки, чтобы получился двойной перевод
    buf.append(captor::nl);

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
    catch (...)
    {   }

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
    try
    {
        auto ptr = initid->ptr;
        if (ptr)
        {
            captor::netcat netcat(ptr);
            netcat.close();
        }
    }
    catch (...)
    {   }
}
