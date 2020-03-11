#pragma once

#include <clean-core/has_operator.hh>

#include <clean-core/fwd.hh>

namespace cc
{
template <class T, class>
struct equal_to
{
    static_assert(has_operator_equal<T>, "if no bool-valued operator< is found, provide a specialization equal_to<>");

    [[nodiscard]] constexpr bool operator()(T const& a, T const& b) const noexcept { return a == b; }
};

template <>
struct equal_to<void> // transparent comparison
{
    template <class A, class B>
    [[nodiscard]] constexpr bool operator()(A const& a, B const& b) const noexcept
    {
        return a == b;
    }
};

}
