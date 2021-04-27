#pragma once

#include <clean-core/fwd.hh>

// converts strings to primitive types
// returns true on success

namespace cc
{
[[nodiscard]] bool from_string(cc::string_view str, bool& out_value); // str: "true" or "false"
[[nodiscard]] bool from_string(cc::string_view str, char& out_value);

[[nodiscard]] bool from_string(char const* c_str, int8& out_value);
[[nodiscard]] bool from_string(char const* c_str, int16& out_value);
[[nodiscard]] bool from_string(char const* c_str, int32& out_value);
[[nodiscard]] bool from_string(char const* c_str, int64& out_value);

[[nodiscard]] bool from_string(char const* c_str, uint8& out_value);
[[nodiscard]] bool from_string(char const* c_str, uint16& out_value);
[[nodiscard]] bool from_string(char const* c_str, uint32& out_value);
[[nodiscard]] bool from_string(char const* c_str, uint64& out_value);

[[nodiscard]] bool from_string(char const* c_str, float& out_value);
[[nodiscard]] bool from_string(char const* c_str, double& out_value);

[[nodiscard]] bool from_string(cc::string_view str, int8& out_value);
[[nodiscard]] bool from_string(cc::string_view str, int16& out_value);
[[nodiscard]] bool from_string(cc::string_view str, int32& out_value);
[[nodiscard]] bool from_string(cc::string_view str, int64& out_value);

[[nodiscard]] bool from_string(cc::string_view str, uint8& out_value);
[[nodiscard]] bool from_string(cc::string_view str, uint16& out_value);
[[nodiscard]] bool from_string(cc::string_view str, uint32& out_value);
[[nodiscard]] bool from_string(cc::string_view str, uint64& out_value);

[[nodiscard]] bool from_string(cc::string_view str, float& out_value);
[[nodiscard]] bool from_string(cc::string_view str, double& out_value);
}
