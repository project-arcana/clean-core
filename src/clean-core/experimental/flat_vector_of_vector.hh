#pragma once

#include <clean-core/sentinel.hh>
#include <clean-core/span.hh>
#include <clean-core/vector.hh>

namespace cc
{
namespace detail
{
template <class T, class IndexT>
struct flat_vector_of_vector_iterator;
}

/// an efficient, array-embedded version of vector<vector<T>>
/// as a range, behaves like [span<T>]
/// IndexT points into the inner vector
template <class T, class IndexT = size_t>
struct flat_vector_of_vector
{
    using index_t = IndexT;

    // container
public:
    size_t size() const { return _start_idx.size(); }
    bool empty() const { return _start_idx.empty(); }

    cc::span<IndexT> start_indices() { return _start_idx; }
    cc::span<IndexT const> start_indices() const { return _start_idx; }

    cc::span<T> elements() { return _elements; }
    cc::span<T const> elements() const { return _elements; }

    cc::span<T> operator[](size_t i)
    {
        CC_CONTRACT(size_t(i) < _start_idx.size());
        auto start_idx = _start_idx[i];
        auto end_idx = i + 1 == _start_idx.size() ? _elements.size() : _start_idx[i + 1];
        return {_elements.data() + start_idx, _elements.data() + end_idx};
    }
    cc::span<T const> operator[](size_t i) const
    {
        CC_CONTRACT(size_t(i) < _start_idx.size());
        auto start_idx = _start_idx[i];
        auto end_idx = i + 1 == _start_idx.size() ? _elements.size() : _start_idx[i + 1];
        return {_elements.data() + start_idx, _elements.data() + end_idx};
    }

    // mutation
public:
    void start_new_range() { _start_idx.push_back(index_t(_elements.size())); }

    /// adds an element to the last range
    template <class... Args>
    T& emplace_back_element(Args&&... args)
    {
        CC_ASSERT(!_start_idx.empty() && "no ranges present. did you forget to call start_new_range()?");
        return _elements.emplace_back(cc::forward<Args>(args)...);
    }
    /// adds an element to the last range
    T& push_back_element(T const& value)
    {
        static_assert(std::is_copy_constructible_v<T>, "only works with copyable types. did you forget a cc::move?");
        return emplace_back_element(value);
    }
    /// adds an element to the last range
    T& push_back_element(T&& value) { return emplace_back_element(cc::move(value)); }

    /// adds all range elements to the last range
    /// NOTE: does NOT start a new range before
    template <class Range>
    void push_back_elements(Range&& range)
    {
        CC_ASSERT(!_start_idx.empty() && "no ranges present. did you forget to call start_new_range()?");
        _elements.push_back_range(cc::forward<Range>(range));
    }

    void clear()
    {
        _elements.clear();
        _start_idx.clear();
    }
    void reserve_elements(size_t capacity) { _elements.reserve(capacity); }
    void reserve_ranges(size_t capacity) { _start_idx.reserve(capacity); }

    // iteration
public:
    detail::flat_vector_of_vector_iterator<T, index_t> begin()
    {
        return {_elements.data(), _elements.data() + _elements.size(), _start_idx.data(), _start_idx.data() + _start_idx.size()};
    }
    detail::flat_vector_of_vector_iterator<T const, index_t> begin() const
    {
        return {_elements.data(), _elements.data() + _elements.size(), _start_idx.data(), _start_idx.data() + _start_idx.size()};
    }
    cc::sentinel end() const { return {}; }

private:
    cc::vector<T> _elements;
    cc::vector<index_t> _start_idx; // TODO: maybe "last idx" has better codegen?
};

namespace detail
{
template <class T, class IndexT>
struct flat_vector_of_vector_iterator
{
    flat_vector_of_vector_iterator() = default;
    flat_vector_of_vector_iterator(T* element_data, T* element_end, IndexT const* curr_idx, IndexT const* end_idx)
      : _element_data(element_data), _element_end(element_end), _curr_idx(curr_idx), _end_idx(end_idx)
    {
    }

    bool operator!=(cc::sentinel) const { return _curr_idx != _end_idx; }
    void operator++() { ++_curr_idx; }
    cc::span<T> operator*() const
    {
        auto e_start = _element_data + *_curr_idx;
        auto e_end = _curr_idx + 1 == _end_idx ? _element_end : _element_data + *(_curr_idx + 1);
        return {e_start, e_end};
    }

private:
    T* _element_data;
    T* _element_end;
    IndexT const* _curr_idx;
    IndexT const* _end_idx;
};
}
}
