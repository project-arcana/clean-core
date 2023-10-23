#pragma once

#include <cstddef> // std::byte
#include <cstdlib> // malloc/free
#include <cstring> // std::memcpy
#include <initializer_list>
#include <type_traits>

#include <clean-core/allocator.hh>
#include <clean-core/allocators/system_allocator.hh>
#include <clean-core/assert.hh>
#include <clean-core/collection_traits.hh>
#include <clean-core/detail/container_impl_util.hh>
#include <clean-core/forward.hh>
#include <clean-core/fwd.hh>
#include <clean-core/hash_combine.hh>
#include <clean-core/invoke.hh>
#include <clean-core/is_contiguous_range.hh>
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
    T* _realloc(T* p, size_t size)
    {
        static_assert(std::is_trivially_copyable_v<T> && std::is_trivially_destructible_v<T>, "realloc not permitted for this type");
        return reinterpret_cast<T*>(_allocator->realloc(p, size * sizeof(T), alignof(T)));
    }
    cc::allocator* _allocator = nullptr;
    constexpr explicit vector_internals_with_allocator(cc::allocator* alloc) : _allocator(alloc) {}
};
template <class T>
struct vector_internals
{
    T* _alloc(size_t size) { return reinterpret_cast<T*>(cc::system_malloc(size * sizeof(T), alignof(T))); }
    void _free(T* p) { cc::system_free(p); }
    T* _realloc(T* p, size_t size)
    {
        static_assert(std::is_trivially_copyable_v<T> && std::is_trivially_destructible_v<T>, "realloc not permitted for this type");
        return reinterpret_cast<T*>(cc::system_realloc(p, size * sizeof(T), alignof(T)));
    }
};


template <class T, class IndexT, bool HasAllocator>
struct vector_base : protected std::conditional_t<HasAllocator, detail::vector_internals_with_allocator<T>, detail::vector_internals<T>>
{
    using index_t = IndexT;

    // properties
public:
    size_t size() const { return _size; }
    size_t size_bytes() const { return _size * sizeof(T); }
    size_t capacity() const { return _capacity; }
    size_t capacity_remaining() const { return _capacity - _size; }
    bool empty() const { return _size == 0; }
    bool at_capacity() const { return capacity_remaining() == 0; }
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

