#pragma once

#include <cstdint>

#include <clean-core/detail/vector_base.hh>

namespace cc
{
/**
 * a customizable and extendable vector type
 *
 * Traits is a type that must provide the following:
 * - Traits::element_t (type of elements of the vector)
 * - Traits::index_t (type of indices into the vector)
 */
template <class Traits>
struct vector_ex : detail::vector_base<typename Traits::element_t, typename Traits::index_t, false>
{
    using element_t = typename Traits::element_t;
    using index_t = typename Traits::index_t;
    using vector_base_t = detail::vector_base<element_t, index_t, false>;

    // ctors
public:
    vector_ex() = default;

    explicit vector_ex(size_t size) : vector_base_t(this->_alloc(size), size, size)
    {
        for (size_t i = 0; i < size; ++i)
            new (placement_new, &this->_data[i]) element_t();
    }

    [[nodiscard]] static vector_ex defaulted(size_t size) { return vector_ex(size); }

    [[nodiscard]] static vector_ex uninitialized(size_t size)
    {
        vector_ex v;
        v._size = size;
        v._capacity = size;
        v._data = v._alloc(size);
        return v;
    }

    [[nodiscard]] static vector_ex filled(size_t size, element_t const& value)
    {
        vector_ex v;
        v.resize(size, value);
        return v;
    }

    explicit vector_ex(element_t const* begin, size_t num_elements)
    {
        this->reserve(num_elements);
        detail::container_copy_construct_range<element_t>(begin, num_elements, this->_data);
        this->_size = num_elements;
    }

    explicit vector_ex(cc::span<element_t const> data) : vector_ex(data.begin(), data.size()) {}

    template <class Range, cc::enable_if<cc::is_any_range<Range>> = true>
    explicit vector_ex(Range const& range)
    {
        this->push_back_range(range);
    }

    vector_ex(std::initializer_list<element_t> data) : vector_ex(data.begin(), data.size()) {}

    vector_ex(vector_ex const& rhs) : vector_ex(rhs.begin(), rhs.size()) {}

    vector_ex(vector_ex&& rhs) noexcept
    {
        this->_data = rhs._data;
        this->_size = rhs._size;
        this->_capacity = rhs._capacity;
        rhs._data = nullptr;
        rhs._size = 0;
        rhs._capacity = 0;
    }
    ~vector_ex()
    {
        detail::container_destroy_reverse<element_t>(this->_data, this->_size);
        this->_free(this->_data);
    }
    vector_ex& operator=(vector_ex const& rhs)
    {
        if (this != &rhs)
        {
            detail::container_destroy_reverse<element_t>(this->_data, this->_size);
            // ensure enough memory has been allocated
            if (this->_capacity < rhs._size)
            {
                this->_free(this->_data);
                this->_data = this->_alloc(rhs._size);
                this->_capacity = rhs._size;
            }
            detail::container_copy_construct_range<element_t>(rhs._data, rhs._size, this->_data);
            this->_size = rhs._size;
        }
        return *this;
    }
    vector_ex& operator=(vector_ex&& rhs) noexcept
    {
        detail::container_destroy_reverse<element_t>(this->_data, this->_size);
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
template <class Traits>
struct hash<vector_ex<Traits>>
{
    [[nodiscard]] constexpr uint64_t operator()(vector_ex<Traits> const& a) const noexcept
    {
        uint64_t h = 0;
        for (auto const& v : a)
            h = cc::hash_combine(h, hash<typename Traits::element_t>{}(v));
        return h;
    }
};
}
