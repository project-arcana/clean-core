#include "to_string.hh"

#include <cinttypes>
#include <cstdio>

cc::string cc::to_string(void* value)
{
    if (value == nullptr)
        return "nullptr";

    char buffer[16 + 2 + 1];
    auto res = std::snprintf(buffer, sizeof(buffer), "0x%.16zx", size_t(value));
    CC_ASSERT(res >= 0);
    return string_view(buffer, res);
}

cc::string cc::to_string(int value)
{
    char buffer[10 + 1 + 1];
    auto res = std::snprintf(buffer, sizeof(buffer), "%d", value);
    CC_ASSERT(res >= 0);
    return string_view(buffer, res);
}

cc::string cc::to_string(long value)
{
    char buffer[19 + 1 + 1];
    auto res = std::snprintf(buffer, sizeof(buffer), "%ld", value);
    CC_ASSERT(res >= 0);
    return string_view(buffer, res);
}

cc::string cc::to_string(long long value)
{
    char buffer[19 + 1 + 1];
    auto res = std::snprintf(buffer, sizeof(buffer), "%lld", value);
    CC_ASSERT(res >= 0);
    return string_view(buffer, res);
}

cc::string cc::to_string(unsigned int value)
{
    char buffer[10 + 1];
    auto res = std::snprintf(buffer, sizeof(buffer), "%u", value);
    CC_ASSERT(res >= 0);
    return string_view(buffer, res);
}

cc::string cc::to_string(unsigned long value)
{
    char buffer[20 + 1];
    auto res = std::snprintf(buffer, sizeof(buffer), "%lu", value);
    CC_ASSERT(res >= 0);
    return string_view(buffer, res);
}

cc::string cc::to_string(unsigned long long value)
{
    char buffer[20 + 1];
    auto res = std::snprintf(buffer, sizeof(buffer), "%llu", value);
    CC_ASSERT(res >= 0);
    return string_view(buffer, res);
}

cc::string cc::to_string(float value)
{
    char buffer[20 + 1];
    auto res = std::snprintf(buffer, sizeof(buffer), "%g", value);
    CC_ASSERT(res >= 0);
    return string_view(buffer, res);
}

cc::string cc::to_string(double value)
{
    char buffer[30 + 1];
    auto res = std::snprintf(buffer, sizeof(buffer), "%g", value);
    CC_ASSERT(res >= 0);
    return string_view(buffer, res);
}

cc::string cc::to_string(long double value)
{
    char buffer[40 + 1];
    auto res = std::snprintf(buffer, sizeof(buffer), "%Lg", value);
    CC_ASSERT(res >= 0);
    return string_view(buffer, res);
}

cc::string cc::to_string(std::byte value)
{
    static constexpr auto hex = "0123456789ABCDEF";
    auto v = uint8_t(value);
    auto s = cc::string::uninitialized(2);
    s[0] = hex[v % 16];
    s[1] = hex[v / 16];
    return s;
}


