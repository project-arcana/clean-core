#include "from_string.hh"

#include <cerrno>
#include <cstdio>
#include <cstdlib>

#include <clean-core/macros.hh>
#include <clean-core/string_view.hh>


bool cc::from_string(char const* c_str, bool& out_value)
{
    if (!c_str)
        return false;

    if (std::strcmp(c_str, "true") == 0)
    {
        out_value = true;
        return true;
    }
    if (std::strcmp(c_str, "false") == 0)
    {
        out_value = false;
        return true;
    }
    return false;
}

bool cc::from_string(char const* c_str, char& out_value)
{
    if (!c_str)
        return false;

    if (std::strlen(c_str) != 1)
        return false;

    out_value = c_str[0];
    return true;
}

bool cc::from_string(char const* c_str, signed char& out_value)
{
    signed long full_long = 0;
    bool const success = cc::from_string(c_str, full_long);
    out_value = static_cast<signed char>(full_long);
    return success && full_long == static_cast<signed long>(out_value);
}

bool cc::from_string(char const* c_str, signed short& out_value)
{
    signed long full_long = 0;
    bool const success = cc::from_string(c_str, full_long);
    out_value = static_cast<signed short>(full_long);
    return success && full_long == static_cast<signed long>(out_value);
}

bool cc::from_string(char const* c_str, signed int& out_value)
{
    char* end = nullptr;
    errno = 0;
    long v = std::strtol(c_str, &end, 10);
    out_value = static_cast<signed int>(v);
    return end != c_str                          // parse error, no conversion done
           && end[0] == '\0'                     // entire string read
           && errno == 0                         // range violation
           && v == static_cast<long>(out_value); // input exceeds signed int
}

bool cc::from_string(char const* c_str, signed long& out_value)
{
    char* end = nullptr;
    errno = 0;
    out_value = std::strtol(c_str, &end, 10);
    return end != c_str      // parse error, no conversion done
           && end[0] == '\0' // entire string read
           && errno == 0;    // range violation
}

bool cc::from_string(char const* c_str, signed long long& out_value)
{
    char* end = nullptr;
    errno = 0;
    out_value = std::strtoll(c_str, &end, 10);
    return end != c_str      // parse error, no conversion done
           && end[0] == '\0' // entire string read
           && errno == 0;    // range violation
}

bool cc::from_string(char const* c_str, unsigned char& out_value)
{
    unsigned long full_long = 0;
    bool const success = cc::from_string(c_str, full_long);
    out_value = static_cast<unsigned char>(full_long);
    return success && full_long == static_cast<unsigned long>(out_value);
}

bool cc::from_string(char const* c_str, unsigned short& out_value)
{
    unsigned long full_long = 0;
    bool const success = cc::from_string(c_str, full_long);
    out_value = static_cast<unsigned short>(full_long);
    return success && full_long == static_cast<unsigned long>(out_value);
}

bool cc::from_string(char const* c_str, unsigned int& out_value)
{
    unsigned long full_long = 0;
    bool const success = cc::from_string(c_str, full_long);
    out_value = static_cast<unsigned int>(full_long);
    return success && full_long == static_cast<unsigned long>(out_value);
}

bool cc::from_string(char const* c_str, unsigned long& out_value)
{
    char* end = nullptr;
    errno = 0;
    out_value = std::strtoul(c_str, &end, 10);
    return end != c_str      // parse error, no conversion done
           && end[0] == '\0' // entire string read
           && errno == 0;    // range violation
}

bool cc::from_string(char const* c_str, unsigned long long& out_value)
{
    char* end = nullptr;
    errno = 0;
    out_value = std::strtoull(c_str, &end, 10);
    return end != c_str      // parse error, no conversion done
           && end[0] == '\0' // entire string read
           && errno == 0;    // range violation
}

bool cc::from_string(char const* c_str, float& out_value)
{
    char* end = nullptr;
    errno = 0;
    out_value = std::strtof(c_str, &end);
    return end != c_str && end[0] == '\0' && errno == 0;
}

bool cc::from_string(char const* c_str, double& out_value)
{
    char* end = nullptr;
    errno = 0;
    out_value = std::strtod(c_str, &end);
    return end != c_str && end[0] == '\0' && errno == 0;
}

