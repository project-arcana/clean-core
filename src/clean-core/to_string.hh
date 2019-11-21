#pragma once

#include <clean-core/string.hh>

namespace cc
{
inline string to_string(char value) { return string::filled(1, value); }
inline string to_string(bool value) { return value ? "true" : "false"; }
inline string to_string(char const* value) { return value; }
inline string to_string(string_view value) { return value; }
inline string to_string(nullptr_t) { return "nullptr"; }

string to_string(void* value);

string to_string(int value);
string to_string(long value);
string to_string(long long value);
string to_string(unsigned int value);
string to_string(unsigned long value);
string to_string(unsigned long long value);

string to_string(float value);
string to_string(double value);
string to_string(long double value);
}
