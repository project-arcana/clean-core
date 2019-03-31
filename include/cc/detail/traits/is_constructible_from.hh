#pragma once

#include <cc/detail/traits/declval.hh>
#include <cc/detail/traits/true_false_type.hh>

namespace cc
{
namespace detail
{
template <class A, class B>
[[maybe_unused]] static auto test_ctor(int) -> decltype(A(declval<B>()), true_type());
template <class A, class B>
[[maybe_unused]] static auto test_ctor(...) -> false_type;

template <class T>
[[maybe_unused]] static auto test_def_ctor(int) -> decltype(T(), true_type());
template <class T>
[[maybe_unused]] static auto test_def_ctor(...) -> false_type;
} // namespace detail

// is_constructible_from<A, B> iff constructor A(B) is callable
template <class A, class B>
inline constexpr bool is_constructible_from = decltype(detail::test_ctor<A, B>(0))::value;

// is_default_constructible<T> iff constructor T() is callable
template <class T>
inline constexpr bool is_default_constructible = decltype(detail::test_def_ctor<T>(0))::value;
} // namespace cc
