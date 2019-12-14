#pragma once

#include "refbuf.hpp"

#include "btpro/socket.hpp"

namespace captor {

class netcat
{
    btpro::socket socket_{};

public:

    netcat() = default;

    netcat(char* ptr) noexcept;

    void close() noexcept;

    void attach(evutil_socket_t fd) noexcept;

    void attach(char* ptr) noexcept
    {
        attach(static_cast<evutil_socket_t>(
            reinterpret_cast<std::intptr_t>(ptr)));
    }

    evutil_socket_t fd() const noexcept
    {
        return socket_.fd();
    }

    char* char_fd() const noexcept
    {
        return reinterpret_cast<char*>(static_cast<std::intptr_t>(fd()));
    }

    void connect(const btpro::ip::addr& dest);

    void send_header(const char* method, unsigned long method_size);

    void send_header(const char* method, unsigned long method_size,
        const char* exchange, unsigned long exchange_size);

    long long send(const refbuf& buf) const;
};

} // namespace captor
