#pragma once

#include <clean-core/string_view.hh>

/**
 * converts strings to primitive types
 *
 * returns true if the parsing was successful AND the whole string was consumed
 */

namespace cc
{
bool from_string(cc::string_view s, signed char& v);
bool from_string(cc::string_view s, signed short& v);
bool from_string(cc::string_view s, signed int& v);
bool from_string(cc::string_view s, signed long& v);
bool from_string(cc::string_view s, signed long long& v);

bool from_string(cc::string_view s, unsigned char& v);
bool from_string(cc::string_view s, unsigned short& v);
bool from_string(cc::string_view s, unsigned int& v);
bool from_string(cc::string_view s, unsigned long& v);
bool from_string(cc::string_view s, unsigned long long& v);

bool from_string(cc::string_view s, float& v);
bool from_string(cc::string_view s, double& v);
}
