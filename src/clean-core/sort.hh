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
/// Generic customizable sort function. Basis for all convenience wrappers based on sort
/// This function works on a "virtual" range from "start..start+size-1"
/// @param get : (int64_t idx) -> T, provides the elements to compare
/// @param compare : (T const& a, T const& b) -> bool, is true if "a < b", i.e. a should be before b
/// @param swap : (int64_t a, int64_t b) -> void, swaps the virtual elements at a and b
/// @param select : (int64_t start, size_t size) -> bool, subrange is sorted only if true
///
/// Requirements:
/// - compare must be a total order, i.e. irreflexive, transitive, antisymmetric, total
///
/// TODO:
/// - insertion_sort is typically move-based, not swap-based, so we cannot use it here
/// - other steps of pdqsort also depend on local tmp copies that cannot be used
/// - maybe a special swap location? prob better as sep arg
/// - is_cheap_to_store trait (e.g. trivially copyable <= 256 bit) so that some get invocations can be removed
/// - parallel version
template <class GetF, class CompareF, class SwapF, class SelectF>
constexpr void sort_ex(int64_t start, size_t size, GetF&& get, CompareF&& compare, SwapF&& swap, SelectF&& select);

/// Generic customizable partition function
/// Swaps elements such that the range consists of two blocks
/// The first block has "is_right(i) == false", the second has "is_right(i) == true"
/// Returns the index of the first "right" element, i.e. is_right(ret) == true and if ret > start, then is_right(ret - 1) == false
/// Return value is 0 if all elements are right
/// Return value is start+size if all elements are left
/// @param is_right : (int64_t i) -> bool, "true" means "right", "false" means "left"
/// @param swap : (int64_t a, int64_t b) -> void, swaps the virtual elements at a and b
/// runs in O(size)
template <class IsRightF, class SwapF>
constexpr int64_t partition_ex(int64_t start, size_t size, IsRightF&& is_right, SwapF&& swap);

/// ensures that the element at idx is "correctly sorted"
/// i.e. if collection were completely sorted but without actually sorting everything
/// has O(collection.size * log collection.size) time complexity (avg and worst case)
/// and O(collection.size) time complexity best case
///   TODO: needs to be implemented for full pdqsort guarantees
/// is deterministic, but not stable
/// @param compare : (T const& a, T const& b) -> bool, is true if "a < b", i.e. a should be before b
/// @param key: (Element const& e) -> T, selects the key to be sorted by
template <class Collection, class CompareF = cc::less<void>>
constexpr void sort(Collection&& collection, CompareF&& compare = {});
template <class Collection, class KeyF, class CompareF = cc::less<void>>
constexpr void sort_by(Collection&& collection, KeyF&& key, CompareF&& compare = {});
template <class Collection>
constexpr void sort_descending(Collection&& collection);
template <class Collection, class KeyF>
constexpr void sort_by_descending(Collection&& collection, KeyF&& key);

/// sorts multiple collections simultaneously,
/// i.e. sorts the keys collection but also swaps elements in all other collections to maintain synchronized order
/// NOTE: comparison/key function are mandatory and at the beginning due to variadic arguments
///       the usual comparison is "cc::less<>{}", descending is "cc::greater<>{}"
/// @param compare : (KeyT const& a, KeyT const& b) -> bool, is true if "a < b", i.e. a should be before b
/// @param key: (Element const&... elements) -> T, selects the key to be sorted by (NOTE: gets _all_ elements as input)
template <class CompareF, class KeyCollection, class... Collections>
constexpr void sort_multi(CompareF&& compare, KeyCollection&& keys, Collections&&... collections);
template <class KeyF, class CompareF, class... Collections>
constexpr void sort_multi_by(KeyF&& key, CompareF&& compare, Collections&&... collections);

/// swaps elements in collection such that we have two blocks:
///   is_right(e) is false for the first block
///   is_right(e) is true for the second block
/// returns the first index of the second block, or collection_size if none are right
/// (is conceptually the same as sort_by(collection, is_right) where false < true)
/// runs in O(collection.size)
template <class Collection, class IsRightF>
constexpr int64_t partition_by(Collection&& collection, IsRightF&& is_right);

