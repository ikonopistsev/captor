#include "refbuf.hpp"

#include "btdef/conv/to_text.hpp"

namespace captor {

iovec_iterator iovec_append(iovec_iterator i,
    char* data, unsigned long size) noexcept
{
    *i++ = { data, size };
    return i;
}

void refbuf::append(char* data, unsigned long size)
{
    if (curr_ < data_.end())
    {
        // добавляем ссылку на данные
        curr_ = iovec_append(curr_, data, size);
    }
    else
        throw std::length_error("too many refs");
}

void numbuf::append(std::int64_t val)
{
    // сохраняем указатель
    auto p = text_.end();
    // пишем новое число
    text_ += btdef::conv::to_text(val);
    // добавляем число
    refbuf::append(p, static_cast<unsigned long>(
        std::distance(p, text_.end())));
}

} // namespace captor
