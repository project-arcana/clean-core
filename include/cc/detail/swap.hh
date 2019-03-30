#pragma once

#include <cc/move>

namespace cc
{
template <class T>
constexpr void swap(T& a, T& b) noexcept
{
    auto tmp = cc::move(a);
    a = cc::move(b);
    b = cc::move(tmp);
}

template <class T, class U = T>
constexpr T exchange(T& obj, U&& new_value)
{
    T old = cc::move(obj);
    obj = cc::forward<U>(new_value);
    return old;
}
} // namespace cc
