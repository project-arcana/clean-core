#pragma once

#include <clean-core/forward.hh>
#include <clean-core/new.hh>
#include <clean-core/polymorphic.hh>
#include <clean-core/span.hh>
#include <clean-core/typedefs.hh>

namespace cc
{
struct allocator : cc::polymorphic
{
    /// allocate a buffer with specified size and alignment
    [[nodiscard]] virtual cc::byte* alloc(size_t size, size_t align = alignof(std::max_align_t)) = 0;

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

    // specific interfaces which can be overriden by allocators to be more efficient, with fallback otherwise
public:
    /// reallocate a buffer, behaves like std::realloc
    virtual cc::byte* realloc(void* ptr, size_t old_size, size_t new_size, size_t align = alignof(std::max_align_t));

    /// allocate a buffer with specified minimum size, will span up to request_size if possible
    [[nodiscard]] virtual cc::byte* alloc_request(size_t min_size, size_t request_size, size_t& out_received_size, size_t align = alignof(std::max_align_t));

    /// reallocate a buffer with specified minimum size, will span up to request_size if possible
    [[nodiscard]] virtual cc::byte* realloc_request(
        void* ptr, size_t old_size, size_t new_min_size, size_t request_size, size_t& out_received_size, size_t align = alignof(std::max_align_t));
};


struct linear_allocator final : public allocator
{
public:
    cc::byte* alloc(size_t size, size_t align = alignof(std::max_align_t)) override;

    void free(void* ptr) override
    {
        // no-op
        (void)ptr;
    }

    void reset() { _head = _buffer; }

    size_t allocated_size() const { return _head - _buffer; }
    size_t remaining_size() const { return _buffer_end - _head; }
    size_t max_size() const { return _buffer_end - _buffer; }
    float allocated_ratio() const { return allocated_size() / float(max_size()); }

    linear_allocator() = default;
    linear_allocator(cc::span<cc::byte> buffer) : _buffer(buffer.data()), _head(buffer.data()), _buffer_end(buffer.data() + buffer.size()) {}

private:
    cc::byte* _buffer = nullptr;
    cc::byte* _head = nullptr;
    cc::byte* _buffer_end = nullptr;
};

struct stack_allocator final : public allocator
{
public:
    cc::byte* alloc(size_t size, size_t align = alignof(std::max_align_t)) override;

    void free(void* ptr) override;

    cc::byte* realloc(void* ptr, size_t old_size, size_t new_size, size_t align = alignof(std::max_align_t)) override;

    void reset()
    {
        _head = _buffer;
        _last_alloc = nullptr;
    }

    stack_allocator() = default;
    stack_allocator(cc::span<cc::byte> buffer) : _buffer(buffer.data()), _head(buffer.data()), _buffer_end(buffer.data() + buffer.size()) {}

private:
    cc::byte* _buffer = nullptr;
    cc::byte* _head = nullptr;
    cc::byte* _buffer_end = nullptr;
    cc::byte* _last_alloc = nullptr;
};

struct system_allocator_t final : public allocator
{
public:
    cc::byte* alloc(size_t size, size_t align = alignof(std::max_align_t)) override;

    void free(void* ptr) override;

    cc::byte* realloc(void* ptr, size_t old_size, size_t new_size, size_t align = alignof(std::max_align_t)) override;

    system_allocator_t() = default;
};

inline cc::system_allocator_t system_allocator_instance;
inline constexpr cc::allocator* const system_allocator = &system_allocator_instance;

//
// implementation below

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
    auto* const buf = this->alloc(sizeof(T), alignof(T));
    return new (placement_new, buf) T(cc::forward<Args>(args)...);
}

template <class T>
void allocator::delete_t(T* ptr)
{
    if (ptr == nullptr)
        return;

    if constexpr (!std::is_trivially_destructible_v<T>)
        ptr->~T();

    this->free(ptr);
}

template <class T>
T* allocator::new_array(size_t num_elems)
{
    constexpr size_t padding = detail::get_array_padding(sizeof(T));
    size_t size = sizeof(T) * num_elems + padding;

    auto* const original_buf = this->alloc(size, alignof(T));
    *reinterpret_cast<size_t*>(original_buf) = num_elems; // write amount of elements

    T* const res_array_ptr = reinterpret_cast<T*>(original_buf + padding);

    for (auto i = 0u; i < num_elems; ++i)
        new (placement_new, res_array_ptr + i) T();

    return res_array_ptr;
}

template <class T>
void allocator::delete_array(T* ptr)
{
    if (ptr == nullptr)
        return;

    constexpr size_t padding = detail::get_array_padding(sizeof(T));

    cc::byte* const original_buf = reinterpret_cast<cc::byte*>(ptr) - padding;
    size_t const num_elems = *reinterpret_cast<size_t*>(original_buf);

    if constexpr (!std::is_trivially_destructible_v<T>)
        for (auto i = 0u; i < num_elems; ++i)
            (ptr + i)->~T();

    this->free(original_buf);
}


template <class T>
T* allocator::new_array_sized(size_t num_elems)
{
    T* const res_array_ptr = reinterpret_cast<T*>(this->alloc(sizeof(T) * num_elems, alignof(T)));
    for (auto i = 0u; i < num_elems; ++i)
        new (placement_new, res_array_ptr + i) T();
    return res_array_ptr;
}


template <class T>
void allocator::delete_array_sized(T* ptr, cc::size_t num_elems)
{
    if (ptr == nullptr)
        return;

    if constexpr (!std::is_trivially_destructible_v<T>)
        for (auto i = 0u; i < num_elems; ++i)
            (ptr + i)->~T();
    this->free(ptr);
}
}
