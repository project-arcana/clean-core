#pragma once

#include <cstdint>

#include <clean-core/fwd.hh>

// converts strings to primitive types
// returns true only if the string was successfully parsed and entirely consumed

namespace cc
{
[[nodiscard]] bool from_string(cc::string_view str, bool& out_value); // str: "true" or "false"
[[nodiscard]] bool from_string(cc::string_view str, char& out_value);

[[nodiscard]] bool from_string(char const* c_str, int8_t& out_value);
[[nodiscard]] bool from_string(char const* c_str, int16_t& out_value);
[[nodiscard]] bool from_string(char const* c_str, int32_t& out_value);
[[nodiscard]] bool from_string(char const* c_str, int64_t& out_value);

[[nodiscard]] bool from_string(char const* c_str, uint8_t& out_value);
[[nodiscard]] bool from_string(char const* c_str, uint16_t& out_value);
[[nodiscard]] bool from_string(char const* c_str, uint32_t& out_value);
[[nodiscard]] bool from_string(char const* c_str, uint64_t& out_value);

[[nodiscard]] bool from_string(char const* c_str, float& out_value);
[[nodiscard]] bool from_string(char const* c_str, double& out_value);

[[nodiscard]] bool from_string(cc::string_view str, int8_t& out_value);
[[nodiscard]] bool from_string(cc::string_view str, int16_t& out_value);
[[nodiscard]] bool from_string(cc::string_view str, int32_t& out_value);
[[nodiscard]] bool from_string(cc::string_view str, int64_t& out_value);

[[nodiscard]] bool from_string(cc::string_view str, uint8_t& out_value);
[[nodiscard]] bool from_string(cc::string_view str, uint16_t& out_value);
[[nodiscard]] bool from_string(cc::string_view str, uint32_t& out_value);
[[nodiscard]] bool from_string(cc::string_view str, uint64_t& out_value);

[[nodiscard]] bool from_string(cc::string_view str, float& out_value);
[[nodiscard]] bool from_string(cc::string_view str, double& out_value);

// 2 chars hex, e.g. "FF" or "1A"
[[nodiscard]] bool from_string(cc::string_view str, std::byte& out_value);
}
