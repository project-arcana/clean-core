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
    /// allocate a buffer with specified size and alignment
    [[nodiscard]] virtual std::byte* alloc(size_t size, size_t align = alignof(std::max_align_t)) = 0;

    /// free a pointer previously received from alloc
    virtual void free(void* ptr) = 0;

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

    // below:
    // specific interfaces which can be overriden by allocators to be more efficient, with fallback otherwise

    /// reallocate a buffer, behaves like std::realloc
    virtual std::byte* realloc(void* ptr, size_t old_size, size_t new_size, size_t align = alignof(std::max_align_t));

    /// allocate a buffer with specified minimum size, will span up to request_size if possible
    [[nodiscard]] virtual std::byte* alloc_request(size_t min_size, size_t request_size, size_t& out_received_size, size_t align = alignof(std::max_align_t));

    /// reallocate a buffer with specified minimum size, will span up to request_size if possible
    [[nodiscard]] virtual std::byte* realloc_request(
        void* ptr, size_t old_size, size_t new_min_size, size_t request_size, size_t& out_received_size, size_t align = alignof(std::max_align_t));

    // delete copy, default move
    allocator() = default;
    allocator(allocator const&) = delete;
    allocator& operator=(allocator const&) = delete;
    allocator(allocator&&) noexcept = default;
    allocator& operator=(allocator&&) noexcept = default;
    virtual ~allocator() = default;
};


/// system provided allocator (malloc / free)
struct system_allocator_t final : allocator
{
    std::byte* alloc(size_t size, size_t align = alignof(std::max_align_t)) override;

    void free(void* ptr) override;

    std::byte* realloc(void* ptr, size_t old_size, size_t new_size, size_t align = alignof(std::max_align_t)) override;

    system_allocator_t() = default;
};


/// global instance of the system allocator (malloc / free) (thread safe)
extern allocator* const system_allocator;

/// trivial linear allocator operating in a given buffer
/// cannot free individual allocations, only reset entirely
struct linear_allocator final : allocator
{
    std::byte* alloc(size_t size, size_t align = alignof(std::max_align_t)) override
    {
        CC_ASSERT(_buffer_begin != nullptr && "linear_allocator uninitialized");

        auto* const padded_res = cc::align_up(_head, align);
        CC_ASSERT(padded_res + size <= _buffer_end && "linear_allocator overcommitted");
        _head = padded_res + size;

        _latest_allocation = padded_res;
        return padded_res;
    }

    void free(void* ptr) override
    {
        // no-op
        (void)ptr;
    }

    std::byte* realloc(void* ptr, size_t old_size, size_t new_size, size_t align = alignof(std::max_align_t)) override
    {
        if (!ptr || ptr != _latest_allocation)
        {
            return new_size > 0 ? this->alloc(new_size, align) : nullptr;
        }

        std::byte* const ptr_byte = static_cast<std::byte*>(ptr);
        CC_ASSERT(old_size == _head - ptr_byte && "old_size incorrect");
        CC_ASSERT(ptr_byte + new_size <= _buffer_end && "linear_allocator overcommitted");

        _head = ptr_byte + new_size;
        return ptr_byte;
    }

    void reset()
    {
        _head = _buffer_begin;
        _latest_allocation = nullptr;
    }

    size_t allocated_size() const { return _head - _buffer_begin; }
    size_t remaining_size() const { return _buffer_end - _head; }
    size_t max_size() const { return _buffer_end - _buffer_begin; }
    float allocated_ratio() const { return allocated_size() / float(max_size()); }

    std::byte* buffer() const { return _buffer_begin; }

    linear_allocator() = default;
    linear_allocator(span<std::byte> buffer) : _buffer_begin(buffer.data()), _head(buffer.data()), _buffer_end(buffer.data() + buffer.size()) {}

private:
    std::byte* _buffer_begin = nullptr;
    std::byte* _head = nullptr;
    std::byte* _buffer_end = nullptr;
    std::byte* _latest_allocation = nullptr;
};


/// stack allocator operating in a given buffer
/// like a linear allocator, but can also free the most recent allocation
struct stack_allocator final : allocator
{
    std::byte* alloc(size_t size, size_t align = alignof(std::max_align_t)) override;

    /// NOTE: ptr must be the most recent allocation received
    void free(void* ptr) override;

    /// NOTE: ptr must be the most recent allocation received
    std::byte* realloc(void* ptr, size_t old_size, size_t new_size, size_t align = alignof(std::max_align_t)) override;

    void reset()
    {
        _head = _buffer_begin;
        _last_alloc_id = 0;
    }

    stack_allocator() = default;
    stack_allocator(span<std::byte> buffer) : _buffer_begin(buffer.data()), _head(buffer.data()), _buffer_end(buffer.data() + buffer.size()) {}

private:
    std::byte* _buffer_begin = nullptr;
    std::byte* _head = nullptr;
    std::byte* _buffer_end = nullptr;
    int32_t _last_alloc_id = 0;
};

/// linear allocator operating in virtual memory
/// reserves pages on init, commits pages on demand
/// only frees pages if explicitly called
struct virtual_linear_allocator final : allocator
{
    virtual_linear_allocator() = default;
    virtual_linear_allocator(size_t max_size_bytes, size_t chunk_size_bytes = 65536) { initialize(max_size_bytes, chunk_size_bytes); }
    ~virtual_linear_allocator() { destroy(); }

