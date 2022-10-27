#pragma once

#include <cstddef>

#include <clean-core/forward.hh>
#include <clean-core/fwd.hh>
#include <clean-core/new.hh>
#include <clean-core/span.hh>
#include <clean-core/utility.hh>

namespace cc
{
struct allocator
{
public:
    //
    // pure virtual

    // allocate a buffer with specified size and alignment
    [[nodiscard]] virtual std::byte* alloc(size_t size, size_t align = alignof(std::max_align_t)) = 0;

    // free a previously allocated buffer
    virtual void free(void* ptr) = 0;

public:
    //
    // optional virtual
    // specific interfaces which can be overriden by allocators

    // reallocate a buffer, behaves like std::realloc
    virtual std::byte* realloc(void* ptr, size_t old_size, size_t new_size, size_t align = alignof(std::max_align_t));

    // can return nullptr if the request can't be satisfied
    [[nodiscard]] virtual std::byte* try_alloc(size_t size, size_t alignment = alignof(std::max_align_t));

    // can return nullptr if the request can't be satisfied
    // the original buffer will remain valid in that case
    virtual std::byte* try_realloc(void* ptr, size_t old_size, size_t new_size, size_t align = alignof(std::max_align_t));

    // reads the size of the given allocation
    // only some allocators can do this, returns true if available
    virtual bool get_allocation_size(void* ptr, size_t& out_size) { return false; }

    // some allocators can internally validate the heap for corruptions
    // returns true if validation is available (asserts internally)
    virtual bool validate_heap() { return false; }

    // returns a descriptive name of this allocator
    virtual char const* get_name() const { return "Unnamed Allocator"; }

public:
    //
    // non-virtual utility

    /// allocate and construct an object
    template <class T, class... Args>
    [[nodiscard]] T* new_t(Args&&... args);

    /// destruct and deallocate an object previously created with new_t
    template <class T>
    void delete_t(T* ptr);

    /// allocate and default-construct an array
    template <class T>
    [[nodiscard]] T* new_array(size_t num_elems);

    /// destruct and deallocate an array previously created with new_array
    template <class T>
    void delete_array(T* ptr);

    /// allocate and default-construct an array without keeping track of the amount of elements
    template <class T>
    [[nodiscard]] T* new_array_sized(size_t num_elems);

    /// destruct and deallocate an array previously created with new_array_sized
    template <class T>
    void delete_array_sized(T* ptr, size_t num_elems);

    /// allocates a new null terminated string with a copy of the source
    char* alloc_string_copy(cc::string_view source);

    template <class T>
    T* alloc_data_copy(cc::span<T const> data);

    // delete copy, default move
    allocator() = default;
    allocator(allocator const&) = delete;
    allocator& operator=(allocator const&) = delete;
    allocator(allocator&&) noexcept = default;
    allocator& operator=(allocator&&) noexcept = default;
    virtual ~allocator() = default;
};

/// global instance of the system allocator (malloc / free) (thread safe)
/// see allocators/system_allocator.cc for implementation
extern allocator* const system_allocator;

//
// templated virtual allocator interface implementation below

namespace detail
{
constexpr size_t get_array_padding(size_t elem_size)
{
    return sizeof(size_t) <= elem_size ? elem_size : elem_size * (1 + sizeof(size_t) / elem_size);
}
}

template <class T, class... Args>
T* allocator::new_t(Args&&... args)
{
    static_assert(sizeof(T) > 0, "cannot construct incomplete type");
    auto* const buf = this->alloc(sizeof(T), alignof(T));
    return new (placement_new, buf) T(forward<Args>(args)...);
}

template <class T>
void allocator::delete_t(T* ptr)
{
    static_assert(sizeof(T) > 0, "cannot destruct incomplete type");
    static_assert(!std::is_const_v<T>, "cannot delete pointer-to-const");

    if (ptr == nullptr)
        return;

    if constexpr (!std::is_trivially_destructible_v<T>)
        ptr->~T();

    this->free(ptr);
}

template <class T>
T* allocator::new_array(size_t num_elems)
{
    static_assert(sizeof(T) > 0, "cannot construct incomplete type");
    constexpr size_t padding = detail::get_array_padding(sizeof(T));
    size_t size = sizeof(T) * num_elems + padding;

    auto* const original_buf = this->alloc(size, alignof(T));
    *reinterpret_cast<size_t*>(original_buf) = num_elems; // write amount of elements

    T* const res_array_ptr = reinterpret_cast<T*>(original_buf + padding);

    if constexpr (!std::is_trivially_constructible_v<T>)
    {
        for (auto i = 0u; i < num_elems; ++i)
            new (placement_new, res_array_ptr + i) T();
    }
    else
    {
        std::memset(res_array_ptr, 0, sizeof(T) * num_elems);
    }

    return res_array_ptr;
}

template <class T>
void allocator::delete_array(T* ptr)
{
    static_assert(sizeof(T) > 0, "cannot destruct incomplete type");
    static_assert(!std::is_const_v<T>, "cannot delete pointer-to-const");

    if (ptr == nullptr)
        return;

    constexpr size_t padding = detail::get_array_padding(sizeof(T));

    std::byte* const original_buf = reinterpret_cast<std::byte*>(ptr) - padding;
    size_t const num_elems = *reinterpret_cast<size_t*>(original_buf);

    if constexpr (!std::is_trivially_destructible_v<T>)
    {
        for (auto i = 0u; i < num_elems; ++i)
            (ptr + i)->~T();
    }

    this->free(original_buf);
}


template <class T>
T* allocator::new_array_sized(size_t num_elems)
{
    static_assert(sizeof(T) > 0, "cannot construct incomplete type");
    T* const res_array_ptr = reinterpret_cast<T*>(this->alloc(sizeof(T) * num_elems, alignof(T)));

    if constexpr (!std::is_trivially_constructible_v<T>)
    {
        for (auto i = 0u; i < num_elems; ++i)
            new (placement_new, res_array_ptr + i) T();
    }
    else
    {
        std::memset(res_array_ptr, 0, sizeof(T) * num_elems);
    }

    return res_array_ptr;
}


template <class T>
void allocator::delete_array_sized(T* ptr, size_t num_elems)
{
    static_assert(sizeof(T) > 0, "cannot destruct incomplete type");
    static_assert(!std::is_const_v<T>, "cannot delete pointer-to-const");

    if (ptr == nullptr)
        return;

    if constexpr (!std::is_trivially_destructible_v<T>)
    {
        for (auto i = 0u; i < num_elems; ++i)
            (ptr + i)->~T();
    }

    this->free(ptr);
}

template <class T>
T* allocator::alloc_data_copy(cc::span<T const> data)
{
    static_assert(sizeof(T) > 0, "T must be complete");
    static_assert(std::is_trivially_copyable_v<T>, "T must be memcpyable");

    T* const res = reinterpret_cast<T*>(this->alloc(data.size_bytes(), alignof(T)));

    if (!res)
        return nullptr;

    std::memcpy(res, data.data(), data.size_bytes());
    return res;
}
}
