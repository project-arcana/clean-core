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

namespace
{
// to_string implementations for builtin types
// format_spec ::=  [[fill]align][sign]["#"]["0"][width]["." precision][type]
// fill        ::=  <a character other than '{' or '}'>
// align       ::=  "<" | ">" | "^"
// sign        ::=  "+" | "-" | " "
// width       ::=  integer | "{" arg_id "}"
// precision   ::=  integer | "{" arg_id "}"
// type        ::=  int_type | "a" | "A" | "c" | "e" | "E" | "f" | "F" | "g" | "G" | "L" | "p" | "s"
// int_type    ::=  "b" | "B" | "d" | "o" | "x" | "X"
struct parsed_fmt_args
{
    int width = -1;                       // width of the replacement, use fill to match width
    int precision = -1;                   // for float/double: precision after comma / dot. For other types: max length
    char fill = ' ';                      // anything but '{' or '}'
    char align = '>';                     // '<': left | '>': right | '^': center
    char sign = '-';                      // '+': show both + and - | '-': show only - | ' ': show only -, but leave space if +
    char type = 0;                        // type: individual for each type. Sometimes determines how type should be interpreted, eg char
    bool alternative_mode = false;        // alternative display mode
    bool sign_aware_zero_padding = false; // add leading zeros after the sign +0000123
};

parsed_fmt_args parse_args(cc::string_view fmt_args)
{
    auto const is_align = [](char c) { return c == '<' || c == '>' || c == '^'; };
    auto const is_sign = [](char c) { return c == '+' || c == '-' || c == ' '; };
    auto const is_digit = [](char c) { return '0' <= c && c <= '9'; };
    auto const parse_unsigned_int = [&](char const* begin, char const* end, int& out) -> char const* {
        out = *begin - '0';
        ++begin;
        while (begin != end && is_digit(*begin))
        {
            out *= 10;
            out += *begin - '0';
            ++begin;
        }
    };

    auto current = fmt_args.begin();
    auto const end = fmt_args.end();

    parsed_fmt_args result;

    if (current == end)
        return result;

    // fill
    if (is_align(*current))
    {
        result.align = *current;
        ++current;
    }
    // alignment and fill
    else if ((current + 1) != end && is_align(*(current + 1)))
    {
        result.fill = *current;
        ++current;
        result.align = *current;
        ++current;
    }
    // sign
    if (current != end && is_sign(*current))
    {
        result.sign = *current;
        ++current;
    }
    // alternative mode
    if (current != end && (*current == '#'))
    {
        result.alternative_mode = true;
        ++current;
    }
    // zero padding and width
    if (current != end && (*current == '0'))
    {
        result.sign_aware_zero_padding = true;
        ++current;
        CC_ASSERT(current != end && is_digit(*current) && "Invalid format string: Zero padding must be followed by by width");
        CC_ASSERT(*current != '0' && "Invalid format string: Width can have at most one preceeding zero");
    }
    // width
    if (current != end && is_digit(*current))
    {
        current = parse_unsigned_int(current, end, result.width);
    }
    // precision
    if (current != end && *current == '.')
    {
        ++current;
        CC_ASSERT(current != end && is_digit(*current) && "Invalid format string: . must be followed by precision");
        current = parse_unsigned_int(current, end, result.precision);
    }
    // type
    if (current != end)
    {
        result.type = *current;
        ++current;
    }
    CC_ASSERT(current == end && "Invalid format string: Malformed argument");

    return result;
}

constexpr bool is_int_type(char c) { return c == 'd' || c == 'b' || c == 'B' || c == 'o' || c == 'x' || c == 'X'; }
constexpr bool is_float_type(char c) { return c == 'a' || c == 'A' || c == 'e' || c == 'E' || c == 'f' || c == 'F' || c == 'g' || c == 'G'; }

template <class IntType>
void to_string_int_impl(IntType value, parsed_fmt_args const& args)
{
    switch (args.type)
    {
    case 'd': // default, decimal
        break;
    case 'b': // binary, with # prefix 0b
        break;
    case 'B': // binary, with # prefix 0B
        break;
    case 'o': // octal
        break;
    case 'x': // hexadecimal, with # prefix 0x
        break;
    case 'X': // hexadecimal, with # prefix 0X
        break;
    default:
        CC_ASSERT(false && "Invalid format string: Unsupported argument type for int type");
    }
}

template <class FloatType>
void to_string_float_impl(FloatType value, parsed_fmt_args const& args)
{
    switch (args.type)
    {
    case 0: // default, like g but always one digit after decimal point, when fixed point notation is used
        break;
    case 'a': // hexadecimal 0x prefix, 'p' exponent, lowercase letters
        break;
    case 'A': // hexadecimal 0x prefix, 'P' exponent, uppercase letters
        break;
    case 'e': // scientific exponent notation, 'e' exponent
        break;
    case 'E': // scientific exponent notation, 'E' exponent
        break;
    case 'f': // fixed-point
        break;
    case 'F': // fixed-point, but "nan" and "inf" become "NAN" and "INF"
        break;
    case 'g': // general form, precision significant digits and fixed-point or exponent notation depending on magnitute
        break;
    case 'G': // same as 'g' but uppercase version
        break;
    default:
        CC_ASSERT(false && "Invalid format string: Unsupported argument type for float type");
    }
}

}

