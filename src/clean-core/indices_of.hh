#pragma once

#include <clean-core/collection_traits.hh>
#include <clean-core/detail/srange.hh>

namespace cc
{
/// returns a stupid-simple-range that iterates over the indices of a sized range
/// usage:
///
///   cc::vector<int> v = ...;
///
///   for (auto i : cc::indices_of(v))
///       use(i);
///
///   // is the same as:
///
///   for (size_t i = 0; i != v.size(); ++i)
///       use(i);
///
///   // and the same as
///   for (auto i : cc::indices_of(v.size())(
///       use(i);
///
/// the generated assembly is equivalent for simple loops: https://godbolt.org/z/vvEKno4jT
/// and can be better when aliasing is involved: https://godbolt.org/z/Tv5ddjvaz
template <class SizedRangeOrIntegral>
constexpr auto indices_of(SizedRangeOrIntegral const& range)
{
    if constexpr (std::is_integral_v<SizedRangeOrIntegral>)
    {
        using T = SizedRangeOrIntegral;
        return cc::detail::irange<T>{T(0), range};
    }
    else
    {
        using T = std::decay_t<decltype(cc::collection_size(range))>;
        return cc::detail::irange<T>{T(0), cc::collection_size(range)};
    }
}
}
