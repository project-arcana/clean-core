#pragma once

#include <initializer_list>

#include <clean-core/algorithms.hh>
#include <clean-core/assert.hh>
#include <clean-core/detail/container_impl_util.hh>
#include <clean-core/forward.hh>
#include <clean-core/function_ptr.hh>
#include <clean-core/fwd.hh>
#include <clean-core/has_operator.hh>
#include <clean-core/span.hh>
#include <clean-core/typedefs.hh>

namespace cc
{
/// a forward-declaration friendly array
template <class T>
struct fwd_array
{
    fwd_array() = default;

    explicit fwd_array(size_t size)
    {
        _op.init();
        _size = size;
        _data = new T[_size](); // default ctor!
    }

    [[nodiscard]] static fwd_array defaulted(size_t size) { return fwd_array(size); }

    [[nodiscard]] static fwd_array uninitialized(size_t size)
    {
        fwd_array a;
        a._op.init();
        a._size = size;
        a._data = new T[size];
        return a;
    }

    [[nodiscard]] static fwd_array filled(size_t size, T const& value)
    {
        fwd_array a;
        a._op.init();
        a._size = size;
        a._data = new T[size];
        cc::fill(a, value);
        return a;
    }

    fwd_array(std::initializer_list<T> data)
    {
        _op.init();
        _size = data.size();
        _data = new T[_size];
        detail::container_copy_construct_range<T>(data.begin(), _size, _data);
    }

    fwd_array(span<T> data)
    {
        _op.init();
        _size = data.size();
        _data = new T[_size];
        detail::container_copy_construct_range<T>(data.data(), _size, _data);
    }

    fwd_array(fwd_array&& a) noexcept
    {
        _op = a._op;
        _data = a._data;
        _size = a._size;
        a._data = nullptr;
        a._size = 0;
    }
    fwd_array& operator=(fwd_array&& a) noexcept
    {
        if (_data)
            _op.delete_data(*this);
        _op = a._op;
        _data = a._data;
        _size = a._size;
        a._data = nullptr;
        a._size = 0;
        return *this;
    }
    ~fwd_array()
    {
        if (_data)
            _op.delete_data(*this);
    }

    fwd_array(fwd_array const& a) = delete;
    fwd_array& operator=(fwd_array const& a) = delete;

    constexpr T* begin() { return _data; }
    constexpr T* end() { return _data + _size; }
    constexpr T const* begin() const { return _data; }
    constexpr T const* end() const { return _data + _size; }
    constexpr T* data() { return _data; }
    constexpr T const* data() const { return _data; }
    constexpr size_t size() const { return _size; }
    constexpr bool empty() const { return _size == 0; }

    constexpr T& operator[](size_t i)
    {
        CC_CONTRACT(i < _size);
        return _data[i];
    }
    constexpr T const& operator[](size_t i) const
    {
        CC_CONTRACT(i < _size);
        return _data[i];
    }

private:
    T* _data = nullptr;
    size_t _size = 0;

    struct ops
    {
        cc::function_ptr<void(fwd_array&)> delete_data = nullptr;

        void init()
        {
            static_assert(sizeof(T) > 0, "cannot construct array of incomplete object");

            delete_data = [](fwd_array& a) { delete[] a._data; };
        }
    } _op;
};
}
