#pragma once

#include <cstdint>

#include <clean-core/collection_traits.hh>
#include <clean-core/functors.hh>
#include <clean-core/invoke.hh>
#include <clean-core/less.hh>
#include <clean-core/utility.hh>

// inspired by https://github.com/orlp/pdqsort
/* pdqsort license (zlib)
 *  Copyright (c) 2021 Orson Peters
 *  This software is provided 'as-is', without any express or implied warranty. In no event will the
 *  authors be held liable for any damages arising from the use of this software.
 *  Permission is granted to anyone to use this software for any purpose, including commercial
 *  applications, and to alter it and redistribute it freely, subject to the following restrictions:
 *  1. The origin of this software must not be misrepresented; you must not claim that you wrote the
 *     original software. If you use this software in a product, an acknowledgment in the product
 *     documentation would be appreciated but is not required.
 *  2. Altered source versions must be plainly marked as such, and must not be misrepresented as
 *     being the original software.
 *  3. This notice may not be removed or altered from any source distribution.
 */

namespace cc
{
/**
 * Generic customizable sort function. Basis for all convenience wrappers based on sort
 * This function works on a "virtual" range from "start..start+size-1"
 * @param get : (int64_t idx) -> T, provides the elements to compare
 * @param compare : (T const& a, T const& b) -> bool, is true if "a < b", i.e. a should be before b
 * @param swap : (int64_t a, int64_t b) -> void, swaps the virtual elements at a and b
 * @param select : (int64_t start, size_t size) -> bool, subrange is sorted only if true
 *
 * Requirements:
 * - compare must be a total order, i.e. irreflexive, transitive, antisymmetric, total
 *
 * TODO:
 * - insertion_sort is typically move-based, not swap-based, so we cannot use it here
 * - other steps of pdqsort also depend on local tmp copies that cannot be used
 * - maybe a special swap location? prob better as sep arg
 */
template <class GetF, class CompareF, class SwapF, class SelectF>
void sort_ex(int64_t start, size_t size, GetF&& get, CompareF&& compare, SwapF&& swap, SelectF&& select);

// TODO: move lambdas out into detail namespace
//       so they don't get generated multiple times (with diff compare funs)
template <class Collection, class CompareF = cc::less<void>>
void sort(Collection&& collection, CompareF&& compare = {})
{
    static_assert(collection_traits<Collection>::is_range);
    size_t size = cc::collection_size(collection);
    cc::sort_ex(
        0, size,                                                                         //
        [&collection](int64_t i) { return collection[i]; },                              //
        compare,                                                                         //
        [&collection](int64_t a, int64_t b) { cc::swap(collection[a], collection[b]); }, //
        cc::constant_function<true>{});
}

template <class Collection, class KeyF, class CompareF = cc::less<void>>
void sort_by(Collection&& collection, KeyF&& key, CompareF&& compare = {})
{
    static_assert(collection_traits<Collection>::is_range);
    size_t size = cc::collection_size(collection);
    cc::sort_ex(
        0, size,                                                                         //
        [&collection, &key](int64_t i) { return cc::invoke(key, collection[i]); },       //
        compare,                                                                         //
        [&collection](int64_t a, int64_t b) { cc::swap(collection[a], collection[b]); }, //
        cc::constant_function<true>{});
}


namespace detail
{
enum
{
    // use optimized small sort if not above this size
    small_sort_threshold = 16,

