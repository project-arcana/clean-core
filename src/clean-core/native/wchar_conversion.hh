#pragma once

#include <clean-core/span.hh>

namespace cc
{
/// converts a wchar_t string to a (UTF-8) char string
/// opt_num_src_chars can be specified to stop conversion before '\0' terminateion
/// returns amount of characters written to dest
int widechar_to_char(char* dest, int max_num_dest_chars, wchar_t const* src, int opt_num_src_chars = -1);

inline int widechar_to_char(cc::span<char> dest, wchar_t const* src, int opt_num_src_chars = -1)
{
    return widechar_to_char(dest.data(), int(dest.size()), src, opt_num_src_chars);
}

/// converts a (UTF-8) char string to a wchar_t string
/// opt_num_src_chars can be specified to stop conversion before '\0' terminateion
/// returns amount of characters written to dest
int char_to_widechar(wchar_t* dest, int max_num_dest_chars, char const* src, int opt_num_src_chars = -1);

inline int char_to_widechar(cc::span<wchar_t> dest, char const* src, int opt_num_src_chars = -1)
{
    return char_to_widechar(dest.data(), int(dest.size()), src, opt_num_src_chars);
}
}
