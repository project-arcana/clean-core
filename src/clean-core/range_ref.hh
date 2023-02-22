#pragma once

#include <initializer_list>

#include <clean-core/collection_traits.hh>
#include <clean-core/enable_if.hh>
#include <clean-core/function_ref.hh>
#include <clean-core/is_range.hh>
#include <clean-core/iterator.hh>

namespace cc
{
namespace detail
{
template <class From, class To, class = void>
struct is_deref_convertible : std::false_type
{
};
template <class From, class To>
struct is_deref_convertible<From, To, std::enable_if_t<std::is_convertible_v<decltype(*std::declval<From>()), To>, void>> : std::true_type
{
};
template <class T, class = void>
struct try_deref_t
{
    using type = void;
};
template <class T>
struct try_deref_t<T, std::void_t<decltype(*std::declval<T>())>>
{
    using type = decltype(*std::declval<T>());
};

template <class Range, class To>
constexpr bool is_convertible_range = cc::is_any_range<Range> //
                                      && std::is_convertible_v<cc::collection_element_t<Range>, To>;

template <class Range, class To>
constexpr bool is_deref_convertible_range = cc::is_any_range<Range> //
                                            && std::is_convertible_v<typename try_deref_t<cc::collection_element_t<Range>>::type, To>;
}

/// a type-erased range of elements that can be converted to T
/// NOTE: any range assigned to this must always outlive the range_ref.
///       this is designed to be mainly used as function argument (similar to span, string_view, stream_ref, etc.)
///
/// TODO: expose function<void(function<void(T)>)> interface
///
/// Implementation note:
///   the templated constructors use the SFINAE-friendly is_convertible_range, is_deref_convertible_range
///   and are themselves designed to support SFINAE.
///   this is important so that functions can be overloaded on different types of range_ref<T>s
///   no-deref and deref must also be the same constructor, because types can be both at the same time
///   in this case, we prefer the no-deref version
template <class T>
struct range_ref
{
    /// empty range
    range_ref()
    {
        _for_each = [](storage const&, cc::function_ref<void(T)>) {};
    }

    /// create a range_ref from any range (Range must support range-based-for)
    /// NOTE: range element types must be convertible to T
    /// CAUTION: range must always outlive the range_ref!
    template <class Range,
              cc::enable_if<std::is_convertible_v<decltype(*cc::collection_begin(std::declval<Range>())), T> || //
                            detail::is_deref_convertible<decltype(*cc::collection_begin(std::declval<Range>())), T>::value>
              = true>
    range_ref(Range&& range)
    {
        constexpr bool is_direct_convertible = std::is_convertible_v<decltype(*cc::collection_begin(std::declval<Range>())), T>;

        if constexpr (std::is_const_v<std::remove_reference_t<Range>>)
        {
            _range.obj_const = &range;
            if constexpr (is_direct_convertible)
                _for_each = [](storage const& s, cc::function_ref<void(T)> f)
                {
                    for (auto&& v : *static_cast<decltype(&range)>(s.obj_const))
                        f(v);
                };
            else
                _for_each = [](storage const& s, cc::function_ref<void(T)> f)
                {
                    for (auto&& v : *static_cast<decltype(&range)>(s.obj_const))
                        f(*v);
                };
        }
        else
        {
            _range.obj = &range;
            if constexpr (is_direct_convertible)
                _for_each = [](storage const& s, cc::function_ref<void(T)> f)
                {
                    for (auto&& v : *static_cast<decltype(&range)>(s.obj))
                        f(v);
                };
            else
                _for_each = [](storage const& s, cc::function_ref<void(T)> f)
                {
                    for (auto&& v : *static_cast<decltype(&range)>(s.obj))
                        f(*v);
                };
        }
    }

    /// creates a range_ref based on fixed values
    template <class U, cc::enable_if<std::is_convertible_v<U const&, T> || detail::is_deref_convertible<U const&, T>::value> = true>
    range_ref(std::initializer_list<U> const& range)
    {
        _range.obj_const = &range;
        if constexpr (std::is_convertible_v<U const&, T>)
            _for_each = [](storage const& s, cc::function_ref<void(T)> f)
            {
                for (auto&& v : *static_cast<decltype(&range)>(s.obj_const))
                    f(v);
            };
        else
            _for_each = [](storage const& s, cc::function_ref<void(T)> f)
            {
                for (auto&& v : *static_cast<decltype(&range)>(s.obj_const))
                    f(*v);
            };
    }

    /// iterates over all elements in the range and calls f for them
    void for_each(cc::function_ref<void(T)> f) const { _for_each(_range, f); }

private:
    union storage
    {
        void* obj;             // pointer to range
        void const* obj_const; // pointer to const range

        storage() { obj = nullptr; }
    };

    storage _range;
    cc::function_ptr<void(storage const&, cc::function_ref<void(T)>)> _for_each = nullptr;
};

/// creates an object that can be used as range_ref<T>
/// CAUTION: the result should be passed directly to a function accepting range_ref<T>
///          (otherwise one can quickly get lifetime issues)
template <class T, class Range>
range_ref<T> make_range_ref(Range&& range)
{
    static_assert(cc::is_any_range<Range>, "only works for ranges");
    return range;
};
/// same as make_range_ref<T> but tries to infer T
/// NOTE: for technical reasons this just returns "range"
///       the actual inference happens in the ctor of range_ref<T>
template <class Range>
decltype(auto) make_range_ref(Range&& range)
{
    static_assert(cc::is_any_range<Range>, "only works for ranges");
    return range;
};
/// support for cc::make_range_ref({1, 2, 3})
/// NOTE: for technical reasons this just returns "range"
///       the actual inference happens in the ctor of range_ref<T>
template <class T>
decltype(auto) make_range_ref(std::initializer_list<T> const& range)
{
    return range;
}
}
