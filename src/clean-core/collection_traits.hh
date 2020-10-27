#pragma once

#include <cstddef>
#include <type_traits>

#include <clean-core/enable_if.hh>
#include <clean-core/forward.hh>
#include <clean-core/fwd.hh>
#include <clean-core/move.hh>
#include <clean-core/priority_tag.hh>

// collection_traits<T> provide compile time information about how to use / call a collection
// the cc::collection_xyz functions are designed to provide collection trait functions in a SFINAE-friendly manner
// e.g. "decltype(*cc::collection_begin(some_range))" can be used in cc::enable_if or in decltype-SFINAE

namespace cc
{
/**
 * A trait class for querying information about a collection
 *
 * Has a strong default but can be customized per type
 *
 * NOTE: T may be const / lvalue ref
 *
 * TODO:
 *   - is_strided_contiguous?
 *   - is_map?
 *   - is_set?
 *   - is_borrow_range? / is_ref? / is_owning?
 *   - resize / capacity?
 */
template <class T, class = void>
struct collection_traits;

// TODO: move here:
//  - begin / end
//  - get
//  - is_range
//  - is_any_range
//  - is_contiguous_range
//  - is_any_contiguous_range

/// returns the begin iterator for the collection
/// NOTE: is SFINAE-friendly
template <class CollectionT, cc::enable_if<collection_traits<CollectionT>::is_range> = true>
constexpr decltype(auto) collection_begin(CollectionT&& c)
{
    return collection_traits<CollectionT>::begin(c);
}

/// returns the end iterator for the collection
/// NOTE: is SFINAE-friendly
template <class CollectionT, cc::enable_if<collection_traits<CollectionT>::is_range> = true>
constexpr decltype(auto) collection_end(CollectionT&& c)
{
    return collection_traits<CollectionT>::end(c);
}

/// returns the size of a collection
/// NOTE: is SFINAE-friendly
template <class CollectionT, cc::enable_if<collection_traits<CollectionT>::has_size> = true>
constexpr decltype(auto) collection_size(CollectionT&& c)
{
    return collection_traits<CollectionT>::size(c);
}

/// adds an element in a collection-defined semantic
/// NOTE: is SFINAE-friendly
template <class CollectionT, class T, cc::enable_if<collection_traits<CollectionT>::can_add> = true>
constexpr decltype(auto) collection_add(CollectionT&& c, T&& value)
{
    return collection_traits<CollectionT>::add(c, cc::forward<T>(value));
}

// ======= Implementation =======

namespace detail
{
// NOTE: unified naming scheme and switch begin/end as a temporary workaround for:
// https://developercommunity.visualstudio.com/content/problem/1234402/wrong-partial-specialization-with-spooky-action-at.html
template <class CollectionT, class = void>
struct has_begin_end_t : std::false_type
{
};
template <class CollectionT>
struct has_begin_end_t<CollectionT,
                       std::void_t<                                      //
                           decltype(std::declval<CollectionT>().end()),  //
                           decltype(std::declval<CollectionT>().begin()) //
                           >> : std::true_type
{
};

template <class CollectionT, class = void>
struct has_size_t : std::false_type
{
};
template <class CollectionT>
struct has_size_t<CollectionT,
                  std::void_t<                                     //
                      decltype(std::declval<CollectionT>().size()) //
                      >> : std::true_type
{
};

template <class CollectionT, class = void>
struct has_data_t : std::false_type
{
};
template <class CollectionT>
struct has_data_t<CollectionT,
                  std::void_t<                                     //
                      decltype(std::declval<CollectionT>().data()) //
                      >> : std::true_type
{
};

struct collection_op_not_supported
{
};

template <class CollectionT, class T>
constexpr auto impl_collection_add(CollectionT& c, T&& v, cc::priority_tag<6>) -> decltype(c.push_back(cc::forward<T>(v)))
{
    return c.push_back(cc::forward<T>(v));
}
template <class CollectionT, class T>
constexpr auto impl_collection_add(CollectionT& c, T&& v, cc::priority_tag<5>) -> decltype(c.add(cc::forward<T>(v)))
{
    return c.add(cc::forward<T>(v));
}
template <class CollectionT, class T>
constexpr auto impl_collection_add(CollectionT& c, T&& v, cc::priority_tag<4>) -> decltype(c.insert(cc::forward<T>(v)))
{
    return c.insert(cc::forward<T>(v));
}
template <class CollectionT, class T>
constexpr auto impl_collection_add(CollectionT& c, T&& v, cc::priority_tag<3>) -> decltype(c.push(cc::forward<T>(v)))
{
    return c.push(cc::forward<T>(v));
}
template <class CollectionT, class T>
constexpr auto impl_collection_add(CollectionT& c, T&& v, cc::priority_tag<2>) -> decltype(c << cc::forward<T>(v))
{
    return c << cc::forward<T>(v);
}
template <class CollectionT, class T>
constexpr auto impl_collection_add(CollectionT& c, T&& v, cc::priority_tag<1>) -> decltype(c += cc::forward<T>(v))
{
    return c += cc::forward<T>(v);
}
template <class CollectionT, class T>
collection_op_not_supported impl_collection_add(CollectionT& c, T&& v, cc::priority_tag<0>);

template <class CollectionT, class T>
constexpr auto collection_add(CollectionT& c, T&& v)
{
    return detail::impl_collection_add(c, cc::forward<T>(v), cc::priority_tag<6>{});
}

struct base_collection_traits
{
    static constexpr bool has_data = false;
    static constexpr bool has_size = false;
    static constexpr bool is_range = false;
    static constexpr bool is_contiguous = false;
    static constexpr bool is_fixed_size = false;
    static constexpr bool can_add = false;
};

template <class CollectionT>
struct inferred_collection_traits : base_collection_traits
{
    // NOTE: some functions should only be accessed inside "if constexpr"
    //       e.g. begin(...) is only guaranteed to work "if constexpr(is_range<...>)"

