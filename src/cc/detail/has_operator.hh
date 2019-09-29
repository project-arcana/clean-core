#pragma once

#include <type_traits>
#include <utility>

namespace cc
{
namespace detail
{
template <class A, class B>
[[maybe_unused]] static auto test_op_less(int) -> decltype(bool(std::declval<A>() < std::declval<B>()), std::true_type{});
template <class A, class B>
[[maybe_unused]] static auto test_op_less(...) -> std::false_type;
} // namespace detail

template <class T>
constexpr bool has_operator_less = decltype(detail::test_op_less<T const&, T const&>(0))::value;
}