/// ensures that the element at idx is "correctly sorted"
/// i.e. if collection were completely sorted but without actually sorting everything
/// has O(collection.size) time complexity
/// also guarantees that everything below and above "idx" is "in the correct partition"
/// i.e. all elements before idx actually belong before index (and same with above)
///      even if they are themselves not necessarily sorted
/// @param compare : (T const& a, T const& b) -> bool, is true if "a < b", i.e. a should be before b
/// @param key: (Element const& e) -> T, selects the key to be sorted by
template <class Collection, class CompareF = cc::less<void>>
constexpr void quickselect(Collection&& collection, size_t idx, CompareF&& compare = {});
template <class Collection, class KeyF, class CompareF = cc::less<void>>
constexpr void quickselect_by(Collection&& collection, size_t idx, KeyF&& key, CompareF&& compare = {});

/// makes sure that all elements from idx..idx+count-1 are what they would be if collection were completely sorted
/// but without actually sorting the whole collection
/// takes O(collection_size + count * log count) time
/// NOTE: idx + count is not required to be in bounds
/// also guarantees that everything below and above the subrange is "in the correct partition"
/// i.e. all elements before the subrange actually belong before (and same with above)
///      even if they are themselves not necessarily sorted
/// @param compare : (T const& a, T const& b) -> bool, is true if "a < b", i.e. a should be before b
/// @param key: (Element const& e) -> T, selects the key to be sorted by
template <class Collection, class CompareF = cc::less<void>>
constexpr void quickselect_range(Collection&& collection, size_t idx, size_t count, CompareF&& compare = {});
template <class Collection, class KeyF, class CompareF = cc::less<void>>
constexpr void quickselect_range_by(Collection&& collection, size_t idx, size_t count, KeyF&& key, CompareF&& compare = {});

/// returns true iff the collection is sorted according to comparison/key criterion
/// takes O(collection_size time)
/// @param compare : (T const& a, T const& b) -> bool, is true if "a < b", i.e. a should be before b
/// @param key: (Element const& e) -> T, selects the key to be sorted by
template <class Collection, class CompareF = cc::less<void>>
[[nodiscard]] constexpr bool is_sorted(Collection&& collection, CompareF&& compare = {});
template <class Collection, class KeyF, class CompareF = cc::less<void>>
[[nodiscard]] constexpr bool is_sorted_by(Collection&& collection, KeyF&& key, CompareF&& compare = {});
template <class GetF, class CompareF>
[[nodiscard]] constexpr bool is_sorted_ex(int64_t start, size_t size, GetF&& get, CompareF&& compare);

//
// Implementation
//

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
constexpr void sort2(int64_t ia, int64_t ib, GetF& get, CompareF& compare, SwapF& swap)
{
    auto&& a = cc::invoke(get, ia);
    auto&& b = cc::invoke(get, ib);
    if (cc::invoke(compare, b, a))
        cc::invoke(swap, ia, ib);
}

template <class GetF, class CompareF, class SwapF>
constexpr void sort3(int64_t ia, int64_t ib, int64_t ic, GetF& get, CompareF& compare, SwapF& swap)
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
constexpr void small_sort(int64_t start, int64_t size, GetF& get, CompareF& compare, SwapF& swap)
{
    if (size <= 1)
        return;

    if (size == 2)
    {
        detail::sort2(start, start + 1, get, compare, swap);
        return;
    }

    if (size == 3)
    {
        detail::sort3(start, start + 1, start + 2, get, compare, swap);
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
constexpr int64_t partition_right(bool& was_already_partitioned, int64_t start, int64_t size, GetF&& get, CompareF&& compare, SwapF&& swap)
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
constexpr void sort_ex_impl(int64_t start, int64_t size, GetF& get, CompareF& compare, SwapF& swap, SelectF& select)
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

    if (cc::invoke(select, start_left, size_left))
        detail::sort_ex_impl(start_left, size_left, get, compare, swap, select);

    if (cc::invoke(select, start_right, size_right))
        detail::sort_ex_impl(start_right, size_right, get, compare, swap, select);
}

// the default collection types that follow _could_ be lambdas
// however:
// - lambdas are generated per instantiation
//   and thus needlessly pollute binary symbols
//   and increase compile times

// TODO:
// - see if trying to get iterators and use "it + i" instead is worth it

template <class Collection>
struct collection_access
{
    Collection collection;

    constexpr decltype(auto) operator()(int64_t i) const { return collection[i]; }
};
template <class Collection, class KeyF>
struct collection_key_access
{
    Collection collection;
    KeyF key;

    constexpr decltype(auto) operator()(int64_t i) const { return cc::invoke(key, collection[i]); }
};
template <class Collection>
struct collection_swap
{
    Collection collection;

    constexpr void operator()(int64_t a, int64_t b) const { cc::swap(collection[a], collection[b]); }
};

struct index_in_range
{
    int64_t idx;

    constexpr bool operator()(int64_t start, int64_t size) const { return start <= idx && idx < start + size; }
};
struct overlaps_range
{
    int64_t idx;
    int64_t count;

    constexpr bool operator()(int64_t start, int64_t size) const { return start <= idx + count && idx <= start + size; }
};
}

