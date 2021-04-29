#pragma once

#include <clean-core/fwd.hh>

namespace cc
{
//
// integral types
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

string to_string(int8 value);
string to_string(int8 value, string_view fmt_str);
void to_string(string_stream_ref ss, int8 value);
void to_string(string_stream_ref ss, int8 value, string_view fmt_str);

string to_string(int16 value);
string to_string(int16 value, string_view fmt_str);
void to_string(string_stream_ref ss, int16 value);
void to_string(string_stream_ref ss, int16 value, string_view fmt_str);

string to_string(int32 value);
string to_string(int32 value, string_view fmt_str);
void to_string(string_stream_ref ss, int32 value);
void to_string(string_stream_ref ss, int32 value, string_view fmt_str);

string to_string(int64 value);
string to_string(int64 value, string_view fmt_str);
void to_string(string_stream_ref ss, int64 value);
void to_string(string_stream_ref ss, int64 value, string_view fmt_str);

string to_string(uint8 value);
string to_string(uint8 value, string_view fmt_str);
void to_string(string_stream_ref ss, uint8 value);
void to_string(string_stream_ref ss, uint8 value, string_view fmt_str);

string to_string(uint16 value);
string to_string(uint16 value, string_view fmt_str);
void to_string(string_stream_ref ss, uint16 value);
void to_string(string_stream_ref ss, uint16 value, string_view fmt_str);

string to_string(uint32 value);
string to_string(uint32 value, string_view fmt_str);
void to_string(string_stream_ref ss, uint32 value);
void to_string(string_stream_ref ss, uint32 value, string_view fmt_str);

string to_string(uint64 value);
string to_string(uint64 value, string_view fmt_str);
void to_string(string_stream_ref ss, uint64 value);
void to_string(string_stream_ref ss, uint64 value, string_view fmt_str);


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
