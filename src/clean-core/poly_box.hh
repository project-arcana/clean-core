#pragma once

#include <clean-core/enable_if.hh>
#include <clean-core/forward.hh>
#include <clean-core/has_operator.hh>
#include <clean-core/move.hh>

namespace cc
{
/// a polymorphic move-only value type (allocated on the heap)
/// (basically a non-nullable poly_unique_ptr)
template <class T>
struct poly_box
{
    template <class U, cc::enable_if<std::is_base_of_v<T, U>> = true>
    poly_box(U&& v)
    {
        _data = new U(cc::forward<U>(v));
    }

    template <class U, cc::enable_if<std::is_base_of_v<T, U>> = true>
    poly_box& operator=(U&& v)
    {
        delete _data;
        _data = new U(cc::forward<U>(v));
        return *this;
    }

    poly_box(poly_box&& b) noexcept
    {
        _data = b._data;
        b._data = nullptr;
    }
    poly_box& operator=(poly_box&& b) noexcept
    {
        if (_data)
            delete _data;
        _data = b._data;
        b._data = nullptr;

        return *this;
    }

    template <class U, cc::enable_if<std::is_base_of_v<T, U>> = true>
    poly_box(poly_box<U>&& b) noexcept
    {
        _data = b._data;
        b._data = nullptr;
    }
    template <class U, cc::enable_if<std::is_base_of_v<T, U>> = true>
    poly_box& operator=(poly_box<U>&& b) noexcept
    {
        if (_data)
            delete _data;
        _data = b._data;
        b._data = nullptr;

        return *this;
    }

    template <class U, class... Args>
    U& emplace(Args&&... args)
    {
        static_assert(std::is_base_of_v<T, U>, "classes not compatible");
        delete _data;
        auto p = new U(cc::forward<Args>(args)...);
        _data = p;
        return *p;
    }

    poly_box(poly_box const&) = delete;
    poly_box& operator=(poly_box const&) = delete;

    ~poly_box() { delete _data; }

    template <class U, class... Args>
    friend poly_box<U> make_poly_box(Args&&... args);

    template <class U>
    friend struct poly_box;

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
    explicit poly_box(T* data) : _data(data) {}

    T* _data;
};

template <class T, class... Args>
poly_box<T> make_poly_box(Args&&... args)
{
    return poly_box<T>(new T(cc::forward<Args>(args)...));
}
}
