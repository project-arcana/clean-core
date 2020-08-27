#pragma once

#include <initializer_list>

#include <clean-core/algorithms.hh>
#include <clean-core/allocator.hh>
#include <clean-core/assert.hh>
#include <clean-core/detail/container_impl_util.hh>
#include <clean-core/forward.hh>
#include <clean-core/span.hh>

namespace cc
{
// heap-allocated (runtime) fixed-size array
// backed by a cc::allocator
template <class T>
struct alloc_array
{
    alloc_array() : _allocator(cc::system_allocator) { static_assert(sizeof(T) > 0, "cannot make alloc_array of incomplete object"); }

    explicit alloc_array(cc::allocator* allocator) : _allocator(allocator) { CC_CONTRACT(allocator != nullptr); }

    explicit alloc_array(size_t size, cc::allocator* allocator = cc::system_allocator) : alloc_array(allocator)
    {
        _size = size;
        _data = _alloc(size);

        for (size_t i = 0; i < size; ++i)
            new (placement_new, &this->_data[i]) T();
    }

    [[nodiscard]] static alloc_array defaulted(size_t size, cc::allocator* allocator = cc::system_allocator) { return alloc_array(size, allocator); }

    [[nodiscard]] static alloc_array uninitialized(size_t size, cc::allocator* allocator = cc::system_allocator)
    {
        alloc_array a(allocator);
        a._size = size;
        a._data = a._alloc(size);
        return a;
    }

    [[nodiscard]] static alloc_array filled(size_t size, T const& value, cc::allocator* allocator = cc::system_allocator)
    {
        alloc_array a(allocator);
        a._size = size;
        a._data = a._alloc(size);
        for (size_t i = 0; i < size; ++i)
            new (placement_new, &a._data[i]) T(value);
        return a;
    }

    alloc_array(span<T const> data, cc::allocator* allocator = cc::system_allocator) : alloc_array(allocator)
    {
        _size = data.size();
        _data = _alloc(_size);
        detail::container_copy_range<T>(data.data(), _size, _data);
    }

    alloc_array(std::initializer_list<T> data, cc::allocator* allocator = cc::system_allocator)
      : alloc_array(cc::span<T const>{data.begin(), data.size()}, allocator)
    {
    }

    alloc_array(alloc_array&& a) noexcept
    {
        _data = a._data;
        _size = a._size;
        _allocator = a._allocator;
        a._data = nullptr;
        a._size = 0;
        a._allocator = cc::system_allocator;
    }
    alloc_array& operator=(alloc_array&& a) noexcept
    {
        _destroy();
        _data = a._data;
        _size = a._size;
        _allocator = a._allocator;
        a._data = nullptr;
        a._size = 0;
        a._allocator = cc::system_allocator;
        return *this;
    }

    alloc_array(alloc_array const& a) = delete;
    alloc_array& operator=(alloc_array const& a) = delete;

    ~alloc_array()
    {
        static_assert(sizeof(T) > 0, "alloc_array destructor requires complete type");
        _destroy();
    }

    void reset(cc::allocator* new_allocator, size_t new_size = 0)
    {
        _destroy();

        CC_CONTRACT(new_allocator != nullptr);
        _allocator = new_allocator;
        _size = new_size;

        if (new_size > 0)
        {
            _data = _alloc(new_size);
            for (size_t i = 0; i < new_size; ++i)
                new (placement_new, &_data[i]) T();
        }
        else
        {
            _data = nullptr;
        }
    }

    void reset(cc::allocator* new_allocator, size_t new_size, T const& new_value)
    {
        _destroy();

        CC_CONTRACT(new_allocator != nullptr);
        _allocator = new_allocator;
        _size = new_size;

        if (new_size > 0)
        {
            _data = _alloc(new_size);
            for (size_t i = 0; i < new_size; ++i)
                new (placement_new, &_data[i]) T(new_value);
        }
        else
        {
            _data = nullptr;
        }
    }

    void resize(size_t new_size, T const& value = {})
    {
        _destroy();
        _size = new_size;
        _data = _alloc(new_size);
        for (size_t i = 0; i < new_size; ++i)
            new (placement_new, &_data[i]) T(value);
    }

    constexpr T* begin() { return _data; }
    constexpr T* end() { return _data + _size; }
    constexpr T const* begin() const { return _data; }
    constexpr T const* end() const { return _data + _size; }
    constexpr T* data() { return _data; }
    constexpr T const* data() const { return _data; }
    constexpr size_t size() const { return _size; }
    constexpr size_t size_bytes() const { return _size * sizeof(T); }
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
    bool operator==(cc::span<T const> rhs) const noexcept
    {
        if (_size != rhs.size())
            return false;
        for (size_t i = 0; i < _size; ++i)
            if (!(_data[i] == rhs[i]))
                return false;
        return true;
    }

    bool operator!=(cc::span<T const> rhs) const noexcept
    {
        if (_size != rhs.size())
            return true;
        for (size_t i = 0; i < _size; ++i)
            if (_data[i] != rhs[i])
                return true;
        return false;
    }

private:
    T* _alloc(size_t size) { return reinterpret_cast<T*>(_allocator->alloc(size * sizeof(T), alignof(T))); }
    void _free(T* p) { _allocator->free(p); }

    void _destroy()
    {
        detail::container_destroy_reverse<T>(_data, _size);
        _free(_data);
    }

private:
    T* _data = nullptr;
    size_t _size = 0;
    cc::allocator* _allocator = nullptr;
};
}