// to_string implementations for builtin types
// format_spec ::=  [[fill]align][sign]["#"]["0"][width]["." precision][type]
// fill        ::=  <a character other than '{' or '}'>
// align       ::=  "<" | ">" | "^"
// sign        ::=  "+" | "-" | " "
// width       ::=  integer | "{" arg_id "}"
// precision   ::=  integer | "{" arg_id "}"
// type        ::=  int_type | "a" | "A" | "c" | "e" | "E" | "f" | "F" | "g" | "G" | "L" | "p" | "s"
// int_type    ::=  "b" | "B" | "d" | "o" | "x" | "X"
namespace
{
struct int_fmt_args
{
    char fill = 0;                        // anything but '{' or '}', only valid if align valid
    char align = 0;                       // '<': left | '>': right | '^': center
    char sign = '-';                      // '+': show both + and - | '-': show only - | ' ': show only -, but leave space if +
    bool alternative_mode = false;        // alternative display mode
    bool sign_aware_zero_padding = false; // add leading zeros after the sign +0000123
    int width = -1;                       //
    char type = 'd';                      // types 'b': binary | 'B': Binary | 'd': decimal | 'o': octal | 'x':hex | 'X': heX [ | L: locale aware]
};


// todo: generic parse int
void parse_int_args(int v, cc::string_view fmt_args)
{
    auto const is_align = [](char c) { return c == '<' || c == '>' || c == '^'; };
    auto const is_sign = [](char c) { return c == '+' || c == '-' || c == ' '; };
    auto const is_digit = [](char c) { return '0' <= c && c <= '9'; };

    auto it = fmt_args.begin();
    auto const end = fmt_args.end();

    char fill;
    char const* align = nullptr; // != nullptr if aligned
    char sign = '-';             // default
    bool alternative = false;
    bool sign_aware_zero_padding = false;
    int width = -1;
    char type = 'd';

    // fill and alignment
    if (fmt_args.size() >= 2 && is_align(*(it + 1)))
    {
        fill = *it;
        ++it;
        align = it;
        ++it;
    }
    // sign
    if (it != end && is_sign(*it))
    {
        sign = *it;
        ++it;
    }
    // alternative mode
    if (it != end && (*it == '#'))
    {
        alternative = true;
        ++it;
    }
    // zero padding and width
    if (it != end && (*it == '0'))
    {
        sign_aware_zero_padding = true;
        ++it;
        CC_ASSERT(it != end && is_digit(*it) && "Invalid format string: zero padding must be followed by by width");
        CC_ASSERT(*it != '0' && "Invalid format string: width can have at most one preceeding zero");
        width = *it - '0';
        ++it;
        while (it != end && is_digit(*it))
        {
            width *= 10;
            width += *it - '0';
            ++it;
        }
    }
    // no zero padding and with
    if (it != end && is_digit(*it))
    {
        width = *it - '0';
        ++it;
        while (it != end && is_digit(*it))
        {
            width *= 10;
            width += *it - '0';
            ++it;
        }
    }
    // precision does not apply to integer and is therefore skipped
    // integer types
    if (it != end)
    {
        // todo: locale?
        CC_ASSERT((*it == 'b' || *it == 'B' || *it == 'd' || *it == 'o' || *it == 'x' || *it == 'X') && "Invalid format string: unsupported type");
        type = *it;
        ++it;
    }
    CC_ASSERT(it == end && "Invalid format string: invalid integer options");

    // todo: what do with settings?
}

void parse_float(float, cc::string_view fmt_args)
{
    auto const is_align = [](char c) { return c == '<' || c == '>' || c == '^'; };
    auto const is_sign = [](char c) { return c == '+' || c == '-' || c == ' '; };
    auto const is_digit = [](char c) { return '0' <= c && c <= '9'; };

    auto it = fmt_args.begin();
    auto const end = fmt_args.end();

    char fill;
    char const* align = nullptr; // != nullptr if aligned
    char sign = '-';             // default
    bool alternative = false;
    bool sign_aware_zero_padding = false;
    int width = -1;
    char type = 'd';
    int precision = -1;

    // fill and alignment
    if (fmt_args.size() >= 2 && is_align(*(it + 1)))
    {
        fill = *it;
        ++it;
        align = it;
        ++it;
    }
    // sign
    if (it != end && is_sign(*it))
    {
        sign = *it;
        ++it;
    }
    // alternative mode
    if (it != end && (*it == '#'))
    {
        alternative = true;
        ++it;
    }
    // zero padding and width
    if (it != end && (*it == '0'))
    {
        sign_aware_zero_padding = true;
        ++it;
        CC_ASSERT(it != end && is_digit(*it) && "Invalid format string: zero padding must be followed by by width");
        CC_ASSERT(*it != '0' && "Invalid format string: width can have at most one preceeding zero");
        width = *it - '0';
        ++it;
        while (it != end && is_digit(*it))
        {
            width *= 10;
            width += *it - '0';
            ++it;
        }
    }
    // no zero padding and with
    if (it != end && is_digit(*it))
    {
        width = *it - '0';
        ++it;
        while (it != end && is_digit(*it))
        {
            width *= 10;
            width += *it - '0';
            ++it;
        }
    }
    if (it != end && *it == '.')
    {
        ++it;
        CC_ASSERT(it != end && is_digit(*it) && "Invalid format string: . must be followed by the precision");
        precision = *it - '0';
        ++it;
        while (it != end && is_digit(*it))
        {
            precision *= 10;
            precision += *it - '0';
            ++it;
        }
    }

    // type
    if (it != end) // todo: locale?
    {
        CC_ASSERT((*it == 'a' || *it == 'A' || *it == 'e' || *it == 'E' || *it == 'f' || *it == 'F' || *it == 'g' || *it == 'G') && "Invalid format string: invalid type");
        type = *it;
        ++it;
    }
    CC_ASSERT(it == end && "Invalid format string: invalid integer options");
}
}

// void cc::to_string(cc::string_stream& ss, char value, cc::string_view fmt_str)
//{
//    // todo
//}
// void cc::to_string(cc::string_stream& ss, bool value, cc::string_view fmt_str)
//{
//    // todo
//}
// void cc::to_string(cc::string_stream& ss, char const* value, cc::string_view fmt_str)
//{
//    // todo
//}
// void cc::to_string(cc::string_stream& ss, cc::string_view value, cc::string_view fmt_str)
//{
//    // todo
//}
// void cc::to_string(cc::string_stream& ss, nullptr_t, cc::string_view fmt_str)
//{
//    // todo
//}

// void cc::to_string(cc::string_stream& ss, void* value, cc::string_view fmt_str)
//{
//    // todo
//}

// void cc::to_string(cc::string_stream& ss, std::byte value, cc::string_view fmt_str)
//{
//    // todo
//}

// void cc::to_string(cc::string_stream& ss, int value, cc::string_view fmt_str)
//{
//    // todo
//}
// void cc::to_string(cc::string_stream& ss, long value, cc::string_view fmt_str)
//{
//    // todo
//}
// void cc::to_string(cc::string_stream& ss, long long value, cc::string_view fmt_str)
//{
//    // todo
//}
// void cc::to_string(cc::string_stream& ss, unsigned int value, cc::string_view fmt_str)
//{
//    // todo
//}
// void cc::to_string(cc::string_stream& ss, unsigned long value, cc::string_view fmt_str)
//{
//    // todo
//}
// void cc::to_string(cc::string_stream& ss, unsigned long long value, cc::string_view fmt_str)
//{
//    // todo
//}

// void cc::to_string(cc::string_stream& ss, float value, cc::string_view fmt_str)
//{
//    // todo
//}
// void cc::to_string(cc::string_stream& ss, double value, cc::string_view fmt_str)
//{
//    // todo
//}
// void cc::to_string(cc::string_stream& ss, long double value, cc::string_view fmt_str)
//{
//    // todo
//}
