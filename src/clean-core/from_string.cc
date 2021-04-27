#include "from_string.hh"

#include <cstdio>
#include <cstdlib>

#include <clean-core/macros.hh>
#include <clean-core/string_view.hh>

#ifdef CC_OS_WINDOWS
// clang-format off
#include <clean-core/native/detail/win32_sanitize_before.inl>
#include <Windows.h>
#include <Shlwapi.h>
#include <clean-core/native/detail/win32_sanitize_after.inl>
// clang-format on
#pragma comment(lib, "Shlwapi.lib")
#else
#include <cerrno>
#endif

bool cc::from_string(char const* c_str, int32& out_value)
{
#ifdef CC_OS_WINDOWS
    return ::StrToIntExA(c_str, STIF_DEFAULT, &out_value);
#else
    char* end;
    errno = 0;
    out_value = int32(std::strtol(c_str, &end, 10));
    // end == str: parse error, no conversion done
    // errno != 0: range violation
    return end != c_str && errno == 0;
#endif
}

bool cc::from_string(char const* c_str, int64& out_value)
{
#ifdef CC_OS_WINDOWS
    return ::StrToInt64ExA(c_str, STIF_DEFAULT, &out_value);
#else
    char* end;
    errno = 0;
    out_value = int64(std::strtoll(c_str, &end, 10));
    // end == str: parse error, no conversion done
    // errno != 0: range violation
    return end != c_str && errno == 0;
#endif
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

bool cc::from_string(cc::string_view s, int32& out_value)
{
    char c_str_buf[256];
    std::snprintf(c_str_buf, sizeof(c_str_buf), "%.*s", int(s.size()), s.data());
    return from_string(c_str_buf, out_value);
}

bool cc::from_string(cc::string_view s, int64& out_value)
{
    char c_str_buf[256];
    std::snprintf(c_str_buf, sizeof(c_str_buf), "%.*s", int(s.size()), s.data());
    return from_string(c_str_buf, out_value);
}

bool cc::from_string(cc::string_view s, float& out_value)
{
    char c_str_buf[256];
    std::snprintf(c_str_buf, sizeof(c_str_buf), "%.*s", int(s.size()), s.data());
    return from_string(c_str_buf, out_value);
}

bool cc::from_string(cc::string_view s, double& out_value)
{
    char c_str_buf[256];
    std::snprintf(c_str_buf, sizeof(c_str_buf), "%.*s", int(s.size()), s.data());
    return from_string(c_str_buf, out_value);
}

bool cc::from_string(cc::string_view s, char& v)
{
    if (s.size() != 1)
        return false;

    v = s[0];
    return true;
}

bool cc::from_string(cc::string_view s, bool& v)
{
    if (s == "true")
    {
        v = true;
        return true;
    }
    else if (s == "false")
    {
        v = false;
        return true;
    }
    else
        return false;
}
