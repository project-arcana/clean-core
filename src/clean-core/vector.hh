#pragma once

#include <cstddef> // std::byte
#include <new>     // placement new
#include <utility> // std::forward, std::move

namespace cc
{
template <class T>
struct vector
{
    // properties
public:
    size_t size() const { return _size; }
    size_t capacity() const { return _capacity; }
    bool empty() const { return _size == 0; }
    T* data() { return _data; }
    T const* data() const { return _data; }
    T* begin() { return _data; }
    T const* begin() const { return _data; }
    T* end() { return _data + _size; }
    T const* end() const { return _data + _size; }
    T& front() { return _data[0]; }
    T const& front() const { return _data[0]; }
    T& back() { return _data[_size - 1]; }
    T const& back() const { return _data[_size - 1]; }

    T& operator[](size_t pos) { return _data[pos]; }
    T const& operator[](size_t pos) const { return _data[pos]; }

    // ctors
public:
    vector() = default;
    vector(vector const& rhs)
    {
        reserve(rhs._size);
        _copy_range(rhs._data, rhs._size, _data);
        _size = rhs._size;
    }
    vector(vector&& rhs) noexcept
    {
        _data = rhs._data;
        _size = rhs._size;
        _capacity = rhs._capacity;
        rhs._data = nullptr;
        rhs._size = 0;
        rhs._capacity = 0;
    }
    ~vector()
    {
        _destroy_reverse(_data, _size);
        _free(_data);
    }
    vector& operator=(vector const& rhs)
    {
        if (this != &rhs)
        {
            _destroy_reverse(_data, _size);
            // ensure enough memory has been allocated
            if (_capacity < rhs._size)
            {
                _free(_data);
                _data = _alloc(rhs._size);
                _capacity = rhs._size;
            }
            _copy_range(rhs._data, rhs._size, _data);
            _size = rhs._size;
        }
        return *this;
    }
    vector& operator=(vector&& rhs) noexcept
    {
        _destroy_reverse(_data, _size);
        _free(_data);
        _data = rhs._data;
        _size = rhs._size;
        _capacity = rhs._capacity;
        rhs._data = nullptr;
        rhs._size = 0;
        rhs._capacity = 0;
    }

    // methods
public:
    void push_back(T const& t)
    {
        if (_size == _capacity)
            _grow();
        new (&_data[_size]) T(t);
        ++_size;
    }

    void push_back(T&& t)
    {
        if (_size == _capacity)
            _grow();
        new (&_data[_size]) T(std::move(t));
        ++_size;
    }

    template <class... Args>
    T& emplace_back(Args&&... args)
    {
        if (_size == _capacity)
            _grow();
        new (&_data[_size]) T(std::forward<Args...>(args...));
        return _data[_size++];
    }

    void pop_back()
    {
        --_size;
        _data[_size].~T();
    }

    void reserve(size_t size)
    {
        if (size <= _capacity)
            return;
        T* new_data = _alloc(size);
        _move_range(_data, _size, new_data);
        _destroy_reverse(_data, _size);
        _free(_data);
        _data = new_data;
        _capacity = size;
    }

    void resize(size_t size)
    {
        if (size > _capacity)
            reserve(size);
        for (size_t i = _size; i < size; ++i)
            new (&_data[i]) T();
        for (size_t i = _size; i > size; --i)
            _data[i - 1].~T();
        _size = size;
    }

    void resize(size_t size, T const& default_value)
    {
        if (size > _capacity)
            reserve(size);
        for (size_t i = _size; i < size; ++i)
            new (&_data[i]) T(default_value);
        for (size_t i = _size; i > size; --i)
            _data[i - 1].~T();
        _size = size;
    }

    void clear()
    {
        _destroy_reverse(_data, _size);
        _size = 0;
    }

    void shrink_to_fit()
    {
        if (_size != _capacity)
        {
            T* new_data = _alloc(_size);
            _move_range(_data, _size, new_data);
            _free(_data);
            _data = new_data;
            _capacity = _size;
        }
    }

    bool operator==(vector const& rhs) const noexcept
    {
        if (_size != rhs._size)
            return false;
        for (size_t i = 0; i < _size; ++i)
            if (_data[i] != rhs._data[i])
                return false;
        return true;
    }
    bool operator!=(vector const& rhs) const noexcept
    {
        if (_size != rhs._size)
            return true;
        for (size_t i = 0; i < _size; ++i)
            if (_data[i] != rhs._data[i])
                return true;
        return false;
    }

    // helpers
private:
    static T* _alloc(size_t size) { return reinterpret_cast<T*>(new std::byte[size * sizeof(T)]); }
    static void _free(T* p) { delete[] reinterpret_cast<std::byte*>(p); }
    static void _move_range(T* src, size_t size, T* dest)
    {
        for (size_t i = 0; i < size; ++i)
            new (&dest[i]) T(std::move(src[i]));
    }
    static void _copy_range(T* src, size_t size, T* dest)
    {
        for (size_t i = 0; i < size; ++i)
            new (&dest[i]) T(src[i]);
    }
    static void _destroy_reverse(T* data, size_t size)
    {
        for (size_t i = size; i > 0; --i)
            data[i - 1].~T();
    }
    void _grow() { reserve(_capacity == 0 ? 1 : _capacity << 1); }

    // members
private:
    T* _data = nullptr;
    size_t _size = 0;
    size_t _capacity = 0;
};
}
