#pragma once

#include <clean-core/string.hh>
#include <clean-core/string_stream.hh>

namespace cc
{
inline string to_string(char value) { return string::filled(1, value); }
inline string to_string(bool value) { return value ? "true" : "false"; }
inline string to_string(char const* value) { return value == nullptr ? "[nullptr]" : value; }
inline string to_string(string_view value) { return value; }
inline string to_string(nullptr_t) { return "nullptr"; }

string to_string(void* value);

string to_string(std::byte value);

string to_string(int value);
string to_string(long value);
string to_string(long long value);
string to_string(unsigned int value);
string to_string(unsigned long value);
string to_string(unsigned long long value);

string to_string(float value);
string to_string(double value);
string to_string(long double value);

// void to_string(string_stream& ss, char value, string_view fmt_str);
// void to_string(string_stream& ss, bool value, string_view fmt_str);
// void to_string(string_stream& ss, char const* value, string_view fmt_str);
// void to_string(string_stream& ss, string_view value, string_view fmt_str);
// void to_string(string_stream& ss, nullptr_t, string_view fmt_str);

// void to_string(string_stream& ss, void* value, string_view fmt_str);

// void to_string(string_stream& ss, std::byte value, string_view fmt_str);

// void to_string(string_stream& ss, int value, string_view fmt_str);
// void to_string(string_stream& ss, long value, string_view fmt_str);
// void to_string(string_stream& ss, long long value, string_view fmt_str);
// void to_string(string_stream& ss, unsigned int value, string_view fmt_str);
// void to_string(string_stream& ss, unsigned long value, string_view fmt_str);
// void to_string(string_stream& ss, unsigned long long value, string_view fmt_str);

// void to_string(string_stream& ss, float value, string_view fmt_str);
// void to_string(string_stream& ss, double value, string_view fmt_str);
// void to_string(string_stream& ss, long double value, string_view fmt_str);

}
