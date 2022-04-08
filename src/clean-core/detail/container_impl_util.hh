#pragma once

#include <cstddef>
#include <cstring>
#include <type_traits>

#include <clean-core/macros.hh>
#include <clean-core/move.hh>
#include <clean-core/new.hh>

namespace cc::detail
{
template <class T, class SizeT = std::size_t>
CC_FORCE_INLINE void container_move_construct_range(T* __restrict src, SizeT num, T* __restrict dest)
{
    static_assert(sizeof(T) > 0, "cannot move incomplete types");
    if constexpr (std::is_trivially_move_constructible_v<T> && std::is_trivially_copyable_v<T>)
    {
        if (num > 0)
            std::memcpy(dest, src, sizeof(T) * num);
    }
    else
    {
        for (SizeT i = 0; i < num; ++i)
            new (placement_new, &dest[i]) T(cc::move(src[i]));
    }
}

template <class T, class SizeT = std::size_t>
CC_FORCE_INLINE void container_copy_construct_range(T const* __restrict src, SizeT num, T* __restrict dest)
{
    static_assert(sizeof(T) > 0, "cannot copy incomplete types");
    if constexpr (std::is_trivially_copyable_v<T>)
    {
        if (num > 0)
            std::memcpy(dest, src, sizeof(T) * num);
    }
    else
    {
        for (SizeT i = 0; i < num; ++i)
            new (placement_new, &dest[i]) T(src[i]);
    }
}

template <class T, class SizeT = std::size_t>
CC_FORCE_INLINE void container_default_construct_or_zeroed(SizeT num, T* __restrict dest)
{
    static_assert(sizeof(T) > 0, "cannot copy incomplete types");
    if constexpr (!std::is_trivially_constructible_v<T>)
    {
        for (SizeT i = 0; i < num; ++i)
            new (placement_new, &dest[i]) T();
    }
    else
    {
        std::memset(dest, 0, num * sizeof(T));
    }
}

template <class T, class SizeT = std::size_t>
CC_FORCE_INLINE void container_copy_construct_fill(T const& value, SizeT num, T* __restrict dest)
{
    static_assert(sizeof(T) > 0, "cannot copy incomplete types");
    for (SizeT i = 0; i < num; ++i)
        new (placement_new, &dest[i]) T(value);
}

template <class T, class SizeT = std::size_t>
CC_FORCE_INLINE void container_relocate_construct_range(T* dest, T* src, SizeT num_elements)
{
    if constexpr (std::is_trivially_copyable_v<T>)
    {
        if (num_elements > 0)
        {
            std::memmove(dest, src, num_elements * sizeof(T));
        }
    }
    else
    {
        for (SizeT i = 0; i < num_elements; ++i)
        {
            // move-construct new element in place
            new (placement_new, &dest[i]) T(cc::move(src[i]));
            // call dtor on old element
            src[i].~T();
        }
    }
}

template <class T, class SizeT = std::size_t>
CC_FORCE_INLINE void container_destroy_reverse([[maybe_unused]] T* data, [[maybe_unused]] SizeT size, [[maybe_unused]] SizeT to_index = 0)
{
    static_assert(sizeof(T) > 0, "cannot destroy incomplete types");
    if constexpr (!std::is_trivially_destructible_v<T>)
    {
        for (SizeT i = size; i > to_index; --i)
            data[i - 1].~T();
    }
}
}
