#pragma once

namespace cc
{
/// Helper class for indicating errors in static_asserts
/// Usage:
///   static_assert(cc::always_false<T>, "some error");
template <class... E>
constexpr bool always_false = false;

/// same as always_false, but for non-type template parameters, e.g. always_false_v<Dimension>
template <auto... E>
constexpr bool always_false_v = false;
}
