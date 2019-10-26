#pragma once

#include <clean-core/assert.hh>
#include <clean-core/cursor.hh>

namespace cc
{
template <class RangeFrom, class RangeTo>
constexpr void copy(RangeFrom const& from, RangeTo& to)
{
    auto c_from = cc::to_cursor(from);
    auto c_to = cc::to_cursor(to);
    while (c_from)
    {
        CC_CONTRACT(c_to);
        *c_to = *c_from;
        ++c_to, ++c_from;
    }
}

template <class Range, class T>
constexpr void fill(Range& range, T const& value)
{
    auto c = cc::to_cursor(range);
    while (c)
    {
        *c = value;
        ++c;
    }
}

template <class RangeA, class RangeB>
[[nodiscard]] constexpr bool are_ranges_equal(RangeA const& a, RangeB const& b)
{
    if (a.size() != b.size())
        return false;

    auto ca = cc::to_cursor(a);
    auto cb = cc::to_cursor(b);

    while (ca)
    {
        if (*ca != *cb)
            return false;
        ++ca, ++cb;
    }

    return true;
}

template <class RangeA, class RangeB>
[[nodiscard]] constexpr bool are_ranges_unequal(RangeA const& a, RangeB const& b)
{
    if (a.size() != b.size())
        return true;

    auto ca = cc::to_cursor(a);
    auto cb = cc::to_cursor(b);

    while (ca)
    {
        if (*ca != *cb)
            return true;
        ++ca, ++cb;
    }

    return false;
}
}