    using element_t = std::remove_reference_t<decltype(*std::declval<CollectionT>().begin())>;

    static constexpr bool has_data = has_data_t<CollectionT>::value;
    static constexpr bool has_size = has_size_t<CollectionT>::value;
    static constexpr bool is_range = has_begin_end_t<CollectionT>::value;
    static constexpr bool is_contiguous = has_data && has_size;
    static constexpr bool is_fixed_size = false;
    static constexpr bool can_add = !std::is_same_v<decltype( //
                                                        detail::collection_add(std::declval<CollectionT&>(), std::declval<element_t>())),
                                                    collection_op_not_supported>;

    static constexpr decltype(auto) begin(CollectionT& range) { return range.begin(); }
    static constexpr decltype(auto) end(CollectionT& range) { return range.end(); }
    static constexpr decltype(auto) data(CollectionT& range) { return range.data(); }
    static constexpr decltype(auto) size(CollectionT& range) { return range.size(); }

    static constexpr void add(CollectionT& range, element_t v) { detail::collection_add(range, cc::move(v)); }

    template <size_t I>
    static constexpr decltype(auto) get(CollectionT& range)
    {
        return range.data()[I];
    }
    static constexpr decltype(auto) get(CollectionT& range, size_t i) { return range.data()[i]; }
};

template <class ArrayT, class ElementT, size_t N>
struct array_collection_traits : base_collection_traits
{
    using element_t = ElementT;

    static constexpr bool has_data = true;
    static constexpr bool has_size = true;
    static constexpr bool is_range = true;
    static constexpr bool is_contiguous = true;
    static constexpr bool is_fixed_size = true;
    static constexpr size_t fixed_size = N;

    static constexpr ElementT* begin(ArrayT& range) { return range; }
    static constexpr ElementT* end(ArrayT& range) { return range + N; }
    static constexpr ElementT* data(ArrayT& range) { return range; }
    static constexpr size_t size(ArrayT&) { return N; }

    template <size_t I>
    static constexpr ElementT& get(ArrayT& range)
    {
        static_assert(I < N);
        return range[I];
    }
    static constexpr ElementT& get(ArrayT& range, size_t i) { return range[i]; }
};

template <class ArrayT, class ElementT, size_t N>
struct cc_array_collection_traits : base_collection_traits
{
    using element_t = ElementT;

    static constexpr bool has_data = true;
    static constexpr bool has_size = true;
    static constexpr bool is_range = true;
    static constexpr bool is_contiguous = true;
    static constexpr bool is_fixed_size = true;
    static constexpr size_t fixed_size = N;

    static constexpr ElementT* begin(ArrayT& range) { return range._values; }
    static constexpr ElementT* end(ArrayT& range) { return range._values + N; }
    static constexpr ElementT* data(ArrayT& range) { return range._values; }
    static constexpr size_t size(ArrayT&) { return N; }

    template <size_t I>
    static constexpr ElementT& get(ArrayT& range)
    {
        static_assert(I < N);
        return range._values[I];
    }
    static constexpr ElementT& get(ArrayT& range, size_t i) { return range._values[i]; }
};
}

template <class T, class>
struct collection_traits : detail::base_collection_traits // not a range
{
};

// specialization for normal range-based for
template <class CollectionT>
struct collection_traits<CollectionT, std::void_t<decltype(std::declval<CollectionT>().end()), decltype(std::declval<CollectionT>().begin())>>
  : detail::inferred_collection_traits<CollectionT>
{
};

// specialization for C arrays
template <class ElementT, size_t N>
struct collection_traits<ElementT[N]> : detail::array_collection_traits<ElementT[N], ElementT, N>
{
};
template <class ElementT, size_t N>
struct collection_traits<ElementT (&)[N]> : detail::array_collection_traits<ElementT (&)[N], ElementT, N>
{
};

// specialization for cc:arrays
// TODO: is there a way to reduce the amount of repetition?
template <class T>
struct collection_traits<cc::array<T>> : detail::inferred_collection_traits<cc::array<T>>
{
};
template <class T>
struct collection_traits<cc::array<T> const> : detail::inferred_collection_traits<cc::array<T> const>
{
};
template <class T>
struct collection_traits<cc::array<T>&> : detail::inferred_collection_traits<cc::array<T>&>
{
};
template <class T>
struct collection_traits<cc::array<T> const&> : detail::inferred_collection_traits<cc::array<T> const&>
{
};
template <class T, size_t N>
struct collection_traits<cc::array<T, N>> : detail::cc_array_collection_traits<cc::array<T, N>, T, N>
{
};
template <class T, size_t N>
struct collection_traits<cc::array<T, N> const> : detail::cc_array_collection_traits<cc::array<T, N> const, T, N>
{
};
template <class T, size_t N>
struct collection_traits<cc::array<T, N>&> : detail::cc_array_collection_traits<cc::array<T, N>&, T, N>
{
};
template <class T, size_t N>
struct collection_traits<cc::array<T, N> const&> : detail::cc_array_collection_traits<cc::array<T, N> const&, T, N>
{
};

}
