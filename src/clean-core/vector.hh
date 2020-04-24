#pragma once

#include <cstddef> // std::byte
#include <cstring> // std::memcpy
#include <initializer_list>
#include <type_traits>

#include <clean-core/assert.hh>
#include <clean-core/detail/container_impl_util.hh>
#include <clean-core/forward.hh>
#include <clean-core/fwd.hh>
#include <clean-core/hash_combine.hh>
#include <clean-core/invoke.hh>
#include <clean-core/is_range.hh>
#include <clean-core/move.hh>
#include <clean-core/new.hh>
#include <clean-core/span.hh>

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

    T& front()
    {
        CC_CONTRACT(!empty());
        return _data[0];
    }
    T const& front() const
    {
        CC_CONTRACT(!empty());
        return _data[0];
    }
    T& back()
    {
        CC_CONTRACT(!empty());
        return _data[_size - 1];
    }
    T const& back() const
    {
        CC_CONTRACT(!empty());
        return _data[_size - 1];
    }

    T& operator[](size_t i)
    {
        CC_CONTRACT(i < _size);
        return _data[i];
    }
    T const& operator[](size_t i) const
    {
        CC_CONTRACT(i < _size);
        return _data[i];
    }

    // ctors
public:
    vector() = default;

    explicit vector(size_t size) : _data(_alloc(size)), _size(size), _capacity(size)
    {
        for (size_t i = 0; i < size; ++i)
            new (placement_new, &_data[i]) T();
    }

    [[nodiscard]] static vector defaulted(size_t size) { return vector(size); }

    [[nodiscard]] static vector uninitialized(size_t size)
    {
        vector v;
        v._size = size;
        v._capacity = size;
        v._data = _alloc(size);
        return v;
    }

    [[nodiscard]] static vector filled(size_t size, T const& value)
    {
        vector v;
        v.resize(size, value);
        return v;
    }

    vector(T const* begin, size_t num_elements)
    {
        reserve(num_elements);
        detail::container_copy_range<T>(begin, num_elements, _data);
        _size = num_elements;
    }
    vector(std::initializer_list<T> data) : vector(data.begin(), data.size()) {}
    vector(cc::span<T const> data) : vector(data.begin(), data.size()) {}
    vector(vector const& rhs) : vector(rhs.begin(), rhs.size()) {}

    template <class Range, cc::enable_if<cc::is_any_range<Range>> = true>
    explicit vector(Range const& range)
    {
        for (auto const& e : range)
            this->emplace_back(e);
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
        detail::container_destroy_reverse<T>(_data, _size);
        _free(_data);
    }
    vector& operator=(vector const& rhs)
    {
        if (this != &rhs)
        {
            detail::container_destroy_reverse<T>(_data, _size);
            // ensure enough memory has been allocated
            if (_capacity < rhs._size)
            {
                _free(_data);
                _data = _alloc(rhs._size);
                _capacity = rhs._size;
            }
            detail::container_copy_range<T>(rhs._data, rhs._size, _data);
            _size = rhs._size;
        }
        return *this;
    }
    vector& operator=(vector&& rhs) noexcept
    {
        detail::container_destroy_reverse<T>(_data, _size);
        _free(_data);
        _data = rhs._data;
        _size = rhs._size;
        _capacity = rhs._capacity;
        rhs._data = nullptr;
        rhs._size = 0;
        rhs._capacity = 0;
        return *this;
    }

    // methods
