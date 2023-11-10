#pragma once

#include <cstdint>

#include <clean-core/detail/vector_base.hh>

namespace cc
{
template <class T>
struct vector : detail::vector_base<T, size_t, false>
{
    static_assert(!std::is_const_v<T>, "vector of const objects is not allowed");

    // ctors
public:
    vector() = default;

    explicit vector(size_t size) : detail::vector_base<T, size_t, false>(this->_alloc(size), size, size)
    {
        detail::container_default_construct_or_zeroed(size, this->_data);
    }

    /// returns a vector with "size" elements that are default-initialized
    [[nodiscard]] static vector defaulted(size_t size) { return vector(size); }

    /// returns a vector with "size" elements that are uninitialized
    /// CAUTION: for non-pod types, you have to know what you're doing (i.e. placement new yourself)
    [[nodiscard]] static vector uninitialized(size_t size)
    {
        vector v;
        v._size = size;
        v._capacity = size;
        v._data = v._alloc(size);
        return v;
    }

    /// returns a vector with "size" elements all initialized with "value"
    [[nodiscard]] static vector filled(size_t size, T const& value)
    {
        vector v;
        v.resize(size, value);
        return v;
    }

    /// returns a vector with "size" elements reserved, but size is actually 0
    [[nodiscard]] static vector reserved(size_t size)
    {
        vector v;
        v.reserve(size);
        return v;
    }

    explicit vector(T const* begin, size_t num_elements)
    {
        this->reserve(num_elements);
        detail::container_copy_construct_range<T>(begin, num_elements, this->_data);
        this->_size = num_elements;
    }

    explicit vector(cc::span<T const> data) : vector(data.begin(), data.size()) {}

    template <class Range, cc::enable_if<cc::is_any_range<Range>> = true>
    explicit vector(Range const& range)
    {
        this->push_back_range(range);
    }

    vector(std::initializer_list<T> data) : vector(data.begin(), data.size()) {}

    vector(vector const& rhs) : vector(rhs.begin(), rhs.size()) {}

    vector(vector&& rhs) noexcept
    {
        this->_data = rhs._data;
        this->_size = rhs._size;
        this->_capacity = rhs._capacity;
        rhs._data = nullptr;
        rhs._size = 0;
        rhs._capacity = 0;
    }
    ~vector()
    {
        detail::container_destroy_reverse<T>(this->_data, this->_size);
        this->_free(this->_data);
    }
    vector& operator=(vector const& rhs)
    {
        if (this != &rhs)
        {
            detail::container_destroy_reverse<T>(this->_data, this->_size);
            // ensure enough memory has been allocated
            if (this->_capacity < rhs._size)
            {
                this->_free(this->_data);
                this->_data = this->_alloc(rhs._size);
                this->_capacity = rhs._size;
            }
            detail::container_copy_construct_range<T>(rhs._data, rhs._size, this->_data);
            this->_size = rhs._size;
        }
        return *this;
    }
    vector& operator=(vector&& rhs) noexcept
    {
        detail::container_destroy_reverse<T>(this->_data, this->_size);
        this->_free(this->_data);
        this->_data = rhs._data;
        this->_size = rhs._size;
        this->_capacity = rhs._capacity;
        rhs._data = nullptr;
        rhs._size = 0;
        rhs._capacity = 0;
        return *this;
    }

    /// moves data from this vector into a newly create vector of a different type
    /// only works for trivial types where U is less restrictive than T
    /// e.g.
    ///   vector<pos3> v = ...;
    ///   auto bytes = cc::move(v).reinterpret_as<std::byte>();
    /// NOTE: the move is necessary because this will be moved-from afterwards
    template <class U>
    vector<U> reinterpret_as() &&
    {
        static_assert(std::is_trivially_copyable_v<T>, "requires memcpy-safe types");
        static_assert(std::is_trivially_copyable_v<U>, "requires memcpy-safe types");
        static_assert(alignof(U) <= alignof(T), "cannot increase alignment requirements");
        static_assert(sizeof(T) % sizeof(U) == 0, "can only cast to types that evenly divide the source type");

        vector<U> v;

        v._data = reinterpret_cast<U*>(this->_data);
        v._size = this->_size * (sizeof(T) / sizeof(U));
        v._capacity = this->_capacity * (sizeof(T) / sizeof(U));

        this->_data = nullptr;
        this->_size = 0;
        this->_capacity = 0;

        return v;
    }

    template <class U>
    friend struct vector;
};

// hash
template <class T>
struct hash<vector<T>>
{
    [[nodiscard]] constexpr uint64_t operator()(vector<T> const& a) const noexcept
    {
        uint64_t h = 0;
        for (auto const& v : a)
            h = cc::hash_combine(h, hash<T>{}(v));
        return h;
    }
};
} // namespace cc
