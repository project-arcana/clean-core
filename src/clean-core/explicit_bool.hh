#pragma once

namespace cc
{
/// A strongly-typed boolean that does not implicitly decay to int
struct explicit_bool
{
    constexpr explicit_bool() = default;
    constexpr explicit_bool(bool v) : _value(v) {}

    constexpr explicit operator bool() const { return _value; }

    constexpr explicit_bool operator!() const { return !_value; }
    constexpr bool operator==(explicit_bool b) const { return _value == b._value; }
    constexpr bool operator!=(explicit_bool b) const { return _value != b._value; }

private:
    bool _value = false;
};
}
