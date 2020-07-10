#pragma once

#include <clean-core/forward.hh>
#include <clean-core/new.hh>
#include <clean-core/span.hh>
#include <clean-core/typedefs.hh>

namespace cc
{
struct allocator
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

    virtual ~allocator() = default;
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

    linear_allocator() = default;
    linear_allocator(cc::span<cc::byte> buffer) : _buffer(buffer.data()), _head(buffer.data()), _buffer_end(buffer.data() + buffer.size()) {}

    linear_allocator(linear_allocator&&) noexcept = default;
    linear_allocator& operator=(linear_allocator&&) noexcept = default;
    linear_allocator(linear_allocator const&) = delete;
    linear_allocator& operator=(linear_allocator const&) = delete;

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

    void reset()
    {
        _head = _buffer;
        _last_alloc = nullptr;
    }

    stack_allocator() = default;
    stack_allocator(cc::span<cc::byte> buffer) : _buffer(buffer.data()), _head(buffer.data()), _buffer_end(buffer.data() + buffer.size()) {}

    stack_allocator(stack_allocator&&) noexcept = default;
    stack_allocator& operator=(stack_allocator&&) noexcept = default;
    stack_allocator(stack_allocator const&) = delete;
    stack_allocator& operator=(stack_allocator const&) = delete;

private:
    cc::byte* _buffer = nullptr;
    cc::byte* _head = nullptr;
    cc::byte* _buffer_end = nullptr;
    cc::byte* _last_alloc = nullptr;
};

struct system_allocator final : public allocator
{
public:
    cc::byte* alloc(size_t size, size_t align = alignof(std::max_align_t)) override;

    void free(void* ptr) override;

    system_allocator() = default;
};

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
    CC_CONTRACT(ptr != nullptr);

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
    CC_CONTRACT(ptr != nullptr);
    constexpr size_t padding = detail::get_array_padding(sizeof(T));

    cc::byte* const original_buf = reinterpret_cast<cc::byte*>(ptr) - padding;
    size_t const num_elems = *reinterpret_cast<size_t*>(original_buf);

    if constexpr (!std::is_trivially_destructible_v<T>)
        for (auto i = 0u; i < num_elems; ++i)
            (ptr + i)->~T();

    this->free(original_buf);
}
}