void cc::to_string(cc::string_stream& ss, char value, cc::string_view fmt_str)
{
    // todo: alignment

    auto const args = parse_args(fmt_str);
    if (args.type == 0 || args.type == 'c')
    { // handle as character

        ss << string_view(&value, 1);
    }
    else
    { // handle as int
        CC_ASSERT(is_int_type(args.type) && "Invalid format string: wrong argument type for char");
        // todo
    }
}
void cc::to_string(cc::string_stream& ss, bool value, cc::string_view fmt_str)
{
    auto const args = parse_args(fmt_str);
    if (args.type == 0) // default
    {
        ss << (value ? "true" : "false");
    }
    else
    { // handle as int
        CC_ASSERT(is_int_type(args.type) && "Invalid format string: wrong argument type for char");
        // todo
    }
}
void cc::to_string(cc::string_stream& ss, char const* value, cc::string_view fmt_str)
{
    if (fmt_str.empty())
        ss << (value ? value : "[nullptr]");

    auto const args = parse_args(fmt_str);
    // todo
}
void cc::to_string(cc::string_stream& ss, cc::string_view value, cc::string_view fmt_str)
{
    if (fmt_str.empty())
        ss << value;

    auto const args = parse_args(fmt_str);
    // todo
}
void cc::to_string(cc::string_stream& ss, nullptr_t, cc::string_view fmt_str)
{
    if (fmt_str.empty())
        ss << "nullptr";
    auto const args = parse_args(fmt_str);
    // todo
}

void cc::to_string(cc::string_stream& ss, void* value, cc::string_view fmt_str)
{
    if (fmt_str.empty())
        ss << to_string(value);
    auto const args = parse_args(fmt_str);
    // todo
}

void cc::to_string(cc::string_stream& ss, std::byte value, cc::string_view fmt_str)
{
    if (fmt_str.empty())
        ss << to_string(value);
    auto const args = parse_args(fmt_str);
    // todo
}

void cc::to_string(cc::string_stream& ss, int value, cc::string_view fmt_str)
{
    if (fmt_str.empty())
        ss << to_string(value);
    auto const args = parse_args(fmt_str);
    // todo
}
void cc::to_string(cc::string_stream& ss, long value, cc::string_view fmt_str)
{
    if (fmt_str.empty())
        ss << to_string(value);
    auto const args = parse_args(fmt_str);
    // todo
}
void cc::to_string(cc::string_stream& ss, long long value, cc::string_view fmt_str)
{
    if (fmt_str.empty())
        ss << to_string(value);
    auto const args = parse_args(fmt_str);
    // todo
}
void cc::to_string(cc::string_stream& ss, unsigned int value, cc::string_view fmt_str)
{
    if (fmt_str.empty())
        ss << to_string(value);
    auto const args = parse_args(fmt_str);
    // todo
}
void cc::to_string(cc::string_stream& ss, unsigned long value, cc::string_view fmt_str)
{
    if (fmt_str.empty())
        ss << to_string(value);
    auto const args = parse_args(fmt_str);
    // todo
}
void cc::to_string(cc::string_stream& ss, unsigned long long value, cc::string_view fmt_str)
{
    if (fmt_str.empty())
        ss << to_string(value);
    auto const args = parse_args(fmt_str);
    // todo
}
void cc::to_string(cc::string_stream& ss, float value, cc::string_view fmt_str)
{
    if (fmt_str.empty())
        ss << to_string(value);
    // todo
}
void cc::to_string(cc::string_stream& ss, double value, cc::string_view fmt_str)
{
    if (fmt_str.empty())
        ss << to_string(value);
    auto const args = parse_args(fmt_str);
    // todo
}
void cc::to_string(cc::string_stream& ss, long double value, cc::string_view fmt_str)
{
    if (fmt_str.empty())
        ss << to_string(value);
    auto const args = parse_args(fmt_str);
    // todo
}
