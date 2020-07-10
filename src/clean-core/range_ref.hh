#pragma once

#include <initializer_list>

#include <clean-core/function_ref.hh>
#include <clean-core/is_range.hh>
#include <clean-core/iterator.hh>

namespace cc
{
/// a type-erased range of elements that can be converted to T
template <class T>
struct range_ref
{
    /// empty range
    range_ref()
    {
        _for_each = [](cc::function_ref<void(T)>) {};
    }

    /// NOTE: make_range_ref is easier to use
    template <class F, cc::enable_if<std::is_invocable_r_v<void, F, cc::function_ref<void(T)>>> = true>
    range_ref(F&& for_each_fun)
    {
        _for_each = for_each_fun;
    }

    /// iterates over all elements in the range and calls f for them
    void for_each(cc::function_ref<void(T)> f) { _for_each(f); }

private:
    cc::function_ref<void(cc::function_ref<void(T)>)> _for_each;
};

/// creates an object that can be used as range_ref<T>
/// CAUTION: the result should be passed directly to a function accepting range_ref<T>
///          (otherwise one can quickly get lifetime issues)
template <class T, class Range>
auto make_range_ref(Range&& range)
{
    static_assert(cc::is_any_range<Range>, "only works for ranges");
    return [&range](cc::function_ref<void(T)> f) {
        for (auto&& v : range)
            f(v);
    };
};
/// same as make_range_ref<T> but tries to infer T
template <class Range>
auto make_range_ref(Range&& range)
{
    static_assert(cc::is_any_range<Range>, "only works for ranges");
    using T = decltype(*cc::begin(range));
    return [&range](cc::function_ref<void(T)> f) {
        for (auto&& v : range)
            f(v);
    };
};
/// support for cc::make_range_ref({1, 2, 3})
template <class T>
auto make_range_ref(std::initializer_list<T> range)
{
    // NOTE: capturing range per value is fine as long as its storage outlives the range_ref
    return [range](cc::function_ref<void(T)> f) {
        for (auto&& v : range)
            f(v);
    };
}
}
