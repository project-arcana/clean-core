#pragma once

#include <cstdint>

#include <clean-core/detail/vector_base.hh>

namespace cc
{
template <class T>
struct vector : detail::vector_base<T, size_t, false>
{
    // ctors
public:
    vector() = default;

    explicit vector(size_t size) : detail::vector_base<T, size_t, false>(this->_alloc(size), size, size)
    {
        detail::container_default_construct_or_zeroed(size, this->_data);
    }

    [[nodiscard]] static vector defaulted(size_t size) { return vector(size); }

    [[nodiscard]] static vector uninitialized(size_t size)
    {
        vector v;
        v._size = size;
        v._capacity = size;
        v._data = v._alloc(size);
        return v;
    }

    [[nodiscard]] static vector filled(size_t size, T const& value)
    {
        vector v;
        v.resize(size, value);
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
}