// TODO: really expose this?
template <class GetF, class CompareF, class SwapF, class SelectF>
constexpr void sort_ex(int64_t start, size_t size, GetF&& get, CompareF&& compare, SwapF&& swap, SelectF&& select)
{
    detail::sort_ex_impl(start, int64_t(size), get, compare, swap, select);
}

template <class IsRightF, class SwapF>
constexpr int64_t partition_ex(int64_t start, size_t size, IsRightF&& is_right, SwapF&& swap)
{
    int64_t end = start + size;
    int64_t first = start;
    int64_t last = end - 1;

    while (true)
    {
        while (first < end && !cc::invoke(is_right, first))
            first++;
        while (last > start && cc::invoke(is_right, last))
            last--;

        if (first >= last)
            break;

        cc::invoke(swap, first, last);
        ++first;
        --last;
    }

    return first;
}

template <class Collection, class IsRightF>
constexpr int64_t partition_by(Collection&& collection, IsRightF&& is_right)
{
    static_assert(collection_traits<Collection>::is_range);
    size_t size = cc::collection_size(collection);
    return cc::partition_ex(0, size,                                                                     //
                            detail::collection_key_access<Collection&, IsRightF&>{collection, is_right}, //
                            detail::collection_swap<Collection&>{collection});
}

// TODO: move lambdas out into detail namespace
//       so they don't get generated multiple times (with diff compare funs)
template <class Collection, class CompareF>
constexpr void sort(Collection&& collection, CompareF&& compare)
{
    static_assert(collection_traits<Collection>::is_range);
    size_t size = cc::collection_size(collection);
    cc::sort_ex(0, size,                                            //
                detail::collection_access<Collection&>{collection}, //
                compare,                                            //
                detail::collection_swap<Collection&>{collection},   //
                cc::constant_function<true>{});
}
template <class Collection, class KeyF, class CompareF>
constexpr void sort_by(Collection&& collection, KeyF&& key, CompareF&& compare)
{
    static_assert(collection_traits<Collection>::is_range);
    size_t size = cc::collection_size(collection);
    cc::sort_ex(0, size,                                                            //
                detail::collection_key_access<Collection&, KeyF&>{collection, key}, //
                compare,                                                            //
                detail::collection_swap<Collection&>{collection},                   //
                cc::constant_function<true>{});
}
template <class Collection>
constexpr void sort_descending(Collection&& collection)
{
    cc::sort(collection, cc::greater<void>{});
}
template <class Collection, class KeyF>
constexpr void sort_by_descending(Collection&& collection, KeyF&& key)
{
    cc::sort_by(collection, key, cc::greater<void>{});
}

