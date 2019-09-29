#pragma once

#include <cc/detail/has_operator.hh>

namespace cc
{
template <class T>
struct less
{
    static_assert(has_operator_less<T>, "if no bool-valued operator< is found, provide a specialization less<>");

    [[nodiscard]] bool operator()(T const& a, T const& b) const noexcept { return a < b; }
};

}
