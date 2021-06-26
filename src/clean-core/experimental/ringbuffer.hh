#pragma once

#include <clean-core/allocator.hh>
#include <clean-core/assert.hh>
#include <clean-core/bits.hh>
#include <clean-core/forward.hh>
#include <clean-core/macros.hh>
#include <clean-core/move.hh>
#include <clean-core/new.hh>

namespace cc
{
/// a ringbuffer that permits efficient push/pop from both ends
/// implemented as a contiguous buffer that wraps around
/// uses power-of-two sizes for efficiency
///
/// TODO: try to use realloc?
///       use memcpy for some operations
///       (both not trivial as data might wrap around)
///
/// TODO: - shrink_to_fit
///       - reserve
///
/// This implementation is quite efficient: https://godbolt.org/z/zYPrPcT8K
template <class T>
struct ringbuffer
{
    // ctors
public:
    explicit ringbuffer(cc::allocator* allocator = cc::system_allocator) : _allocator(allocator) { CC_CONTRACT(allocator != nullptr); }

    ringbuffer(ringbuffer&& rhs) noexcept
    {
        _begin = rhs._begin;
        _end = rhs._end;
        _mask = rhs._mask;
        _data = rhs._data;
        _allocator = rhs._allocator;

        rhs._begin = 0;
        rhs._end = 0;
        rhs._mask = 0;
        rhs._data = nullptr;
        rhs._allocator = cc::system_allocator;
    }
    ringbuffer& operator=(ringbuffer&& rhs) noexcept
    {
        if constexpr (!std::is_trivially_destructible_v<T>)
        {
            auto const s = size();
            for (size_t i = 0; i < s; ++i)
                _data[(_begin + i) & _mask].~T();
        }
        this->_free(_data);

        _begin = rhs._begin;
        _end = rhs._end;
        _mask = rhs._mask;
        _data = rhs._data;
        _allocator = rhs._allocator;

        rhs._begin = 0;
        rhs._end = 0;
        rhs._mask = 0;
        rhs._data = nullptr;
        rhs._allocator = cc::system_allocator;

        return *this;
    }

    ringbuffer(ringbuffer const& rhs)
    {
        _allocator = rhs._allocator;
        _begin = 0;
        _end = rhs.size();
        _mask = rhs._mask;
        _data = _alloc(_mask + 1);
        auto const s = size();
        for (size_t i = 0; i < s; ++i)
            new (placement_new, &_data[i]) T(rhs._data[(rhs._begin + i) & rhs._mask]);
    }

    // copy assignment is deleted because it's unclear which allocator to use
    ringbuffer& operator=(ringbuffer const& rhs) = delete;

    ~ringbuffer()
    {
        static_assert(sizeof(T) > 0, "ringbuffer destructor requires complete type");
        if constexpr (!std::is_trivially_destructible_v<T>)
        {
            auto const s = size();
            for (size_t i = 0; i < s; ++i)
                _data[(_begin + i) & _mask].~T();
        }
        this->_free(_data);
    }

    // container API
public:
    size_t size() const { return (_end - _begin) & _mask; }
    size_t capacity() const { return _mask; }
    size_t capacity_remaining() const { return _mask - size(); }
    bool empty() const { return _begin == _end; }
    bool at_capacity() const { return ((_end - _begin + 1) & _mask) == 0; }

    T& front()
    {
        CC_CONTRACT(!empty());
        return _data[_begin];
    }
    T const& front() const
    {
        CC_CONTRACT(!empty());
        return _data[_begin];
    }

    T& back()
    {
        CC_CONTRACT(!empty());
        return _data[(_end - 1) & _mask];
    }
    T const& back() const
    {
        CC_CONTRACT(!empty());
        return _data[(_end - 1) & _mask];
    }

    T& operator[](size_t i)
    {
        CC_CONTRACT(i < size());
        return _data[(_begin + i) & _mask];
    }
    T const& operator[](size_t i) const
    {
        CC_CONTRACT(i < size());
        return _data[(_begin + i) & _mask];
    }

    cc::allocator* allocator() const { return _allocator; }

    // methods
public:
    /// creates a new element at the end (with the given constructor arguments)
    /// returns a reference to the newly created element
    template <class... Args>
    T& emplace_back(Args&&... args)
    {
        auto const pos = _end;
        auto const new_end = (_end + 1) & _mask;

        if (CC_LIKELY(new_end != _begin))
        {
            _end = new_end;
            return *(new (placement_new, &_data[pos]) T(cc::forward<Args>(args)...));
        }
        else
        {
            return emplace_back_grow(cc::forward<Args>(args)...);
        }
    }
    /// same as emplace_back but asserts that no growing is necessary
    template <class... Args>
    T& emplace_back_stable(Args&&... args)
    {
        CC_CONTRACT(!at_capacity());
        auto pos = _end;
        _end = (_end + 1) & _mask;
        return *(new (placement_new, &_data[pos]) T(cc::forward<Args>(args)...));
    }

