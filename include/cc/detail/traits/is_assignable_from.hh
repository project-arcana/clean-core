#pragma once

#include <cc/detail/traits/declval.hh>
#include <cc/detail/traits/true_false_type.hh>

namespace cc
{
namespace detail
{
template <class A, class B>
[[maybe_unused]] static auto test_assignment(int) -> decltype(declval<A&>() = declval<B>(), true_type());
template <class A, class B>
[[maybe_unused]] static auto test_assignment(...) -> false_type;
} // namespace detail

// is_assignable_from<A, B> iff assignment a = b is callable
template <class A, class B>
inline constexpr bool is_assignable_from = decltype(detail::test_assignment<A, B>(0))::value;
} // namespace cc
