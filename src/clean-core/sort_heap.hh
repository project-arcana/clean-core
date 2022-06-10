#pragma once

#include <cstdint>

#include <clean-core/collection_traits.hh>
#include <clean-core/functors.hh>
#include <clean-core/invoke.hh>
#include <clean-core/utility.hh>

namespace cc
{
/// Generic customizable function that turns a range into a max heap
/// This function works on a "virtual" range from "start..start+size-1"
/// @param get : (int64_t idx) -> T, provides the elements to compare
/// @param compare : (T const& a, T const& b) -> bool, is true if "a < b", i.e. b should be "above" a in the heap
/// @param swap : (int64_t a, int64_t b) -> void, swaps the virtual elements at a and b
///
/// Requirements:
/// - compare must be a total order, i.e. irreflexive, transitive, antisymmetric, total
///
template <class GetF, class CompareF, class SwapF>
constexpr void make_heap_ex(int64_t start, size_t size, GetF&& get, CompareF&& compare, SwapF&& swap);

/// inserts the element at "start + size - 1" into the max heap from "start..start+size-2"
/// for functor description, see make_heap_ex
template <class GetF, class CompareF, class SwapF>
constexpr void push_heap_ex(int64_t start, size_t size, GetF&& get, CompareF&& compare, SwapF&& swap);

/// moves the value at "start" to "start + size - 1" and restores max heap property for the rest
/// for functor description, see make_heap_ex
template <class GetF, class CompareF, class SwapF>
constexpr void pop_heap_ex(int64_t start, size_t size, GetF&& get, CompareF&& compare, SwapF&& swap);

/// converts the max heap from "start" to "start + size - 1" into a sorted range of ascending order
/// for functor description, see make_heap_ex
/// NOTE: this corresponds to std::sort_heap
///       a different name was chosen to reduce surprising behavior
///       in particular, std::sort_heap(first, last) does NOT perform a (full) heap sort
template <class GetF, class CompareF, class SwapF>
constexpr void unmake_heap_ex(int64_t start, size_t size, GetF&& get, CompareF&& compare, SwapF&& swap);

//
// Implementation
//

template <class GetF, class CompareF, class SwapF>
constexpr void make_heap_ex(int64_t start, size_t size, GetF&& get, CompareF&& compare, SwapF&& swap)
{
    for (size_t s = 1; s <= size; ++s)
        cc::push_heap_ex(start, s, get, compare, swap);
}

template <class GetF, class CompareF, class SwapF>
constexpr void push_heap_ex(int64_t start, size_t size, GetF&& get, CompareF&& compare, SwapF&& swap)
{
    if (size <= 1)
        return;

    auto i = int64_t(size - 1);
    auto curr_i = start + i;
    while (i > 0)
    {
        i = (i - 1) >> 1;
        auto parent_i = start + i;

        if (!cc::invoke(compare, cc::invoke(get, parent_i), cc::invoke(get, curr_i)))
            break;

        cc::invoke(swap, parent_i, curr_i);
        curr_i = parent_i;
    }
}

template <class GetF, class CompareF, class SwapF>
constexpr void pop_heap_ex(int64_t start, size_t size, GetF&& get, CompareF&& compare, SwapF&& swap)
{
    if (size <= 1)
        return;

    cc::invoke(swap, start, start + int64_t(size) - 1);

    size_t i = 0;
    auto max_i = (size - 1) >> 1;
    auto max_c = start + int64_t(size) - 2;
    while (i < max_i)
    {
        auto c0 = start + int64_t(i << 1) + 1;
        auto c1 = c0 + 1;

        auto&& e_i = cc::invoke(get, i);
        auto&& e_c0 = cc::invoke(get, c0);

        if (cc::invoke(compare, e_i, e_c0))
        {
            // only one child OR c1 is smaller than c0
            if (c1 > max_c || cc::invoke(compare, cc::invoke(get, c1), e_c0))
            {
                cc::invoke(swap, i, c0);
                i = c0;
            }
            else
            {
                cc::invoke(swap, i, c1);
                i = c1;
            }
        }
        else if (c1 <= max_c && cc::invoke(compare, e_i, cc::invoke(get, c1)))
        {
            cc::invoke(swap, i, c1);
            i = c1;
        }
        else
            break; // correct pos
    }
}

template <class GetF, class CompareF, class SwapF>
constexpr void unmake_heap_ex(int64_t start, size_t size, GetF&& get, CompareF&& compare, SwapF&& swap)
{
    while (size > 1)
        cc::pop_heap_ex(start, size--, get, compare, swap);
}

}
