#pragma once

#include "btpro/socket.hpp"

#include <algorithm>

namespace captor {

// размер массива хранящего указатели на буфера отправки
#ifndef CAPTOR_IOVECARR_LEN
#define CAPTOR_IOVECARR_LEN 1024
#endif // CAPTOR_IOVECARR_LEN

// размера строки CAPTOR_NUMBUF_SIZE под печать чисел
#ifndef CAPTOR_NUMBUF_SIZE
#define CAPTOR_NUMBUF_SIZE 8192
#endif // CAPTOR_NUMBUF_SIZE

typedef std::array<iovec, CAPTOR_IOVECARR_LEN> iovec_type;
typedef iovec_type::iterator iovec_iterator;
typedef iovec_type::const_pointer const_pointer;

iovec_iterator iovec_append(iovec_iterator i,
    char* data, unsigned long size) noexcept;

class refbuf
{
    iovec_type data_{};
    iovec_iterator curr_{data_.begin()};

public:
    refbuf() = default;

    const_pointer data() const noexcept
    {
        return data_.data();
    }

    std::size_t size() const noexcept
    {
        return static_cast<std::size_t>(
            std::distance(const_cast<iovec_iterator>(data_.begin()), curr_));
    }

    void append(char* data, unsigned long size) noexcept;

    void append(const char* data, unsigned long size) noexcept
    {
        append(const_cast<char*>(data), size);
    }

    template<class T>
    void append(const T& str_val) noexcept
    {
        append(str_val.data(), str_val.size());
    }

    bool empty() const noexcept
    {
        return size() == 0;
    }

    bool drain(std::size_t len);
};

class numbuf
    : public refbuf
{
    btdef::util::basic_text<char, CAPTOR_NUMBUF_SIZE> text_{};

public:
    numbuf() = default;

    void append(std::int64_t val) noexcept;

    void append(long long val) noexcept
    {
        append(static_cast<std::int64_t>(val));
    }

    template<class T>
    void append(T* ptr, unsigned long size) noexcept
    {
        refbuf::append(ptr, size);
    }

    template<class T>
    void append(const T& str_val) noexcept
    {
        append(str_val.data(), str_val.size());
    }
};

} // namespace captor
