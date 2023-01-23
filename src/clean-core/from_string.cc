#include "from_string.hh"

#include <cerrno>
#include <cstdio>
#include <cstdlib>

#include <clean-core/macros.hh>
#include <clean-core/string_view.hh>

bool cc::from_string(char const* c_str, int8_t& out_value)
{
    int32_t full_int = 0;
    bool success = cc::from_string(c_str, full_int);
    out_value = int8_t(full_int);
    return success && full_int == int32_t(out_value);
}

bool cc::from_string(char const* c_str, int16_t& out_value)
{
    int32_t full_int = 0;
    bool success = cc::from_string(c_str, full_int);
    out_value = int16_t(full_int);
    return success && full_int == int32_t(out_value);
}

bool cc::from_string(char const* c_str, int32_t& out_value)
{
    char* end;
    errno = 0;
    out_value = int32_t(std::strtol(c_str, &end, 10));
    // end == str: parse error, no conversion done
    // end[0] = '\0': entire string read
    // errno != 0: range violation
    return end != c_str && end[0] == '\0' && errno == 0;
}

bool cc::from_string(char const* c_str, int64_t& out_value)
{
    char* end;
    errno = 0;
    out_value = int64_t(std::strtoll(c_str, &end, 10));
    return end != c_str && end[0] == '\0' && errno == 0;
}

bool cc::from_string(char const* c_str, uint8_t& out_value)
{
    uint32_t full_int = 0;
    bool success = cc::from_string(c_str, full_int);
    out_value = uint8_t(full_int);
    return success && full_int == uint32_t(out_value);
}

bool cc::from_string(char const* c_str, uint16_t& out_value)
{
    uint32_t full_int = 0;
    bool success = cc::from_string(c_str, full_int);
    out_value = uint16_t(full_int);
    return success && full_int == uint32_t(out_value);
}

bool cc::from_string(char const* c_str, uint32_t& out_value)
{
    char* end;
    errno = 0;
    out_value = uint32_t(std::strtoul(c_str, &end, 10));
    return end != c_str && end[0] == '\0' && errno == 0;
}

bool cc::from_string(char const* c_str, uint64_t& out_value)
{
    char* end;
    errno = 0;
    out_value = uint64_t(std::strtoull(c_str, &end, 10));
    return end != c_str && end[0] == '\0' && errno == 0;
}

bool cc::from_string(char const* c_str, float& out_value)
{
    char* end;
    errno = 0;
    out_value = std::strtof(c_str, &end);
    return end != c_str && end[0] == '\0' && errno == 0;
}

bool cc::from_string(char const* c_str, double& out_value)
{
    char* end;
    errno = 0;
    out_value = std::strtod(c_str, &end);
    return end != c_str && end[0] == '\0' && errno == 0;
}

bool cc::from_string(cc::string_view str, int8_t& out_value)
{
    char c_str_buf[256];
    std::snprintf(c_str_buf, sizeof(c_str_buf), "%.*s", int(str.size()), str.data());
    return from_string(c_str_buf, out_value);
}

bool cc::from_string(cc::string_view str, int16_t& out_value)
{
    char c_str_buf[256];
    std::snprintf(c_str_buf, sizeof(c_str_buf), "%.*s", int(str.size()), str.data());
    return from_string(c_str_buf, out_value);
}

bool cc::from_string(cc::string_view str, int32_t& out_value)
{
    char c_str_buf[256];
    std::snprintf(c_str_buf, sizeof(c_str_buf), "%.*s", int(str.size()), str.data());
    return from_string(c_str_buf, out_value);
}

bool cc::from_string(cc::string_view str, int64_t& out_value)
{
    char c_str_buf[256];
    std::snprintf(c_str_buf, sizeof(c_str_buf), "%.*s", int(str.size()), str.data());
    return from_string(c_str_buf, out_value);
}

bool cc::from_string(cc::string_view str, uint8_t& out_value)
{
    char c_str_buf[256];
    std::snprintf(c_str_buf, sizeof(c_str_buf), "%.*s", int(str.size()), str.data());
    return from_string(c_str_buf, out_value);
}

bool cc::from_string(cc::string_view str, uint16_t& out_value)
{
    char c_str_buf[256];
    std::snprintf(c_str_buf, sizeof(c_str_buf), "%.*s", int(str.size()), str.data());
    return from_string(c_str_buf, out_value);
}

bool cc::from_string(cc::string_view str, uint32_t& out_value)
{
    char c_str_buf[256];
    std::snprintf(c_str_buf, sizeof(c_str_buf), "%.*s", int(str.size()), str.data());
    return from_string(c_str_buf, out_value);
}

bool cc::from_string(cc::string_view str, uint64_t& out_value)
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

bool cc::from_string(cc::string_view str, char& v)
{
    if (str.size() != 1)
        return false;

    v = str[0];
    return true;
}

bool cc::from_string(cc::string_view str, bool& v)
{
    if (str == "true")
    {
        v = true;
        return true;
    }
    else if (str == "false")
    {
        v = false;
        return true;
    }
    else
        return false;
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
