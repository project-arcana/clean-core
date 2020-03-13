#pragma once

#include <cstddef>
#include <utility>

#include <clean-core/priority_tag.hh>

namespace cc
{
namespace detail
{
template <size_t I, class T>
constexpr auto impl_get(T&& v, cc::priority_tag<1>) -> decltype(v.template get<I>())
{
    return v.template get<I>();
}
template <size_t I, class T>
constexpr auto impl_get(T&& v, cc::priority_tag<0>) -> decltype(std::get<I>(v))
{
    return std::get<I>(v);
}
}

template <size_t I, class T>
[[nodiscard]] constexpr decltype(auto) get(T&& v)
{
    return cc::detail::impl_get<I, T>(v, cc::priority_tag<1>{});
}
}
