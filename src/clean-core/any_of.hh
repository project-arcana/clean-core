#pragma once

#include <clean-core/is_range.hh>
#include <clean-core/move.hh>

namespace cc
{
/// NOTE: this class should not be stored!
template <class F>
struct any_of_t
{
    any_of_t(F f) : test(cc::move(f)) {}

    F test;
};

template <class T, class F>
bool operator==(T const& lhs, any_of_t<F> const& rhs)
{
    return rhs.test(lhs);
}
template <class T, class F>
bool operator!=(T const& lhs, any_of_t<F> const& rhs)
{
    return !rhs.test(lhs);
}

template <class Range, class = std::enable_if_t<is_any_range<Range>>>
auto any_of(Range const& r)
{
    return any_of_t([&r](auto const& lhs) {
        for (auto const& rhs : r)
            if (lhs == rhs)
                return true;
        return false;
    });
}

template <class A, class B, class... Rest>
auto any_of(A const& a, B const& b, Rest const&... rest)
{
    return any_of_t([&](auto const& lhs) {
        if (lhs == a)
            return true;
        if (lhs == b)
            return true;
        return (... || (lhs == rest));
    });
}
}
