#pragma once

#include <clean-core/forward.hh>

namespace cc
{
/// a function object that ignores all arguments
struct void_function
{
    template <class... Args>
    constexpr void operator()(Args&&...) const noexcept
    {
    }
};

/// a function object that always returns a given constant
template <auto C>
struct constant_function
{
    template <class... Args>
    constexpr auto operator()(Args&&...) const noexcept
    {
        return C;
    }
};

/// general-purpose identity function object
/// (preserves value category)
struct identity_function
{
    template <class T>
    constexpr T&& operator()(T&& v) const noexcept
    {
        return cc::forward<T>(v);
    }
};

/// always returns the I-th argument
/// (preserves value category)
template <int I>
struct projection_function
{
    template <class T, class... Rest>
    constexpr decltype(auto) operator()(T&&, Rest&&... args) const noexcept
    {
        static_assert(sizeof...(Rest) + 1 > I, "not enough arguments");
        return projection_function<I - 1>{}(cc::forward<Rest>(args)...);
    }
};
template <>
struct projection_function<0>
{
    template <class T, class... Rest>
    constexpr T&& operator()(T&& v, Rest&&...) const noexcept
    {
        return cc::forward<T>(v);
    }
};
}
