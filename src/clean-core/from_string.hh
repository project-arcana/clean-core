#pragma once

#include <cstdint>

#include <clean-core/fwd.hh>

// converts strings to primitive types
// returns true only if the string was successfully parsed and entirely consumed

namespace cc
{
[[nodiscard]] bool from_string(char const* c_str, bool& out_value); // str: "true" or "false"
[[nodiscard]] bool from_string(char const* c_str, char& out_value);

[[nodiscard]] bool from_string(char const* c_str, signed char& out_value);
[[nodiscard]] bool from_string(char const* c_str, signed short& out_value);
[[nodiscard]] bool from_string(char const* c_str, signed int& out_value);
[[nodiscard]] bool from_string(char const* c_str, signed long& out_value);
[[nodiscard]] bool from_string(char const* c_str, signed long long& out_value);

[[nodiscard]] bool from_string(char const* c_str, unsigned char& out_value);
[[nodiscard]] bool from_string(char const* c_str, unsigned short& out_value);
[[nodiscard]] bool from_string(char const* c_str, unsigned int& out_value);
[[nodiscard]] bool from_string(char const* c_str, unsigned long& out_value);
[[nodiscard]] bool from_string(char const* c_str, unsigned long long& out_value);

[[nodiscard]] bool from_string(char const* c_str, float& out_value);
[[nodiscard]] bool from_string(char const* c_str, double& out_value);

[[nodiscard]] bool from_string(cc::string_view str, bool& out_value); // str: "true" or "false"
[[nodiscard]] bool from_string(cc::string_view str, char& out_value);

[[nodiscard]] bool from_string(cc::string_view str, signed char& out_value);
[[nodiscard]] bool from_string(cc::string_view str, signed short& out_value);
[[nodiscard]] bool from_string(cc::string_view str, signed int& out_value);
[[nodiscard]] bool from_string(cc::string_view str, signed long& out_value);
[[nodiscard]] bool from_string(cc::string_view str, signed long long& out_value);

[[nodiscard]] bool from_string(cc::string_view str, unsigned char& out_value);
[[nodiscard]] bool from_string(cc::string_view str, unsigned short& out_value);
[[nodiscard]] bool from_string(cc::string_view str, unsigned int& out_value);
[[nodiscard]] bool from_string(cc::string_view str, unsigned long& out_value);
[[nodiscard]] bool from_string(cc::string_view str, unsigned long long& out_value);

[[nodiscard]] bool from_string(cc::string_view str, float& out_value);
[[nodiscard]] bool from_string(cc::string_view str, double& out_value);

// 2 chars hex, e.g. "FF" or "1A"
[[nodiscard]] bool from_string(cc::string_view str, std::byte& out_value);
} // namespace cc