    // max_size_bytes: amount of contiguous virtual memory being reserved
    // chunk_size_bytes: increment of physical memory being committed whenever more is required
    // note there is a lower limit on virtual allocation granularity (Win32: 64K = 16 pages)
    void initialize(size_t max_size_bytes, size_t chunk_size_bytes = 65536);

    void destroy();

    std::byte* alloc(size_t size, size_t align = alignof(std::max_align_t)) override;

    void free(void* ptr) override { (void)ptr; }

    std::byte* realloc(void* ptr, size_t old_size, size_t new_size, size_t align = alignof(std::max_align_t)) override;

    // free all current allocations
    // does not decommit any memory!
    size_t reset()
    {
        size_t const num_bytes_allocated = _physical_current - _virtual_begin;
        _physical_current = _virtual_begin;
        _last_allocation = nullptr;
        return num_bytes_allocated;
    }

    // decommit the physical memory of all pages not currently required
    // returns amount of bytes decommitted
    size_t decommit_idle_memory();

    // amount of bytes in the virtual address range
    size_t get_virtual_size_bytes() const { return _virtual_end - _virtual_begin; }

    // amount of bytes in the physically committed memory
    size_t get_physical_size_bytes() const { return _physical_end - _virtual_begin; }

    // amount of bytes in the physically committed and allocated memory
    size_t get_allocated_size_bytes() const { return _physical_current - _virtual_begin; }

private:
    std::byte* _virtual_begin = nullptr;
    std::byte* _virtual_end = nullptr;
    std::byte* _physical_current = nullptr;
    std::byte* _physical_end = nullptr;
    std::byte* _last_allocation = nullptr;
    size_t _chunk_size_bytes = 0;
};

/// stack allocator operating in virtual memory
/// reserves pages on init, commits pages on demand
/// only frees pages if explicitly called
struct virtual_stack_allocator final : allocator
{
    virtual_stack_allocator() = default;
    virtual_stack_allocator(size_t max_size_bytes, size_t chunk_size_bytes = 65536) { initialize(max_size_bytes, chunk_size_bytes); }
    ~virtual_stack_allocator() { destroy(); }

    // max_size_bytes: amount of contiguous virtual memory being reserved
    // chunk_size_bytes: increment of physical memory being committed whenever more is required
    // note there is a lower limit on virtual allocation granularity (Win32: 64K = 16 pages)
    void initialize(size_t max_size_bytes, size_t chunk_size_bytes = 65536);

    void destroy();

    std::byte* alloc(size_t size, size_t align = alignof(std::max_align_t)) override;

    /// NOTE: ptr must be the most recent allocation received
    void free(void* ptr) override;

    /// NOTE: ptr must be the most recent allocation received
    std::byte* realloc(void* ptr, size_t old_size, size_t new_size, size_t align = alignof(std::max_align_t)) override;

    // free all current allocations
    // does not decommit any memory!
    size_t reset();

    // decommit the physical memory of all pages not currently required
    // returns amount of bytes decommitted
    size_t decommit_idle_memory();

    // amount of bytes in the virtual address range
    size_t get_virtual_size_bytes() const { return _virtual_end - _virtual_begin; }

    // amount of bytes in the physically committed memory
    size_t get_physical_size_bytes() const { return _physical_end - _virtual_begin; }

    // amount of bytes in the physically committed and allocated memory
    size_t get_allocated_size_bytes() const { return _physical_current - _virtual_begin; }

    // returns whether the given ptr is the latest allocation, meaning it can be freed or reallocated
    bool is_latest_allocation(void* ptr) const;

private:
    std::byte* _virtual_begin = nullptr;
    std::byte* _virtual_end = nullptr;
    std::byte* _physical_current = nullptr;
    std::byte* _physical_end = nullptr;
    int32_t _last_alloc_id = 0;
    size_t _chunk_size_bytes = 0;
};

/// Two Level Segregated Fit allocator
/// O(1) cost for alloc, free, realloc
/// extremely low memory overhead, 4 byte per allocation
struct tlsf_allocator final : allocator
{
    tlsf_allocator() = default;
    tlsf_allocator(cc::span<std::byte> buffer) { initialize(buffer); }
    ~tlsf_allocator() { destroy(); }

    void initialize(cc::span<std::byte> buffer);
    void destroy();

    // provide an additional memory to the TLSF (can be done multiple times)
    void add_pool(cc::span<std::byte> buffer);

    // returns false if internal consistency checks fail
    bool check_consistency();

    std::byte* alloc(size_t size, size_t align = alignof(std::max_align_t)) override;

    void free(void* ptr) override;

    std::byte* realloc(void* ptr, size_t old_size, size_t new_size, size_t align = alignof(std::max_align_t)) override;

    tlsf_allocator(tlsf_allocator&& rhs) noexcept : _tlsf(rhs._tlsf) { rhs._tlsf = nullptr; }
    tlsf_allocator& operator==(tlsf_allocator&& rhs) noexcept
    {
        destroy();
        _tlsf = rhs._tlsf;
        rhs._tlsf = nullptr;
        return *this;
    }

private:
    void* _tlsf = nullptr;
};

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
