#pragma once

namespace cc
{
namespace detail
{
template <class A, class B>
struct is_same_t
{
    static constexpr bool value = false;
};
template <class T>
struct is_same_t<T, T>
{
    static constexpr bool value = true;
};
} // namespace detail

template <class A, class B>
inline constexpr bool is_same = detail::is_same_t<A, B>::value;
} // namespace cc
