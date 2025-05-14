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

// NOTE: does NOT delete
template <class T>
struct deferred_dtor_call
{
    T& v;

    ~deferred_dtor_call() { v.~T(); }
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
        CC_ASSERT(_size < _capacity && "must have remaining capacity");
        return *(new (placement_new, &_data[_size++]) T(cc::forward<Args>(args)...));
    }

    /// adds all elements of the range
    /// NOTE: explicitly supports ranges that are in any way subviews or dependent on "this"
    ///       in particular, this->push_back_range(*this) is safe to do
    /// NOTE: this currently always calls the copy ctors because lvalue-ness of range is not a good indicator for ownership
    template <class Range>
    void push_back_range(Range&& range)
    {
        static_assert(cc::is_any_range<Range>);
        static_assert(std::is_copy_constructible_v<T>, "only works with copyable types. use push_back(T&&) to move elements into the vector");

        if constexpr (cc::is_contiguous_range<Range, T const>)
        {
            size_t const additional_size = cc::collection_size(range);
            if (additional_size == 0)
                return;

            // NOTE: this would assert for an empty span<T>
            //       only get this pointer after checking for size == 0
            // NOTE: this uses optimized realloc for trivial types
            this->push_back_range_n(&range[0], additional_size);
        }
        else
        {
            // everything allocated in _data is not moved until the very end
            // this allows range to arbitrarily reference into the old data
            // the number of elements that are moved at the end is "frozen_size"
            T* curr_data = _data;
            auto is_curr_newly_allocated = false;
            size_t curr_cap = _capacity;
            size_t curr_size = _size;
            size_t frozen_size = _size;

            // in case we need to grow, we cannot simply reserve
            // because the range might point into "this" (possibly obscurely) and therefore invalidate during reserve
            // thus, we alloc new data, copy-construct into the appropriate space
            //       (potentially multiple times during the simulated push-back)
            //       and only THEN free the old data
            // NOTE: this path never uses realloc
            //       because this might invalidate the range we're iterating over
            // NOTE: we should keep the old properties of "this" until the iteration is over

            // use collection size as a hint
            if constexpr (collection_traits<Range>::has_size)
            {
                auto min_new_size = _size + cc::collection_size(range);
                if (min_new_size > curr_cap)
                {
                    // adjust capacity
                    curr_cap <<= 1;
                    if (curr_cap < min_new_size)
                        curr_cap = min_new_size;

                    // do initial alloc
                    // IMPORTANT: the slots for pre-existing data stay uninitialized until the end
                    curr_data = this->_alloc(curr_cap);
                    is_curr_newly_allocated = true;

                    // NOTE: we don't move anything yet
                }
            }

            // for a general range, we need to iterate each element
            // NOTE: v must not necessarily be of type T
            //       we use auto&& to have whatever type is most appropriate, including values
            for (auto&& v : range)
            {
                // if we need to resize
                if (curr_size >= curr_cap)
                {
                    // compute new cap
                    // at least double, subject to some minimums
                    curr_cap <<= 1;
                    if (curr_cap <= curr_size)
                        curr_cap++;

                    // alloc new data
                    // IMPORTANT: the slots for pre-existing data stay uninitialized until the end
                    auto new_data = this->_alloc(curr_cap);

                    // move over data that we pushed into the vector
                    // IMPORTANT: all pre-existing data is kept in the old vector for now to keep them stable
                    detail::container_move_construct_range<T>(curr_data + frozen_size, //
                                                              curr_size - frozen_size, //
                                                              new_data + frozen_size);

                    // if we already had temporary allocated data (not the old one)
                    // we need to destroy + free it
                    if (is_curr_newly_allocated)
                    {
                        detail::container_destroy_reverse<T>(curr_data + frozen_size,  //
                                                             curr_size - frozen_size); //
                        this->_free(curr_data);
                    }

                    // keep track of temporarily allocated data
                    curr_data = new_data;
                    is_curr_newly_allocated = true;
                }

                // NOTE: this always calls the copy ctor
                //       because lvalue-ness of range is not a good indicator for ownership
                CC_ASSERT(curr_size < curr_cap);
                new (placement_new, &curr_data[curr_size]) T(v);
                ++curr_size;

                // if we are still in the original data block, increase the frozen "high water mark"
                if (!is_curr_newly_allocated)
                    ++frozen_size;
            }

            // if we have newly allocated data, we also need to move over the old data and free the memory
            if (is_curr_newly_allocated)
            {
                CC_ASSERT(_data != curr_data);
                detail::container_move_construct_range<T>(_data, frozen_size, curr_data);
                detail::container_destroy_reverse<T>(_data, frozen_size);
                this->_free(_data);

                _data = curr_data;
                _capacity = curr_cap;
            }
            else
                CC_ASSERT(_capacity == curr_cap && "no-alloc path should never modify capacity");

            // always write through new size
            _size = curr_size;
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
    /// NOTE: use get_and_pop_back if you need the value
    void pop_back()
    {
        CC_CONTRACT(_size > 0);
        --_size;
        _data[_size].~T();
    }
    /// removes the last element and returns it
    /// NOTE: if the return value is unused AND move is expensive and not inlined, then this has some overhead
    ///       see https://godbolt.org/z/ev3qa5e66
    ///       it's still as efficient or more as doing it manually
    T get_and_pop_back()
    {
        CC_CONTRACT(_size > 0);
        --_size;
        auto _ = cc::detail::deferred_dtor_call<T>{_data[_size]};
        return cc::move(_data[_size]);
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

    /// extends or shrinks the vector to the new size
    /// newly created entries are default constructed
    void resize(size_t new_size)
    {
        if (new_size > _capacity)
            reserve(new_size);
        for (size_t i = _size; i < new_size; ++i)
            new (placement_new, &_data[i]) T();
        detail::container_destroy_reverse<T>(_data, _size, new_size);
        _size = new_size;
    }

    /// extends or shrinks the vector to the new size
    /// newly created entries are copy-constructed from the provided value
    /// CAUTION: currently default_value must not be an interior reference
    void resize(size_t new_size, T const& default_value)
    {
        CC_ASSERT(!is_interior_reference(default_value) && "must not use an interior reference as default value");

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

    /// removes all entries where cc::invoke(pred, entry) is true in O(n)
    /// returns the number of removed entries
    /// NOTE: is guaranteed to call pred exactly once for each element in order
    /// NOTE: see move_all_to if you want to retain the removed elements
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

    /// removes all entries where cc::invoke(pred, idx) is true in O(n)
    /// returns the number of removed entries
    /// NOTE: is guaranteed to call pred exactly once for each index in order
    /// NOTE: see move_all_to_by_idx if you want to retain the removed elements
    template <class Predicate>
    size_t remove_all_by_idx(Predicate&& pred)
    {
        size_t idx = 0;
        for (size_t i = 0; i < _size; ++i)
            if (!cc::invoke(pred, i))
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

    /// sets the whole vector to zero bytewise using memset
    /// NOTE: only works on trivially copyable types
    void fill_memzero()
    {
        static_assert(std::is_trivially_copyable_v<T>, "Can only memzero trivial types");
        memset(_data, 0, size_bytes());
    }

    /// reverses the position of all elements using cc::swap
    void reverse()
    {
        if (_size == 0)
            return;

        auto p_front = _data;
        auto p_back = _data + _size - 1;
        while (p_front < p_back)
            cc::swap(*p_front++, *p_back--);
    }

    /// moves all entries where cc::invoke(pred, entry) is true to container in O(n)
    /// returns the number of moved entries
    /// NOTE: is guaranteed to call pred exactly once for each index in order
    template <class Predicate, class Container>
    size_t move_all_to(Container& container, Predicate&& pred)
    {
        size_t idx = 0;
        for (size_t i = 0; i < _size; ++i)
            if (cc::invoke(pred, _data[i]))
            {
                cc::collection_add(container, cc::move(_data[i]));
            }
            else
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

    /// moves all entries where cc::invoke(pred, idx) is true to container in O(n)
    /// returns the number of moved entries
    /// NOTE: is guaranteed to call pred exactly once for each index in order
    template <class Predicate, class Container>
    size_t move_all_to_by_idx(Container& container, Predicate&& pred)
    {
        size_t idx = 0;
        for (size_t i = 0; i < _size; ++i)
            if (cc::invoke(pred, i))
            {
                cc::collection_add(container, cc::move(_data[i]));
            }
            else
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

    /// returns true iff any entry is == value
    template <class U = T>
    bool contains(U const& value) const
    {
        for (size_t i = 0; i < _size; ++i)
            if (_data[i] == value)
                return true;
        return false;
    }

    /// true iff the vector and the rhs span are content-equal
    bool operator==(span<T const> rhs) const noexcept
    {
        if (_size != rhs.size())
            return false;
        for (size_t i = 0; i < _size; ++i)
            if (!(_data[i] == rhs[i]))
                return false;
        return true;
    }

    /// true iff the vector and the rhs span are content-unequal
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
    /// optimized version of push_back_range for contiguous memory
    /// (this is a low-level building block that is sometimes useful from the outside and thus exposed)
    /// NOTE: nullptr data or zero count are explicitly allowed (and no-op)
    /// NOTE: explicitly supports ranges that are in any way subviews or dependent on "this"
    ///       in particular, this->push_back_range(*this) is safe to do
    void push_back_range_n(T const* data, size_t count)
    {
        if (data == nullptr || count == 0)
            return;

        // fast-path for trivially copyable types
        if constexpr (std::is_trivially_copyable_v<T> && std::is_trivially_destructible_v<T>)
        {
            // we try to use realloc here
            // and simply adjust the data pointer if it was moved
            auto const old_data_start = _data;
            auto const old_data_end = _data + _size;

            // uses potentially more efficient realloc
            reserve(_size + count);

            // adjust data pointer to point to the new memory region
            // NOTE: see is_interior_reference for a description of why we do integer comparisons
            if (size_t(old_data_start) <= size_t(data) && size_t(data) < size_t(old_data_end))
                data += _data - old_data_start;

            detail::container_copy_construct_range<T>(data, count, &_data[_size]);
            _size += count;
        }
        else
        {
            T* curr_data = _data;
            size_t curr_cap = _capacity;
            auto const new_size = _size + count;

            // in case we need to grow, we cannot simply reserve
            // because "data" might point into "this" and therefore invalidate during reserve
            // thus, we alloc new data, copy-construct into the appropriate space
            //       and only THEN free the old data
            // NOTE: non-trivial types cannot use realloc anyways
            if (new_size > _capacity)
            {
                // at least double cap
                curr_cap = _capacity << 1;
                if (curr_cap < new_size)
                    curr_cap = new_size;

                // freshly allocated data for the new capacity
                // NOTE: the new data is copied in first, THEN the old one
                curr_data = this->_alloc(curr_cap);
            }

            // copy data into new data region (which might be the old one or a newly allocated one)
            detail::container_copy_construct_range<T>(data, count, curr_data + _size);

            // if we reallocated, we can now safely move and free the old data over
            if (new_size > _capacity)
            {
                detail::container_move_construct_range<T>(_data, _size, curr_data);
                detail::container_destroy_reverse<T>(_data, _size);
                this->_free(_data);

                _data = curr_data;
                _capacity = curr_cap;
            }
            else
                CC_ASSERT(_capacity == curr_cap && "no-alloc path should never modify capacity");

            // size always changes
            _size = new_size;
            CC_ASSERT(_size <= _capacity);
        }
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

    /// returns true iff v is a value inside this vector
    /// NOTE: this is tricky: https://en.cppreference.com/w/cpp/language/operator_comparison#Built-in_pointer_equality_comparison
    /// NOTE: this is technically pointer comparison _unspecified_ if v is NOT inside the vector
    ///       we compare the integer versions though, which should be fine on a 64 bit unified memory system
    bool is_interior_reference(T const& v) const { return size_t(_data) <= size_t(&v) && size_t(&v) < size_t(_data + _capacity); }
};
} // namespace cc::detail
