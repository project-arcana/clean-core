#pragma once

#include <cstddef>
#include <cstdint>

namespace cc::detail
{
template <size_t N, size_t Alignment>
auto helper_size_t()
{
    if constexpr (N < (1 << 8) && Alignment <= 1)
        return uint8_t{};
    else if constexpr (N < (1 << 16) && Alignment <= 2)
        return uint16_t{};
    else if constexpr (N < (1uLL << 32) && Alignment <= 4)
        return uint32_t{};
    else
        return uint64_t{};
}

template <size_t N, size_t Alignment>
using compact_size_t_for = decltype(helper_size_t<N, Alignment>());

/// Indirection workaround for a current MSVC compiler bug (19.22)
/// without indirection: https://godbolt.org/z/iQ19yj
/// with indirection: https://godbolt.org/z/6MoWE4
/// Did not yet report this to Microsoft
template<class T, size_t N>
using compact_size_t_typed = compact_size_t_for<N, alignof(T)>;
}
