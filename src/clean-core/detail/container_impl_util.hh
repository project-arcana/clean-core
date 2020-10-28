#pragma once

#include <cstddef>
#include <cstring>
#include <type_traits>

#include <clean-core/move.hh>
#include <clean-core/new.hh>

namespace cc::detail
{
template <class T, class SizeT = std::size_t>
void container_move_construct_range(T* __restrict src, SizeT num, T* __restrict dest)
{
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
void container_copy_construct_range(T const* __restrict src, SizeT num, T* __restrict dest)
{
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
void container_copy_construct_fill(T const& value, SizeT num, T* __restrict dest)
{
    for (SizeT i = 0; i < num; ++i)
        new (placement_new, &dest[i]) T(value);
}

template <class T, class SizeT = std::size_t>
void container_destroy_reverse([[maybe_unused]] T* data, [[maybe_unused]] SizeT size, [[maybe_unused]] SizeT to_index = 0)
{
    if constexpr (!std::is_trivially_destructible_v<T>)
    {
        for (SizeT i = size; i > to_index; --i)
            data[i - 1].~T();
    }
}
}
