#pragma once

// locale independent character predicates on 'char's
// see https://en.cppreference.com/w/cpp/string/byte

namespace cc
{
[[nodiscard]] constexpr bool is_space(char c) { return c == ' ' || c == '\f' || c == '\t' || c == '\n' || c == '\r' || c == '\v'; }
[[nodiscard]] constexpr bool is_blank(char c) { return c == ' ' || c == '\t'; }
[[nodiscard]] constexpr bool is_digit(char c) { return '0' <= c && c <= '9'; }
[[nodiscard]] constexpr bool is_hex_digit(char c) { return ('0' <= c && c <= '9') || ('a' <= c && c <= 'f') || ('A' <= c && c <= 'F'); }
[[nodiscard]] constexpr bool is_alphanumeric(char c) { return ('0' <= c && c <= '9') || ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z'); }
[[nodiscard]] constexpr bool is_lower(char c) { return 'a' <= c && c <= 'z'; }
[[nodiscard]] constexpr bool is_upper(char c) { return 'A' <= c && c <= 'Z'; }
[[nodiscard]] constexpr bool is_punctuation(char c)
{
    return ('\x21' <= c && c <= '\x2F') || ('\x3A' <= c && c <= '\x40') || ('\x5B' <= c && c <= '\x60') || ('\x7B' <= c && c <= '\x7E');
}
[[nodiscard]] constexpr bool is_graphical(char c) { return is_alphanumeric(c) || is_punctuation(c); }
[[nodiscard]] constexpr bool is_printable(char c) { return c == ' ' || is_alphanumeric(c) || is_punctuation(c); }
[[nodiscard]] constexpr bool is_control(char c) { return ('\x00' <= c && c <= '\x1F') || c == '\x7F'; }

[[nodiscard]] constexpr char to_lower(char c) { return is_upper(c) ? char('a' + (c - 'A')) : c; }
[[nodiscard]] constexpr char to_upper(char c) { return is_lower(c) ? char('A' + (c - 'a')) : c; }
}