bool cc::from_string(cc::string_view str, bool& out_value)
{
    if (str == "true")
    {
        out_value = true;
        return true;
    }
    else if (str == "false")
    {
        out_value = false;
        return true;
    }
    else
        return false;
}

bool cc::from_string(cc::string_view str, char& out_value)
{
    if (str.size() != 1)
        return false;

    out_value = str[0];
    return true;
}

bool cc::from_string(cc::string_view str, signed char& out_value)
{
    char c_str_buf[256];
    std::snprintf(c_str_buf, sizeof(c_str_buf), "%.*s", int(str.size()), str.data());
    return from_string(c_str_buf, out_value);
}

bool cc::from_string(cc::string_view str, signed short& out_value)
{
    char c_str_buf[256];
    std::snprintf(c_str_buf, sizeof(c_str_buf), "%.*s", int(str.size()), str.data());
    return from_string(c_str_buf, out_value);
}

bool cc::from_string(cc::string_view str, signed int& out_value)
{
    char c_str_buf[256];
    std::snprintf(c_str_buf, sizeof(c_str_buf), "%.*s", int(str.size()), str.data());
    return from_string(c_str_buf, out_value);
}

bool cc::from_string(cc::string_view str, signed long& out_value)
{
    char c_str_buf[256];
    std::snprintf(c_str_buf, sizeof(c_str_buf), "%.*s", int(str.size()), str.data());
    return from_string(c_str_buf, out_value);
}

bool cc::from_string(cc::string_view str, signed long long& out_value)
{
    char c_str_buf[256];
    std::snprintf(c_str_buf, sizeof(c_str_buf), "%.*s", int(str.size()), str.data());
    return from_string(c_str_buf, out_value);
}

bool cc::from_string(cc::string_view str, unsigned char& out_value)
{
    char c_str_buf[256];
    std::snprintf(c_str_buf, sizeof(c_str_buf), "%.*s", int(str.size()), str.data());
    return from_string(c_str_buf, out_value);
}

bool cc::from_string(cc::string_view str, unsigned short& out_value)
{
    char c_str_buf[256];
    std::snprintf(c_str_buf, sizeof(c_str_buf), "%.*s", int(str.size()), str.data());
    return from_string(c_str_buf, out_value);
}

bool cc::from_string(cc::string_view str, unsigned int& out_value)
{
    char c_str_buf[256];
    std::snprintf(c_str_buf, sizeof(c_str_buf), "%.*s", int(str.size()), str.data());
    return from_string(c_str_buf, out_value);
}

bool cc::from_string(cc::string_view str, unsigned long& out_value)
{
    char c_str_buf[256];
    std::snprintf(c_str_buf, sizeof(c_str_buf), "%.*s", int(str.size()), str.data());
    return from_string(c_str_buf, out_value);
}

bool cc::from_string(cc::string_view str, unsigned long long& out_value)
{
    char c_str_buf[256];
    std::snprintf(c_str_buf, sizeof(c_str_buf), "%.*s", int(str.size()), str.data());
    return from_string(c_str_buf, out_value);
}

bool cc::from_string(cc::string_view str, float& out_value)
{
    char c_str_buf[256];
    std::snprintf(c_str_buf, sizeof(c_str_buf), "%.*s", int(str.size()), str.data());
    return from_string(c_str_buf, out_value);
}

bool cc::from_string(cc::string_view str, double& out_value)
{
    char c_str_buf[256];
    std::snprintf(c_str_buf, sizeof(c_str_buf), "%.*s", int(str.size()), str.data());
    return from_string(c_str_buf, out_value);
}

bool cc::from_string(string_view str, std::byte& out_value)
{
    if (str.size() != 2)
        return false;

    auto hex_from_char = [](char c) -> int
    {
        if ('0' <= c && c <= '9')
            return c - '0';

        if ('a' <= c && c <= 'f')
            return 10 + (c - 'a');

        if ('A' <= c && c <= 'F')
            return 10 + (c - 'A');

        return -1;
    };

    auto i0 = hex_from_char(str[0]);
    auto i1 = hex_from_char(str[1]);
    if (i0 == -1 || i1 == -1)
        return false;

    out_value = std::byte((i0 << 4) + i1);
    return true;
}
