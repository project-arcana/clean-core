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
/// the generated assembly is equivalent: https://godbolt.org/z/vvEKno4jT
template <class SizedRange>
constexpr auto indices_of(SizedRange const& range)
{
    using T = std::decay_t<decltype(cc::collection_size(range))>;
    return cc::detail::irange<T>{T(0), cc::collection_size(range)};
}
}