public:
    /// creates a new element at the end (with the given constructor arguments)
    template <class... Args>
    T& emplace_back(Args&&... args)
    {
        if (_size == _capacity)
        {
            auto const new_cap = _capacity == 0 ? 1 : _capacity << 1;
            T* new_data = _alloc(new_cap);
            T* new_element = new (placement_new, &new_data[_size]) T(cc::forward<Args>(args)...);
            detail::container_move_range<T>(_data, _size, new_data);
            detail::container_destroy_reverse<T>(_data, _size);
            _free(_data);
            _data = new_data;
            _capacity = new_cap;
            _size++;
            return *new_element;
        }

        return *(new (placement_new, &_data[_size++]) T(cc::forward<Args>(args)...));
    }

    /// adds an element at the end
    T& push_back(T const& value) { return emplace_back(value); }
    /// adds an element at the end
    T& push_back(T&& value) { return emplace_back(cc::move(value)); }

    /// removes the last element
    void pop_back()
    {
        CC_CONTRACT(_size > 0);
        --_size;
        _data[_size].~T();
    }

    void reserve(size_t size)
    {
        if (size <= _capacity)
            return;
        T* new_data = _alloc(size);
        detail::container_move_range<T>(_data, _size, new_data);
        detail::container_destroy_reverse<T>(_data, _size);
        _free(_data);
        _data = new_data;
        _capacity = size;
    }

    void resize(size_t new_size)
    {
        if (new_size > _capacity)
            reserve(new_size);
        for (size_t i = _size; i < new_size; ++i)
            new (placement_new, &_data[i]) T();
        detail::container_destroy_reverse<T>(_data, _size, new_size);
        _size = new_size;
    }

    void resize(size_t new_size, T const& default_value)
    {
        if (new_size > _capacity)
            reserve(new_size);
        for (size_t i = _size; i < new_size; ++i)
            new (placement_new, &_data[i]) T(default_value);
        detail::container_destroy_reverse<T>(_data, _size, new_size);
        _size = new_size;
    }

    /// delete all stored elements
    /// does NOT deallocate internal memory
    void clear()
    {
        detail::container_destroy_reverse<T>(_data, _size);
        _size = 0;
    }

    /// ensures that _capacity == _size (without changing elements)
    void shrink_to_fit()
    {
        if (_size != _capacity)
        {
            T* new_data = _alloc(_size);
            detail::container_move_range<T>(_data, _size, new_data);
            _free(_data);
            _data = new_data;
            _capacity = _size;
        }
    }

    /// removes all entries where cc::invoke(pred, entry) is true
    /// returns the number of removed entries
    template <class Predicate>
    size_t remove_all(Predicate&& pred)
    {
        size_t idx = 0;
        for (size_t i = 0; i < _size; ++i)
            if (!cc::invoke(pred, _data[i]))
            {
                if (idx != i)
                    _data[idx] = cc::move(_data[i]);
                ++idx;
            }
        detail::container_destroy_reverse<T>(_data, _size, idx);
        auto old_size = _size;
        _size = idx;
        return old_size - _size;
    }

    /// removes the first entry where cc::invoke(pred, entry) is true
    /// returns true iff any element was removed
    template <class Predicate>
    bool remove_first(Predicate&& pred)
    {
        for (size_t i = 0; i < _size; ++i)
            if (cc::invoke(pred, _data[i]))
            {
                for (size_t j = i + 1; j < _size; ++j)
                    _data[j - 1] = cc::move(_data[j]);
                --_size;
                _data[_size].~T();
                return true;
            }
        return false;
    }

    /// remove all entries that are == value
    /// returns the number of removed entries
    template <class U = T>
    size_t remove(U const& value)
    {
        return remove_all([&](T const& v) { return v == value; });
    }

    /// removes a range (start + count) of elements
    /// count == 0 is allowed and a no-op
    void remove_range(size_t idx, size_t cnt)
    {
        if (cnt == 0)
            return;

        CC_CONTRACT(idx < _size);
        CC_CONTRACT(idx + cnt <= _size);

        for (size_t i = idx; i < _size - cnt; ++i)
            _data[i] = cc::move(_data[i + cnt]);
        detail::container_destroy_reverse<T>(_data, _size, _size - cnt);
        _size -= cnt;
    }

    /// removes the element at the given index
    void remove_at(size_t idx)
    {
        CC_CONTRACT(idx < _size);
        for (size_t i = idx + 1; i < _size; ++i)
            _data[i - 1] = cc::move(_data[i]);
        --_size;
        _data[_size].~T();
    }

    /// returns true iff any entry is == value
    template <class U = T>
    bool contains(U const& value) const
    {
        for (size_t i = 0; i < _size; ++i)
            if (_data[i] == value)
                return true;
        return false;
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

    // helpers
private:
    static T* _alloc(size_t size) { return reinterpret_cast<T*>(new std::byte[size * sizeof(T)]); }
    static void _free(T* p) { delete[] reinterpret_cast<std::byte*>(p); }

    // members
private:
    T* _data = nullptr;
    size_t _size = 0;
    size_t _capacity = 0;
};

// hash
template <class T>
struct hash<vector<T>>
{
    [[nodiscard]] constexpr hash_t operator()(vector<T> const& a) const noexcept
    {
        size_t h = 0;
        for (auto const& v : a)
            h = cc::hash_combine(h, hash<T>{}(v));
        return h;
    }
};
}
