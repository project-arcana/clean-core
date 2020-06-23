#include "wchar_conversion.hh"

#include <cwchar>

#include <clean-core/macros.hh>
#include <clean-core/utility.hh>

#include <clean-core/native/win32_sanitized.hh>

int cc::widechar_to_char(cc::span<char> dest, const wchar_t* src, int opt_num_src_chars)
{
#ifdef CC_OS_WINDOWS
    return ::WideCharToMultiByte(CP_UTF8, 0, src, opt_num_src_chars, dest.data(), int(dest.size()), nullptr, nullptr);
#else
    std::mbstate_t state = {};
    int num_dest_chars = opt_num_src_chars > 0 ? cc::min(int(dest.size()), opt_num_src_chars / int(sizeof(wchar_t))) : int(dest.size());
    return int(std::wcsrtombs(dest.data(), &src, size_t(num_dest_chars), &state));
#endif
}

int cc::char_to_widechar(cc::span<wchar_t> dest, const char* src, int opt_num_src_chars)
{
#ifdef CC_OS_WINDOWS
    return ::MultiByteToWideChar(CP_UTF8, 0, src, opt_num_src_chars, dest.data(), int(dest.size()));
#else
    std::mbstate_t state = {};
    int num_dest_chars = opt_num_src_chars > 0 ? cc::min(int(dest.size()), opt_num_src_chars) : int(dest.size());
    return int(std::mbsrtowcs(dest.data(), &src, size_t(num_dest_chars), &state));
#endif
}
