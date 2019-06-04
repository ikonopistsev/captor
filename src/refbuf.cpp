#include "refbuf.hpp"

namespace captor {

iovec_iterator iovec_append(iovec_iterator i,
    char* data, unsigned long size) noexcept
{
    *i++ = { data, size };
    return i;
}

void refbuf::append(char* data, unsigned long size) noexcept
{
    assert(curr_ < data_.end());
    curr_ = iovec_append(curr_, data, size);
}

void numbuf::append(std::int64_t val) noexcept
{
    // сохраняем указатель
    auto p = text_.end();
    // пишем новое число
    text_ += utility::to_text(val);
    // добавляем число
    refbuf::append(p, static_cast<unsigned long>(
        std::distance(p, text_.end())));
}

} // namespace captor