    /// adds an element at the end
    T& push_back(T const& value) { return emplace_back(value); }
    /// adds an element at the end
    T& push_back(T&& value) { return emplace_back(cc::move(value)); }

    /// removes the front-most element and returns it
    T pop_back()
    {
        CC_CONTRACT(!empty());
        _end = (_end - 1) & _mask;
        auto& v = _data[_end];
        T r = cc::move(v);
        v.~T();
        return r;
    }

    /// creates a new element at the start (with the given constructor arguments)
    /// returns a reference to the newly created element
    template <class... Args>
    T& emplace_front(Args&&... args)
    {
        auto new_begin = (_begin - 1) & _mask;
        if (CC_LIKELY(new_begin != _end))
        {
            _begin = new_begin;
            return *(new (placement_new, &_data[new_begin]) T(cc::forward<Args>(args)...));
        }
        else
        {
            return emplace_front_grow(cc::forward<Args>(args)...);
        }
    }
    /// same as emplace_front but asserts that no growing is necessary
    template <class... Args>
    T& emplace_front_stable(Args&&... args)
    {
        CC_CONTRACT(!at_capacity());
        _begin = (_begin - 1) & _mask;
        return *(new (placement_new, &_data[_begin]) T(cc::forward<Args>(args)...));
    }

    /// adds an element at the front
    T& push_front(T const& value) { return emplace_front(value); }
    /// adds an element at the front
    T& push_front(T&& value) { return emplace_front(cc::move(value)); }

    /// removes the front-most element and returns it
    T pop_front()
    {
        CC_CONTRACT(!empty());
        auto& v = _data[_begin];
        T r = cc::move(v);
        v.~T();
        _begin = (_begin + 1) & _mask;
        return r;
    }

    void clear()
    {
        if constexpr (!std::is_trivially_destructible_v<T>)
        {
            auto const s = size();
            for (size_t i = 0; i < s; ++i)
                _data[(_begin + i) & _mask].~T();
        }
        _begin = 0;
        _end = 0;
    }

    // helper
private:
    template <class... Args>
    CC_COLD_FUNC T& emplace_back_grow(Args&&... args)
    {
        CC_ASSERT(at_capacity());

        auto const new_mask = _mask == 0 ? 0b11 : (_mask << 1) + 1;
        auto const new_cap = new_mask + 1;
        auto const s = size();

        T* new_data = this->_alloc(new_cap);
        T* new_element = new (placement_new, &new_data[s]) T(cc::forward<Args>(args)...);
        for (size_t i = 0; i < s; ++i)
        {
            auto& v = _data[(_begin + i) & _mask];
            new (placement_new, &new_data[i]) T(cc::move(v));
            v.~T();
        }
        this->_free(_data);
        _data = new_data;
        _mask = new_mask;
        _begin = 0;
        _end = s + 1;
        return *new_element;
    }
    template <class... Args>
    CC_COLD_FUNC T& emplace_front_grow(Args&&... args)
    {
        CC_ASSERT(at_capacity());

        auto const new_mask = _mask == 0 ? 0b11 : (_mask << 1) + 1;
        auto const new_cap = new_mask + 1;
        auto const s = size();

        T* new_data = this->_alloc(new_cap);
        T* new_element = new (placement_new, &new_data[0]) T(cc::forward<Args>(args)...);
        for (size_t i = 0; i < s; ++i)
        {
            auto& v = _data[(_begin + i) & _mask];
            new (placement_new, &new_data[i + 1]) T(cc::move(v));
            v.~T();
        }
        this->_free(_data);
        _data = new_data;
        _mask = new_mask;
        _begin = 0;
        _end = s + 1;
        return *new_element;
    }

    T* _alloc(size_t size)
    {
        CC_ASSERT(_allocator && "no allocator set?");
        return reinterpret_cast<T*>(_allocator->alloc(size * sizeof(T), alignof(T)));
    }
    void _free(T* p)
    {
        CC_ASSERT(_allocator && "no allocator set?");
        _allocator->free(p);
    }

private:
    size_t _begin = 0; // points to first valid entry, IF non-empty
    size_t _end = 0;   // points BEHIND last valid entry (or == _begin if empty)
    size_t _mask = 0;  // "_mask + 1" is size, "& _mask" wraps around ringbuffer
    T* _data = nullptr;
    cc::allocator* _allocator = nullptr;
};
}
