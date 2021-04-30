#pragma once

#include <cstddef>

#include <clean-core/fwd.hh>

namespace cc
{
//
// integral types
// these intentionally use int / long / long long instead of sized types
//

string to_string(bool value);
string to_string(bool value, string_view fmt_str);
void to_string(string_stream_ref ss, bool value);
void to_string(string_stream_ref ss, bool value, string_view fmt_str);

string to_string(char value);
string to_string(char value, string_view fmt_str);
void to_string(string_stream_ref ss, char value);
void to_string(string_stream_ref ss, char value, string_view fmt_str);

string to_string(std::byte value);
string to_string(std::byte value, string_view fmt_str);
void to_string(string_stream_ref ss, std::byte value);
void to_string(string_stream_ref ss, std::byte value, string_view fmt_str);

string to_string(signed char value);
string to_string(signed char value, string_view fmt_str);
void to_string(string_stream_ref ss, signed char value);
void to_string(string_stream_ref ss, signed char value, string_view fmt_str);

string to_string(signed short value);
string to_string(signed short value, string_view fmt_str);
void to_string(string_stream_ref ss, signed short value);
void to_string(string_stream_ref ss, signed short value, string_view fmt_str);

string to_string(signed int value);
string to_string(signed int value, string_view fmt_str);
void to_string(string_stream_ref ss, signed int value);
void to_string(string_stream_ref ss, signed int value, string_view fmt_str);

string to_string(signed long value);
string to_string(signed long value, string_view fmt_str);
void to_string(string_stream_ref ss, signed long value);
void to_string(string_stream_ref ss, signed long value, string_view fmt_str);

string to_string(signed long long value);
string to_string(signed long long value, string_view fmt_str);
void to_string(string_stream_ref ss, signed long long value);
void to_string(string_stream_ref ss, signed long long value, string_view fmt_str);

string to_string(unsigned char value);
string to_string(unsigned char value, string_view fmt_str);
void to_string(string_stream_ref ss, unsigned char value);
void to_string(string_stream_ref ss, unsigned char value, string_view fmt_str);

string to_string(unsigned short value);
string to_string(unsigned short value, string_view fmt_str);
void to_string(string_stream_ref ss, unsigned short value);
void to_string(string_stream_ref ss, unsigned short value, string_view fmt_str);

string to_string(unsigned int value);
string to_string(unsigned int value, string_view fmt_str);
void to_string(string_stream_ref ss, unsigned int value);
void to_string(string_stream_ref ss, unsigned int value, string_view fmt_str);

string to_string(unsigned long value);
string to_string(unsigned long value, string_view fmt_str);
void to_string(string_stream_ref ss, unsigned long value);
void to_string(string_stream_ref ss, unsigned long value, string_view fmt_str);

string to_string(unsigned long long value);
string to_string(unsigned long long value, string_view fmt_str);
void to_string(string_stream_ref ss, unsigned long long value);
void to_string(string_stream_ref ss, unsigned long long value, string_view fmt_str);


//
// floating types
//

string to_string(float value);
string to_string(float value, string_view fmt_str);
void to_string(string_stream_ref ss, float value);
void to_string(string_stream_ref ss, float value, string_view fmt_str);

string to_string(double value);
string to_string(double value, string_view fmt_str);
void to_string(string_stream_ref ss, double value);
void to_string(string_stream_ref ss, double value, string_view fmt_str);

string to_string(long double value);
string to_string(long double value, string_view fmt_str);
void to_string(string_stream_ref ss, long double value);
void to_string(string_stream_ref ss, long double value, string_view fmt_str);


//
// pointer types
//

string to_string(std::nullptr_t);
string to_string(std::nullptr_t, string_view fmt_str);
void to_string(string_stream_ref ss, std::nullptr_t);
void to_string(string_stream_ref ss, std::nullptr_t, string_view fmt_str);

string to_string(void* value);
string to_string(void* value, string_view fmt_str);
void to_string(string_stream_ref ss, void* value);
void to_string(string_stream_ref ss, void* value, string_view fmt_str);

string to_string(void const* value);
string to_string(void const* value, string_view fmt_str);
void to_string(string_stream_ref ss, void const* value);
void to_string(string_stream_ref ss, void const* value, string_view fmt_str);


//
// string types
//

string to_string(char const* value);
string to_string(char const* value, string_view fmt_str);
void to_string(string_stream_ref ss, char const* value);
void to_string(string_stream_ref ss, char const* value, string_view fmt_str);

string to_string(wchar_t const* value);
string to_string(wchar_t const* value, string_view fmt_str);
void to_string(string_stream_ref ss, wchar_t const* value);
void to_string(string_stream_ref ss, wchar_t const* value, string_view fmt_str);

string to_string(string_view value);
string to_string(string_view value, string_view fmt_str);
void to_string(string_stream_ref ss, string_view value);
void to_string(string_stream_ref ss, string_view value, string_view fmt_str);
}
