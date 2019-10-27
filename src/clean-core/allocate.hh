#pragma once

#include <cstddef>

#include <clean-core/forward.hh>
#include <clean-core/assert.hh>
#include <clean-core/typedefs.hh>

namespace cc
{
/**
 * allocates and creates a new T with the given arguments
 * uses an optimized pool allocator
 * is thread-safe and faster than new/delete
 *
 * Good codegen: https://godbolt.org/z/GMfwoF
 *
 * LIMITATIONS:
 *   * every alloc<T> must be met with a free<T>
 *     (especially alloc<T> with free<BaseOfT> does NOT work!)
 */
template <class T, class... Args>
T* alloc(Args&&... args);

/// every obj created by cc::alloc must be freed by cc::free
template <class T>
void free(T* p);

/// cannot free void pointers
void free(void*) = delete;

// ============== Implementation ==============

namespace detail
{
template <size_t Size>
struct pool_block
{
    size_t curr = Size;
    void* ptrs[Size];

    bool can_pop() const { return curr < Size; }
    bool can_push() const { return curr > 0; }

    void push(void* p)
    {
        CC_CONTRACT(can_push());
        ptrs[--curr] = p;
    }
    void* pop()
    {
        CC_CONTRACT(can_pop());
        return ptrs[curr++];
    }
};

template <size_t Size, size_t Align>
struct pool_allocator
{
    static constexpr size_t block_bytes = 1 << 16; // 65kb
    static constexpr size_t block_size = block_bytes / Size;

    struct alignas(Align) storage
    {
        std::byte data[Size];
    };

    struct free_block // TODO: can maybe be merged with block
    {
        pool_block<block_size>* block;
        free_block* next = nullptr;
    };

    pool_block<block_size> free_list;

    // TODO: "work stealing"
    free_block* free_list_next = nullptr;

    CC_COLD_FUNC void alloc_block()
    {
        CC_CONTRACT(!free_list.can_pop());

        if (free_list_next) // take from big list
        {
            auto fb = free_list_next;
            free_list = *fb->block;
            free_list_next = fb->next;
            delete fb->block;
            delete fb;
        }
        else // allocate new block
        {
            auto objs = new storage[block_size];
            free_list.curr = 0;
            for (size_t i = 0; i < block_size; ++i)
                free_list.ptrs[i] = objs + i;
        }
    }

    CC_COLD_FUNC void move_free_block()
    {
        CC_CONTRACT(!free_list.can_push());

        auto b = new pool_block<block_size>(free_list);
        auto fb = new free_block{b, free_list_next};
        free_list_next = fb;
        free_list.curr = block_size; // set to free
    }

    void* alloc()
    {
        if (CC_UNLIKELY(!free_list.can_pop()))
            alloc_block();

        return free_list.pop();
    }

    void free(void* p)
    {
        if (CC_UNLIKELY(!free_list.can_push()))
            move_free_block();

        free_list.push(p);
    }
};

template <size_t Size, size_t Align>
pool_allocator<Size, Align>& get_pool_allocator()
{
    static thread_local pool_allocator<Size, Align> a;
    return a;
}
}

template <class T, class... Args>
T* alloc(Args&&... args)
{
    auto p = detail::get_pool_allocator<sizeof(T), alignof(T)>().alloc();
    new (p) T(cc::forward<Args>(args)...);
    return reinterpret_cast<T*>(p);
}
template <class T>
void free(T* p)
{
    CC_CONTRACT(p != nullptr);
    p->~T();
    detail::get_pool_allocator<sizeof(T), alignof(T)>().free(p);
}
}
