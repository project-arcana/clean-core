#pragma once

#include <clean-core/forward.hh>
#include <clean-core/macros.hh>
#include <clean-core/sentinel.hh>

namespace cc::detail
{
// generic sentinel range
// (requires ItT != cc::sentinel check and copyable iterator)
template <class ItT>
struct srange
{
    template <class... Args>
    CC_FORCE_INLINE explicit srange(Args&&... args) : it(cc::forward<Args>(args)...)
    {
    }

    CC_FORCE_INLINE auto begin() { return it; }
    CC_FORCE_INLINE auto begin() const { return it; }
    CC_FORCE_INLINE cc::sentinel end() const { return {}; }

private:
    ItT it;
};

template <class T>
struct iiterator
{
    T _curr;

    CC_FORCE_INLINE constexpr bool operator!=(iiterator rhs) const { return _curr != rhs._curr; }
    CC_FORCE_INLINE constexpr T operator*() const { return _curr; }
    CC_FORCE_INLINE constexpr void operator++() { ++_curr; }
};

template <class T>
struct rev_iiterator
{
    T _curr;

    CC_FORCE_INLINE constexpr bool operator!=(rev_iiterator rhs) const { return _curr != rhs._curr; }
    CC_FORCE_INLINE constexpr T operator*() const { return _curr; }
    CC_FORCE_INLINE constexpr void operator++() { --_curr; }
};

template <class T>
struct irange;
template <class T>
struct rev_irange;

// a dumb range that iterates from begin to end (exclusive)
// creates the same asm code as:
//   for (T i = _begin; i != _end; ++i)
// see https://godbolt.org/z/vvEKno4jT
template <class T>
struct irange
{
    T _begin;
    T _end; // _end >= _begin

    CC_FORCE_INLINE constexpr iiterator<T> begin() const { return {_begin}; }
    CC_FORCE_INLINE constexpr iiterator<T> end() const { return {_end}; }

    // NOTE: is no-op for empty ranges
    CC_FORCE_INLINE constexpr irange skip_first() const
    {
        if (_begin == _end)
            return *this;

        auto r = *this; // copy
        ++r._begin;
        return r;
    }
    // NOTE: is no-op for empty ranges
    CC_FORCE_INLINE constexpr irange skip_last() const
    {
        if (_begin == _end)
            return *this;

        auto r = *this; // copy
        --r._end;
        return r;
    }

    /// ensures that any iterated value is at most "v"
    CC_FORCE_INLINE constexpr irange max(T v) const
    {
        if (v >= _end)
            return *this; // already in range
        if (v < _begin)
            return {}; // empty range
        return {_begin, v + 1};
    }

    CC_FORCE_INLINE constexpr rev_irange<T> reversed() const
    {
        auto b = _end;
        auto e = _begin;
        --b;
        --e;
        return {b, e};
    }
};
template <class T>
struct rev_irange
{
    T _begin;
    T _end;

    CC_FORCE_INLINE constexpr rev_iiterator<T> begin() const { return {_begin}; }
    CC_FORCE_INLINE constexpr rev_iiterator<T> end() const { return {_end}; }

    CC_FORCE_INLINE constexpr irange<T> reversed() const
    {
        auto b = _end;
        auto e = _begin;
        ++b;
        ++e;
        return {b, e};
    }
};
} // namespace cc::detail
