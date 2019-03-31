#pragma once

#include <cc/detail/traits/declval.hh>
#include <cc/detail/traits/true_false_type.hh>

namespace cc
{
namespace detail
{
template <class A, class B>
[[maybe_unused]] static auto test_op_less(int) -> decltype(bool(declval<A>() < declval<B>()), true_type{});
template <class A, class B>
[[maybe_unused]] static auto test_op_less(...) -> false_type;
} // namespace detail

template <class T>
inline constexpr bool has_operator_less = decltype(detail::test_op_less<T const&, T const&>(0))::value;
} // namespace cc
