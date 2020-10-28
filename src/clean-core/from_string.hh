#pragma once

#include <clean-core/string_view.hh>

/**
 * converts strings to primitive types
 *
 * returns true if the parsing was successful AND the whole string was consumed
 */

namespace cc
{
[[nodiscard]] bool from_string(cc::string_view s, bool& v); // "true" or "false"
[[nodiscard]] bool from_string(cc::string_view s, char& v);

[[nodiscard]] bool from_string(cc::string_view s, signed char& v);
[[nodiscard]] bool from_string(cc::string_view s, signed short& v);
[[nodiscard]] bool from_string(cc::string_view s, signed int& v);
[[nodiscard]] bool from_string(cc::string_view s, signed long& v);
[[nodiscard]] bool from_string(cc::string_view s, signed long long& v);

[[nodiscard]] bool from_string(cc::string_view s, unsigned char& v);
[[nodiscard]] bool from_string(cc::string_view s, unsigned short& v);
[[nodiscard]] bool from_string(cc::string_view s, unsigned int& v);
[[nodiscard]] bool from_string(cc::string_view s, unsigned long& v);
[[nodiscard]] bool from_string(cc::string_view s, unsigned long long& v);

[[nodiscard]] bool from_string(cc::string_view s, float& v);
[[nodiscard]] bool from_string(cc::string_view s, double& v);
}
