#pragma once

namespace cc
{
/// Helper class for indicating errors in static_asserts
/// Usage:
///   static_assert(cc::always_false<T>, "some error");
template <class... E>
constexpr bool always_false = false;
}
