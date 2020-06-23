#include "wchar_conversion.hh"

#include <cwchar>

#include <clean-core/macros.hh>
#include <clean-core/utility.hh>

#include <clean-core/native/win32_sanitized.hh>

static_assert(sizeof(char) == 1 && sizeof(wchar_t) == 2, "Unsupported environment");

int cc::widechar_to_char(char* dest, int max_num_dest_chars, const wchar_t* src, int opt_num_src_chars)
{
#ifdef CC_OS_WINDOWS
    return ::WideCharToMultiByte(CP_UTF8, 0, src, opt_num_src_chars, dest, max_num_dest_chars, nullptr, nullptr);
#else
    std::mbstate_t state = {};
    unsigned num_dest_chars = opt_num_src_chars > 0 ? cc::min(max_num_dest_chars, opt_num_src_chars / 2) : max_num_dest_chars;
    return std::wcsrtombs(dest, &src, num_dest_chars, &state);
#endif
}

int cc::char_to_widechar(wchar_t* dest, int max_num_dest_chars, const char* src, int opt_num_src_chars)
{
#ifdef CC_OS_WINDOWS
    return ::MultiByteToWideChar(CP_UTF8, 0, src, opt_num_src_chars, dest, max_num_dest_chars);
#else
    std::mbstate_t state = {};
    unsigned num_dest_chars = opt_num_src_chars > 0 ? cc::min(max_num_dest_chars, opt_num_src_chars) : max_num_dest_chars;
    return std::mbsrtowcs(dest, &src, num_dest_chars, &state);
#endif
}
