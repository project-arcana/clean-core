#include "from_string.hh"

#include <cerrno>
#include <cstdio>
#include <cstdlib>

#include <clean-core/macros.hh>
#include <clean-core/string_view.hh>

bool cc::from_string(char const* c_str, int8& out_value)
{
    int32 full_int = 0;
    bool success = cc::from_string(c_str, full_int);
    out_value = int8(full_int);
    return success && full_int == int32(out_value);
}

bool cc::from_string(char const* c_str, int16& out_value)
{
    int32 full_int = 0;
    bool success = cc::from_string(c_str, full_int);
    out_value = int16(full_int);
    return success && full_int == int32(out_value);
}

bool cc::from_string(char const* c_str, int32& out_value)
{
    char* end;
    errno = 0;
    out_value = int32(std::strtol(c_str, &end, 10));
    // end == str: parse error, no conversion done
    // errno != 0: range violation
    return end != c_str && errno == 0;
}

bool cc::from_string(char const* c_str, int64& out_value)
{
    char* end;
    errno = 0;
    out_value = int64(std::strtoll(c_str, &end, 10));
    // end == str: parse error, no conversion done
    // errno != 0: range violation
    return end != c_str && errno == 0;
}

bool cc::from_string(char const* c_str, uint8& out_value)
{
    uint32 full_int = 0;
    bool success = cc::from_string(c_str, full_int);
    out_value = uint8(full_int);
    return success && full_int == uint32(out_value);
}

bool cc::from_string(char const* c_str, uint16& out_value)
{
    uint32 full_int = 0;
    bool success = cc::from_string(c_str, full_int);
    out_value = uint16(full_int);
    return success && full_int == uint32(out_value);
}

bool cc::from_string(char const* c_str, uint32& out_value)
{
    char* end;
    errno = 0;
    out_value = uint32(std::strtoul(c_str, &end, 10));
    // end == str: parse error, no conversion done
    // errno != 0: range violation
    return end != c_str && errno == 0;
}

bool cc::from_string(char const* c_str, uint64& out_value)
{
    char* end;
    errno = 0;
    out_value = uint64(std::strtoull(c_str, &end, 10));
    // end == str: parse error, no conversion done
    // errno != 0: range violation
    return end != c_str && errno == 0;
}

bool cc::from_string(char const* c_str, float& out_value)
{
    char* end;
    out_value = std::strtof(c_str, &end);
    return end != c_str;
}

bool cc::from_string(char const* c_str, double& out_value)
{
    char* end;
    out_value = std::strtod(c_str, &end);
    return end != c_str;
}

bool cc::from_string(cc::string_view str, int8& out_value)
{
    char c_str_buf[256];
    std::snprintf(c_str_buf, sizeof(c_str_buf), "%.*s", int(str.size()), str.data());
    return from_string(c_str_buf, out_value);
}

bool cc::from_string(cc::string_view str, int16& out_value)
{
    char c_str_buf[256];
    std::snprintf(c_str_buf, sizeof(c_str_buf), "%.*s", int(str.size()), str.data());
    return from_string(c_str_buf, out_value);
}

bool cc::from_string(cc::string_view str, int32& out_value)
{
    char c_str_buf[256];
    std::snprintf(c_str_buf, sizeof(c_str_buf), "%.*s", int(str.size()), str.data());
    return from_string(c_str_buf, out_value);
}

bool cc::from_string(cc::string_view str, int64& out_value)
{
    char c_str_buf[256];
    std::snprintf(c_str_buf, sizeof(c_str_buf), "%.*s", int(str.size()), str.data());
    return from_string(c_str_buf, out_value);
}

bool cc::from_string(cc::string_view str, uint8& out_value)
{
    char c_str_buf[256];
    std::snprintf(c_str_buf, sizeof(c_str_buf), "%.*s", int(str.size()), str.data());
    return from_string(c_str_buf, out_value);
}

bool cc::from_string(cc::string_view str, uint16& out_value)
{
    char c_str_buf[256];
    std::snprintf(c_str_buf, sizeof(c_str_buf), "%.*s", int(str.size()), str.data());
    return from_string(c_str_buf, out_value);
}

bool cc::from_string(cc::string_view str, uint32& out_value)
{
    char c_str_buf[256];
    std::snprintf(c_str_buf, sizeof(c_str_buf), "%.*s", int(str.size()), str.data());
    return from_string(c_str_buf, out_value);
}

bool cc::from_string(cc::string_view str, uint64& out_value)
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
