#pragma once

namespace cc
{
namespace detail
{
template <bool B>
struct enable_if_t
{
};
template <>
struct enable_if_t<true>
{
    using type = bool;
};
}

/// Example:
///   template <class T, cc::enable_if<some_cond<T>> = true>
///   void foo(T t);
template <bool Condition>
using enable_if = typename detail::enable_if_t<Condition>::type;
}
