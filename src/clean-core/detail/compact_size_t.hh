#pragma once

#include <cstddef>
#include <cstdint>

#include <clean-core/always_false.hh>

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
/// Bug report: https://developercommunity.visualstudio.com/content/problem/800899/false-positive-for-c2975-on-alias-template-fixed-w.html
template <class T, size_t N>
using compact_size_t_typed = compact_size_t_for<N, alignof(T)>;

template <size_t bits>
struct compact_size_t_by_bits_impl
{
    static_assert(always_false_v<bits>, "bit size not supported");
};
template <>
struct compact_size_t_by_bits_impl<8>
{
    using type = std::uint8_t;
};
template <>
struct compact_size_t_by_bits_impl<16>
{
    using type = std::uint16_t;
};
template <>
struct compact_size_t_by_bits_impl<32>
{
    using type = std::uint32_t;
};
template <>
struct compact_size_t_by_bits_impl<64>
{
    using type = std::uint64_t;
};

template <size_t bits>
using compact_size_t_by_bits = typename compact_size_t_by_bits_impl<(bits <= 8 ? 8 : bits <= 16 ? 16 : bits <= 32 ? 32 : 64)>::type;
}