    T& operator[](index_t i)
    {
        CC_CONTRACT(size_t(i) < _size);
        return _data[size_t(i)];
    }
    T const& operator[](index_t i) const
    {
        CC_CONTRACT(size_t(i) < _size);
        return _data[size_t(i)];
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

            if constexpr (std::is_trivially_copyable_v<T> && sizeof(T) <= 512)
            {
                // we can use realloc (size limit to keep stack usage limited)
                // temporary object required because the arg could reference memory inside this buffer
                auto tmp_obj = T(cc::forward<Args>(args)...);
                _data = this->_realloc(_data, new_cap);
                CC_ASSERT(cc::is_aligned(_data, alignof(T)));
                T* new_element = new (placement_new, &_data[_size]) T(cc::move(tmp_obj));
                _capacity = new_cap;
                _size++;
                return *new_element;
            }
            else
            {
                // we can't use realloc, use separate alloc/free calls
                T* new_data = this->_alloc(new_cap);
                T* new_element = new (placement_new, &new_data[_size]) T(cc::forward<Args>(args)...);
                detail::container_move_construct_range<T>(_data, _size, new_data);
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
    T& push_back(T const& value)
    {
        static_assert(std::is_copy_constructible_v<T>, "only works with copyable types. did you forget a cc::move?");
        return this->emplace_back(value);
    }
    /// adds an element at the end
    T& push_back(T&& value) { return this->emplace_back(cc::move(value)); }

    /// creates a new element at the end without growing
    template <class... Args>
    CC_FORCE_INLINE T& emplace_back_stable(Args&&... args)
    {
        CC_ASSERT(_size < _capacity && "At capacity");
        return *(new (placement_new, &_data[_size++]) T(cc::forward<Args>(args)...));
    }

    /// adds all elements of the range
    template <class Range>
    void push_back_range(Range&& range)
    {
        static_assert(cc::is_any_range<Range>);
        static_assert(std::is_copy_constructible_v<T>, "only works with copyable types. use push_back(T&&) to move elements into the vector");

        if constexpr (cc::is_contiguous_range<Range, T>)
        {
            size_t const num_new_elems = cc::collection_size(range);
            if (num_new_elems == 0)
            {
                return;
            }

            // NOTE: this would assert for an empty span<T>
            // only get this pointer after checking for size == 0
            this->push_back_range_n(&range[0], num_new_elems);
        }
        else
        {
            if constexpr (collection_traits<Range>::has_size)
                this->reserve(_size + cc::collection_size(range));

            for (auto&& v : range)
                this->push_back(v);
        }
    }

    /// adds an element at the given index, moves
    /// NOTE: currently, value MUST NOT point into this vector
    /// TODO: remove this restriction
    /// TODO: move/emplace version
    void insert_at(index_t index, T const& value) { insert_range_at(index, cc::span<T const>(value)); }

    /// NOTE: currently, values MUST NOT point into this vector
    /// TODO: remove this restriction
    /// TODO: generic range version
    void insert_range_at(index_t index, cc::span<T const> values)
    {
        CC_CONTRACT(size_t(index) <= _size);

        size_t const num_new_elems = values.size();
        if (num_new_elems == 0)
            return;

        reserve(_size + num_new_elems);

        T* const data = _data + size_t(index);
        detail::container_relocate_construct_range<T>(data + num_new_elems, data, _size - size_t(index));

        detail::container_copy_construct_range<T>(values.data(), num_new_elems, data);
        _size += num_new_elems;
    }

    /// removes the last element
    void pop_back()
    {
        CC_CONTRACT(_size > 0);
        --_size;
        _data[_size].~T();
    }

    void reserve(size_t size)
    {
        CC_CONTRACT(size <= (1uLL << 48) && "trying to allocate too much memory");

        if (size <= _capacity)
            return;

        // at least double cap
        auto new_cap = _capacity << 1;
        if (new_cap < size)
            new_cap = size;

        if constexpr (std::is_trivially_copyable_v<T> && std::is_trivially_destructible_v<T>)
        {
            // we can use realloc
            _data = this->_realloc(_data, new_cap);
            _capacity = new_cap;
        }
        else
        {
            // we can't use realloc, call alloc/free separately
            T* new_data = this->_alloc(new_cap);
            detail::container_move_construct_range<T>(_data, _size, new_data);
            detail::container_destroy_reverse<T>(_data, _size);
            this->_free(_data);
            _data = new_data;
            _capacity = new_cap;
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

    /// CAUTION: currently default_value must not be an interior reference
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
                _data = this->_realloc(_data, _size);
                _capacity = _size;
            }
            else
            {
                // we can't
                T* new_data = this->_alloc(_size);
                detail::container_move_construct_range<T>(_data, _size, new_data);
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
                this->remove_at(i);
                return true;
            }
        return false;
    }

    /// removes the first entry where cc::invoke(pred, entry) is true without preserving order
    /// returns true iff any element was removed
    template <class Predicate>
    bool remove_first_unordered(Predicate&& pred)
    {
        for (size_t i = 0; i < _size; ++i)
            if (cc::invoke(pred, _data[i]))
            {
                this->remove_at_unordered(i);
                return true;
            }
        return false;
    }

    /// removes the first entry that is == value without preserving order
    /// returns true iff any element was removed
    bool remove_first_value_unordered(T value)
    {
        return remove_first_unordered([&](T const& v) { return v == value; });
    }

    /// remove all entries that are == value
    /// returns the number of removed entries
    /// NOTE: the argument is taken per value
    ///       this ensures correct behavior in cases like "v.remove(v[10])"
    size_t remove_value(T value)
    {
        return remove_all([&](T const& v) { return v == value; });
    }
    [[deprecated("renamed to remove_value due to interior reference issues")]] size_t remove(T value) { return remove_value(cc::move(value)); }

    /// removes a range (start + count) of elements
    /// count == 0 is allowed and a no-op
    void remove_range(index_t idx, size_t cnt)
    {
        if (cnt == 0)
            return;

        CC_CONTRACT(size_t(idx) < _size);
        CC_CONTRACT(size_t(idx) + cnt <= _size);

        for (size_t i = size_t(idx); i < _size - cnt; ++i)
            _data[i] = cc::move(_data[i + cnt]);
        detail::container_destroy_reverse<T>(_data, _size, _size - cnt);
        _size -= cnt;
    }

    /// removes the element at the given index
    void remove_at(index_t idx)
    {
        CC_CONTRACT(idx < _size);
        for (size_t i = size_t(idx) + 1; i < _size; ++i)
            _data[i - 1] = cc::move(_data[i]);
        --_size;
        _data[_size].~T();
    }

    /// removes the element at the given index without preserving order
    void remove_at_unordered(index_t idx)
    {
        CC_CONTRACT(size_t(idx) < _size);
        cc::swap(_data[size_t(idx)], this->back());
        this->pop_back();
    }

    void fill_memzero()
    {
        static_assert(std::is_trivially_copyable_v<T>, "Can only memzero trivial types");
        memset(_data, 0, size_bytes());
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

    bool operator==(span<T const> rhs) const noexcept
    {
        if (_size != rhs.size())
            return false;
        for (size_t i = 0; i < _size; ++i)
            if (!(_data[i] == rhs[i]))
                return false;
        return true;
    }

    bool operator!=(span<T const> rhs) const noexcept
    {
        if (_size != rhs.size())
            return true;
        for (size_t i = 0; i < _size; ++i)
            if (_data[i] != rhs[i])
                return true;
        return false;
    }

    bool operator==(vector_base const& rhs) const noexcept { return operator==(span<T const>(rhs)); }
    bool operator!=(vector_base const& rhs) const noexcept { return operator!=(span<T const>(rhs)); }

public:
    void push_back_range_n(T const* data, size_t num)
    {
        if (!data || !num)
            return;

        reserve(_size + num);
        detail::container_copy_construct_range<T>(data, num, &_data[_size]);
        _size += num;
    }

    // members
protected:
    vector_base() = default;
    // non-alloc
    explicit constexpr vector_base(T* data, size_t size, size_t cap) noexcept : _data(data), _size(size), _capacity(cap)
    {
        static_assert(!HasAllocator, "wrong ctor");
    }
    // alloc
    explicit constexpr vector_base(cc::allocator* alloc) noexcept : vector_internals_with_allocator<T>(alloc)
    {
        static_assert(HasAllocator, "wrong ctor");
    }
    T* _data = nullptr;
    size_t _size = 0;
    size_t _capacity = 0;
};
} // namespace cc::detail
