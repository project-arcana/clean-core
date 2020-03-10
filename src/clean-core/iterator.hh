#pragma once

#include <type_traits> // std::is_array_v<T>, std::extent_v<T>

#include <clean-core/priority_tag.hh>

namespace cc::detail
{
template <class ContainerT>
constexpr auto begin_impl(ContainerT& c, cc::priority_tag<0>) -> decltype(begin(c))
{
    return begin(c);
}

template <class ContainerT>
constexpr auto begin_impl(ContainerT& c, cc::priority_tag<1>) -> decltype(c.begin())
{
    return c.begin();
}

template <class ContainerT>
constexpr auto end_impl(ContainerT& c, cc::priority_tag<0>) -> decltype(end(c))
{
    return end(c);
}

template <class ContainerT>
constexpr auto end_impl(ContainerT& c, cc::priority_tag<1>) -> decltype(c.end())
{
    return c.end();
}
}

namespace cc
{
template <class ContainerT>
constexpr auto begin(ContainerT& c)
{
    if constexpr (std::is_array_v<ContainerT>)
        return c;
    else
        return cc::detail::begin_impl(c, cc::priority_tag<1>{});
}
template <class ContainerT>
constexpr auto begin(ContainerT const& c)
{
    if constexpr (std::is_array_v<ContainerT>)
        return c;
    else
        return cc::detail::begin_impl(c, cc::priority_tag<1>{});
}

template <class ContainerT>
constexpr auto end(ContainerT& c)
{
    if constexpr (std::is_array_v<ContainerT>)
        return c + std::extent_v<ContainerT>;
    else
        return cc::detail::end_impl(c, cc::priority_tag<1>{});
}
template <class ContainerT>
constexpr auto end(ContainerT const& c)
{
    if constexpr (std::is_array_v<ContainerT>)
        return c + std::extent_v<ContainerT>;
    else
        return cc::detail::end_impl(c, cc::priority_tag<1>{});
}
}
