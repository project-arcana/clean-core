#pragma once

#include <cstddef> // std::byte
#include <cstring> // std::memcpy
#include <initializer_list>
#include <type_traits>

#include <clean-core/allocator.hh>
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
#include <clean-core/utility.hh>

namespace cc::detail
{
template <class T>
struct vector_internals_with_allocator
{
    T* _alloc(size_t size) { return reinterpret_cast<T*>(_allocator->alloc(size * sizeof(T), alignof(T))); }
    void _free(T* p) { _allocator->free(p); }
    T* _realloc(T* p, size_t old_size, size_t size)
    {
        static_assert(std::is_trivially_copyable_v<T> && std::is_trivially_destructible_v<T>, "realloc not permitted for this type");
        return reinterpret_cast<T*>(_allocator->realloc(p, old_size * sizeof(T), size * sizeof(T), alignof(T)));
    }
    cc::allocator* _allocator = nullptr;
    constexpr explicit vector_internals_with_allocator(cc::allocator* alloc) : _allocator(alloc) {}
};
template <class T>
struct vector_internals
{
    T* _alloc(size_t size) { return reinterpret_cast<T*>(new std::byte[size * sizeof(T)]); }
    void _free(T* p) { delete[] reinterpret_cast<std::byte*>(p); }
    T* _realloc(T* p, size_t old_size, size_t size)
    {
        static_assert(std::is_trivially_copyable_v<T> && std::is_trivially_destructible_v<T>, "realloc not permitted for this type");
        T* res = nullptr;

        if (size > 0)
        {
            res = this->_alloc(size);

            if (p != nullptr)
            {
                std::memcpy(res, p, cc::min(old_size, size) * sizeof(T));
            }
        }

        this->_free(p);
        return res;
    }
};


template <class T, bool HasAllocator>
struct vector_base : protected std::conditional_t<HasAllocator, detail::vector_internals_with_allocator<T>, detail::vector_internals<T>>
{
    // properties
public:
    size_t size() const { return _size; }
    size_t size_bytes() const { return _size * sizeof(T); }
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

    // methods
public:
    /// creates a new element at the end (with the given constructor arguments)
    template <class... Args>
    T& emplace_back(Args&&... args)
    {
        if (_size == _capacity)
        {
            auto const new_cap = _capacity == 0 ? 1 : _capacity << 1;

            if constexpr (std::is_trivially_copyable_v<T> && std::is_trivially_destructible_v<T>)
            {
                // we can use realloc
                _data = this->_realloc(_data, _capacity, new_cap);
                T* new_element = new (placement_new, &_data[_size]) T(cc::forward<Args>(args)...);
                _capacity = new_cap;
                _size++;
                return *new_element;
            }
            else
            {
                // we can't
                T* new_data = this->_alloc(new_cap);
                T* new_element = new (placement_new, &new_data[_size]) T(cc::forward<Args>(args)...);
                detail::container_move_range<T>(_data, _size, new_data);
                detail::container_destroy_reverse<T>(_data, _size);
                this->_free(_data);
                _data = new_data;
                _capacity = new_cap;
                _size++;
                return *new_element;
            }
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

        if constexpr (std::is_trivially_copyable_v<T> && std::is_trivially_destructible_v<T>)
        {
            // we can use realloc
            _data = this->_realloc(_data, _capacity, size);
            _capacity = size;
        }
        else
        {
            // we can't
            T* new_data = this->_alloc(size);
            detail::container_move_range<T>(_data, _size, new_data);
            detail::container_destroy_reverse<T>(_data, _size);
            this->_free(_data);
            _data = new_data;
            _capacity = size;
        }
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
            if constexpr (std::is_trivially_copyable_v<T> && std::is_trivially_destructible_v<T>)
            {
                // we can use realloc
                _data = this->_realloc(_data, _capacity, _size);
                _capacity = _size;
            }
            else
            {
                // we can't
                T* new_data = this->_alloc(_size);
                detail::container_move_range<T>(_data, _size, new_data);
                this->_free(_data);
                _data = new_data;
                _capacity = _size;
            }
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

    // members
protected:
    vector_base() = default;
    // non-alloc
    explicit constexpr vector_base(T* data, size_t size, size_t cap) : _data(data), _size(size), _capacity(cap)
    {
        static_assert(!HasAllocator, "wrong ctor");
    }
    // alloc
    explicit constexpr vector_base(cc::allocator* alloc) : vector_internals_with_allocator<T>(alloc) { static_assert(HasAllocator, "wrong ctor"); }
    T* _data = nullptr;
    size_t _size = 0;
    size_t _capacity = 0;
};
}