    // use Tukey's ninther above this size
    ninther_threshold = 128,
};

template <class GetF, class CompareF, class SwapF>
void sort2(int64_t ia, int64_t ib, GetF& get, CompareF& compare, SwapF& swap)
{
    auto&& a = cc::invoke(get, ia);
    auto&& b = cc::invoke(get, ib);
    if (cc::invoke(compare, b, a))
        cc::invoke(swap, ia, ib);
}

template <class GetF, class CompareF, class SwapF>
void sort3(int64_t ia, int64_t ib, int64_t ic, GetF& get, CompareF& compare, SwapF& swap)
{
    // TODO: check what is better
    detail::sort2(ia, ib, get, compare, swap);
    detail::sort2(ib, ic, get, compare, swap);
    detail::sort2(ia, ib, get, compare, swap);

    // auto&& a = cc::invoke(get, ia);
    // auto&& b = cc::invoke(get, ib);
    // auto&& c = cc::invoke(get, ic);
    // auto a_smaller_b = cc::invoke(compare, b, a);
    // auto a_smaller_c = cc::invoke(compare, c, a);
    // auto b_smaller_c = cc::invoke(compare, c, b);
}

// NOTE: we take by normal ref so it's not instantiated multiple times
template <class GetF, class CompareF, class SwapF>
void small_sort(int64_t start, int64_t size, GetF& get, CompareF& compare, SwapF& swap)
{
    if (size <= 1)
        return;

    if (size == 2)
    {
        auto&& e0 = cc::invoke(get, start + 0);
        auto&& e1 = cc::invoke(get, start + 1);
        if (!cc::invoke(compare, e0, e1))
            cc::invoke(swap, start + 0, start + 1);
        return;
    }

    // TODO: other explicit small cases?

    // TODO: calls get quite often, might be better with a diff algo?
    auto const end = start + size;
    for (int64_t i = start + 1; i < end; ++i)
    {
        auto j = i;
        while (j > start && !cc::invoke(compare, cc::invoke(get, j - 1), cc::invoke(get, j)))
        {
            cc::invoke(swap, j - 1, j);
            j--;
        }
    }
}

// partitions the range around the pivot
// equal elements are moved to the right of the pivot
// pivot is initially at start
// returns position of pivot
template <class GetF, class CompareF, class SwapF>
int64_t partition_right(bool& was_already_partitioned, int64_t start, int64_t size, GetF&& get, CompareF&& compare, SwapF&& swap)
{
    // TODO: move pivot into some local config?
    auto&& pivot = cc::invoke(get, start);

    int64_t first = start;
    int64_t last = start + size;

    // find an element >= pivot (exists, because pivot is at least median of 3)
    while (cc::invoke(compare, cc::invoke(get, ++first), pivot))
    {
        // empty
    }

    // find first strictly smaller than pivot
    // guard is only needed if no element before
    if (first - 1 == start)
        while (first < last && !cc::invoke(compare, cc::invoke(get, --last), pivot))
        {
            // empty
        }
    else
        while (!cc::invoke(compare, cc::invoke(get, --last), pivot))
        {
            // empty
        }

    // partition already correct
    was_already_partitioned = first >= last;

    // keep swapping until partitioned
    // no guards needed due to above iteration
    while (first < last)
    {
        cc::invoke(swap, first, last);
        while (cc::invoke(compare, cc::invoke(get, ++first), pivot))
        {
            // empty
        }
        while (!cc::invoke(compare, cc::invoke(get, --last), pivot))
        {
            // empty
        }
    }

    // move pivot to correct pos
    int64_t pivot_pos = first - 1;
    cc::invoke(swap, pivot_pos, start);

    return pivot_pos;
}

template <class GetF, class CompareF, class SwapF, class SelectF>
void sort_ex_impl(int64_t start, int64_t size, GetF& get, CompareF& compare, SwapF& swap, SelectF& select)
{
    if (size <= detail::small_sort_threshold)
    {
        detail::small_sort(start, size, get, compare, swap);
        return;
    }


    // TODO: linear time for two element types

    // TODO: some tail recursion opt? or simply a local stack?

    // TODO: optimization for highly unbalanced partitions

    // select pivot (median of 3 or pseudomedian of 9)
    int64_t end = start + size;
    int64_t size_half = size >> 1;
    if (size > detail::ninther_threshold)
    {
        detail::sort3(start, start + size_half, end - 1, get, compare, swap);
        detail::sort3(start + 1, start + (size_half - 1), end - 2, get, compare, swap);
        detail::sort3(start + 2, start + (size_half + 1), end - 3, get, compare, swap);
        detail::sort3(start + (size_half - 1), start + size_half, start + (size_half + 1), get, compare, swap);
        cc::invoke(swap, start, start + size_half);
    }
    else
    {
        detail::sort3(start + size_half, start, end - 1, get, compare, swap);
    }
    // NOTE: pivot is now at start

    // partition range
    bool was_already_partitioned = false;
    auto pivot_pos = detail::partition_right(was_already_partitioned, start, size, get, compare, swap);

    int64_t start_left = start;
    int64_t size_left = pivot_pos - start_left;

    int64_t start_right = pivot_pos + 1;
    int64_t size_right = end - start_right;

    if (cc::invoke(select, start_left, size_left, get, compare, swap, select))
        detail::sort_ex_impl(start_left, size_left, get, compare, swap, select);

    if (cc::invoke(select, start_right, size_right, get, compare, swap, select))
        detail::sort_ex_impl(start_right, size_right, get, compare, swap, select);
}
}

// TODO: really expose this?
template <class GetF, class CompareF, class SwapF, class SelectF>
void sort_ex(int64_t start, size_t size, GetF&& get, CompareF&& compare, SwapF&& swap, SelectF&& select)
{
    detail::sort_ex_impl(start, int64_t(size), get, compare, swap, select);
}
}
