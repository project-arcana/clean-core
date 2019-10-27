#pragma once

#include <clean-core/has_operator.hh>

#include <clean-core/fwd.hh>

namespace cc
{
template <class T, class>
struct less
{
    static_assert(has_operator_less<T>, "if no bool-valued operator< is found, provide a specialization less<>");

    [[nodiscard]] bool operator()(T const& a, T const& b) const noexcept { return a < b; }
};

template <>
struct less<void> // transparent comparison
{
    template <class A, class B>
    [[nodiscard]] bool operator()(A const& a, B const& b) const noexcept
    {
        return a < b;
    }
};

}