template <class CompareF, class KeyCollection, class... Collections>
constexpr void sort_multi(CompareF&& compare, KeyCollection&& keys, Collections&&... collections)
{
    static_assert(collection_traits<KeyCollection>::is_range);
    static_assert((collection_traits<Collections>::is_range && ...));
    size_t size = cc::collection_size(keys);
    CC_ASSERT(((cc::collection_size(collections) == size) && ...) && "collections must have the same size");
    cc::sort_ex(
        0, size,                                         //
        detail::collection_access<KeyCollection&>{keys}, //
        compare,                                         //
        [&](int64_t a, int64_t b)
        {
            cc::swap(keys[a], keys[b]);
            (cc::swap(collections[a], collections[b]), ...);
        },
        cc::constant_function<true>{});
}
template <class KeyF, class CompareF, class... Collections>
constexpr void sort_multi_by(KeyF&& key, CompareF&& compare, Collections&&... collections)
{
    static_assert(sizeof...(collections) >= 1, "must sort at least one collection");
    static_assert((collection_traits<Collections>::is_range && ...));
    size_t size = (cc::collection_size(collections), ...);
    CC_ASSERT(((cc::collection_size(collections) == size) && ...) && "collections must have the same size");
    cc::sort_ex(
        0, size,                                                                        //
        [&](int64_t i) { return key(collections[i]...); },                              //
        compare,                                                                        //
        [&](int64_t a, int64_t b) { (cc::swap(collections[a], collections[b]), ...); }, //
        cc::constant_function<true>{});
}

template <class Collection, class CompareF>
constexpr void quickselect(Collection&& collection, size_t idx, CompareF&& compare)
{
    static_assert(collection_traits<Collection>::is_range);
    size_t size = cc::collection_size(collection);
    cc::sort_ex(0, size,                                            //
                detail::collection_access<Collection&>{collection}, //
                compare,                                            //
                detail::collection_swap<Collection&>{collection},   //
                detail::index_in_range{int64_t(idx)});
}
template <class Collection, class KeyF, class CompareF>
constexpr void quickselect_by(Collection&& collection, size_t idx, KeyF&& key, CompareF&& compare)
{
    static_assert(collection_traits<Collection>::is_range);
    size_t size = cc::collection_size(collection);
    cc::sort_ex(0, size,                                                            //
                detail::collection_key_access<Collection&, KeyF&>{collection, key}, //
                compare,                                                            //
                detail::collection_swap<Collection&>{collection},                   //
                detail::index_in_range{int64_t(idx)});
}

template <class Collection, class CompareF>
constexpr void quickselect_range(Collection&& collection, size_t idx, size_t count, CompareF&& compare)
{
    static_assert(collection_traits<Collection>::is_range);
    size_t size = cc::collection_size(collection);
    cc::sort_ex(0, size,                                            //
                detail::collection_access<Collection&>{collection}, //
                compare,                                            //
                detail::collection_swap<Collection&>{collection},   //
                detail::overlaps_range{int64_t(idx), int64_t(count)});
}
template <class Collection, class KeyF, class CompareF>
constexpr void quickselect_range_by(Collection&& collection, size_t idx, size_t count, KeyF&& key, CompareF&& compare)
{
    static_assert(collection_traits<Collection>::is_range);
    size_t size = cc::collection_size(collection);
    cc::sort_ex(0, size,                                                            //
                detail::collection_key_access<Collection&, KeyF&>{collection, key}, //
                compare,                                                            //
                detail::collection_swap<Collection&>{collection},                   //
                detail::overlaps_range{int64_t(idx), int64_t(count)});
}

template <class GetF, class CompareF>
[[nodiscard]] constexpr bool is_sorted_ex(int64_t start, size_t size, GetF&& get, CompareF&& compare)
{
    int64_t end = start + size;
    for (int64_t i = start + 1; i < end; ++i)
        if (cc::invoke(compare, cc::invoke(get, i), cc::invoke(get, i - 1)))
            return false;
    return true;
}
template <class Collection, class CompareF>
[[nodiscard]] constexpr bool is_sorted(Collection&& collection, CompareF&& compare)
{
    static_assert(collection_traits<Collection>::is_range);
    size_t size = cc::collection_size(collection);
    return cc::is_sorted_ex(0, size, detail::collection_access<Collection&>{collection}, compare);
}
template <class Collection, class KeyF, class CompareF>
[[nodiscard]] constexpr bool is_sorted_by(Collection&& collection, KeyF&& key, CompareF&& compare)
{
    static_assert(collection_traits<Collection>::is_range);
    size_t size = cc::collection_size(collection);
    return cc::is_sorted_ex(0, size, detail::collection_key_access<Collection&, KeyF&>{collection, key}, compare);
}
}
