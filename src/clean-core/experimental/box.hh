#pragma once

#include <clean-core/allocate.hh>
#include <clean-core/enable_if.hh>
#include <clean-core/forward.hh>
#include <clean-core/has_operator.hh>
#include <clean-core/move.hh>

namespace cc
{
/// a non-polymorphic move-only value type (allocated on the heap)
/// (basically a non-nullable unique_ptr)
template <class T>
struct box
{
    box(T const& v) { _data = cc::alloc<T>(v); }
    box(T&& v) { _data = cc::alloc<T>(cc::move(v)); }

    box& operator=(T&& v)
    {
        if (!_data) // can be in a moved-from state
            _data = cc::alloc<T>(cc::move(v));
        else
            *_data = cc::move(v);

        return *this;
    }

    box(box&& b) noexcept
    {
        _data = b._data;
        b._data = nullptr;
    }
    box& operator=(box&& b) noexcept
    {
        if (_data)
            cc::free(_data);
        _data = b._data;
        b._data = nullptr;

        return *this;
    }

    box(box const&) = delete;
    box& operator=(box const&) = delete;

    ~box()
    {
        if (_data)
            cc::free(_data);
    }

    template <class U, class... Args>
    friend box<U> make_box(Args&&... args);

    [[nodiscard]] T* get() { return _data; }
    [[nodiscard]] T const* get() const { return _data; }

    T* operator->()
    {
        CC_ASSERT_NOT_NULL(_data); // moved-from state?
        return _data;
    }
    T const* operator->() const
    {
        CC_ASSERT_NOT_NULL(_data); // moved-from state?
        return _data;
    }
    T& operator*()
    {
        CC_ASSERT_NOT_NULL(_data); // moved-from state?
        return *_data;
    }
    T const& operator*() const
    {
        CC_ASSERT_NOT_NULL(_data); // moved-from state?
        return *_data;
    }

    operator T const&() const { return *_data; }
    operator T&() { return *_data; }

private:
    explicit box(T* data) : _data(data) {}

    T* _data;
};

template <class T, class... Args>
box<T> make_box(Args&&... args)
{
    return box<T>(cc::alloc<T>(cc::forward<Args>(args)...));
}

// TODO: is making these SFINAE-friendly worth it?

template <class A, class B>
bool operator==(box<A> const& a, box<B> const& b)
{
    return *a == *b;
}
template <class A, class B>
bool operator!=(box<A> const& a, box<B> const& b)
{
    return *a != *b;
}
template <class A, class B>
bool operator<(box<A> const& a, box<B> const& b)
{
    return *a < *b;
}
template <class A, class B>
bool operator>(box<A> const& a, box<B> const& b)
{
    return *a > *b;
}
template <class A, class B>
bool operator<=(box<A> const& a, box<B> const& b)
{
    return *a <= *b;
}
template <class A, class B>
bool operator>=(box<A> const& a, box<B> const& b)
{
    return *a >= *b;
}

}
