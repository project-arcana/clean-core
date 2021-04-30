#pragma once

#include <cstdint>

#include <clean-core/macros.hh>

#ifdef CC_COMPILER_MSVC
#include <intrin.h>
#else
#include <cpuid.h>
#include <x86intrin.h>
#endif


namespace cc
{
#ifdef CC_COMPILER_MSVC

inline int popcount(uint8_t v) { return int(__popcnt16(v)); }
inline int popcount(uint16_t v) { return int(__popcnt16(v)); }
inline int popcount(uint32_t v) { return int(__popcnt(v)); }
inline int popcount(uint64_t v) { return int(__popcnt64(v)); }

inline uint16_t byteswap(uint16_t v) { return _byteswap_ushort(v); }
inline uint32_t byteswap(uint32_t v) { return _byteswap_ulong(v); }
inline uint64_t byteswap(uint64_t v) { return _byteswap_uint64(v); }

// NOTE: the _lzcnt_u32/_tzcnt_u32 intrinsics don't care about compilation target arch, they always output T/LZCNT
// however LZCNT, on an assembly level, falls back to plain BSR if LZCNT isn't supported
// if unsupported, we work around this using (explicit) BSR, XOR, SUB and a branch on 0
// LZCNT support: Intel: Haswell (4th gen Core-i, 2013), AMD: Piledriver (ABM, 2012)
// TZCNT is de-facto always supported if LZCNT is, but it's nominally part of BMI1
// MSVC has no macro for LZCNT/BMI1 specifically, but AVX2 always includes it
// only a few AMD CPUs after piledriver (pre-Zen) are incorrectly excluded by this approximation
#ifdef __AVX2__
inline int count_trailing_zeros(uint32_t v) { return int(_tzcnt_u32(v)); }
inline int count_trailing_zeros(uint64_t v) { return int(_tzcnt_u64(v)); }

inline int count_leading_zeros(uint8_t v) { return int(_lzcnt_u32(v) - 24); }
inline int count_leading_zeros(uint16_t v) { return int(_lzcnt_u32(v) - 16); }
inline int count_leading_zeros(uint32_t v) { return int(_lzcnt_u32(v)); }
inline int count_leading_zeros(uint64_t v) { return int(_lzcnt_u64(v)); }
#else
inline int count_trailing_zeros(uint32_t v)
{
    if (v == 0)
        return 32;
    unsigned long idx;
    _BitScanForward(&idx, v);
    return int(idx);
}
inline int count_trailing_zeros(uint64_t v)
{
    if (v == 0)
        return 64;
    unsigned long idx;
    _BitScanForward64(&idx, v);
    return int(idx);
}

inline int count_leading_zeros(uint8_t v)
{
    if (v == 0)
        return 8;
    unsigned long idx;
    _BitScanReverse(&idx, v);
    return int(idx ^ 31u) - 24;
}
inline int count_leading_zeros(uint16_t v)
{
    if (v == 0)
        return 16;
    unsigned long idx;
    _BitScanReverse(&idx, v);
    return int(idx ^ 31u) - 16;
}
inline int count_leading_zeros(uint32_t v)
{
    if (v == 0)
        return 32;
    unsigned long idx;
    _BitScanReverse(&idx, v);
    return int(idx ^ 31u);
}
inline int count_leading_zeros(uint64_t v)
{
    if (v == 0)
        return 64;
    unsigned long idx;
    _BitScanReverse64(&idx, v);
    return int(idx ^ 63u);
}
#endif

#else

inline int popcount(uint8_t v) { return __builtin_popcount(v); }
inline int popcount(uint16_t v) { return __builtin_popcount(v); }
inline int popcount(uint32_t v) { return __builtin_popcount(v); }
inline int popcount(uint64_t v) { return __builtin_popcountll(v); }

inline uint16_t byteswap(uint16_t val) { return __builtin_bswap16(val); }
inline uint32_t byteswap(uint32_t val) { return __builtin_bswap32(val); }
inline uint64_t byteswap(uint64_t val) { return __builtin_bswap64(val); }

#ifdef __BMI__
inline int count_trailing_zeros(uint32_t v) { return int(_tzcnt_u32(v)); }
inline int count_trailing_zeros(uint64_t v) { return int(_tzcnt_u64(v)); }
#else
inline int count_trailing_zeros(uint32_t v) { return v ? __builtin_ctz(v) : 32; }
inline int count_trailing_zeros(uint64_t v) { return v ? __builtin_ctzll(v) : 64; }
#endif

// NOTE: __builtin_clz by default compiles to "BSR <ret>, <val>; XOR <ret>, 31"
// Only with -mlzcnt (or -march=...) does it compile to LZCNT proper.
// This causes inconsistent behavior with 0 as BSRs result is unspecified if <val> is zero
// __LZCNT__ is defined if compiling with a target arch that supports LZCNT (like AVX2)
// if not, we work around this using BSR, XOR (as part of __builtin_clz), SUB, and a branch on 0 (explicit)
#ifdef __LZCNT__
inline int count_leading_zeros(uint8_t v) { return int(__lzcnt16(v) - 8); }
inline int count_leading_zeros(uint16_t v) { return int(__lzcnt16(v)); }
inline int count_leading_zeros(uint32_t v) { return int(__lzcnt32(v)); }
inline int count_leading_zeros(uint64_t v) { return int(__lzcnt64(v)); }
#else
inline int count_leading_zeros(uint8_t v) { return v ? __builtin_clz(v) - 24 : 8; }
inline int count_leading_zeros(uint16_t v) { return v ? __builtin_clz(v) - 16 : 16; }
inline int count_leading_zeros(uint32_t v) { return v ? __builtin_clz(v) : 32; }
inline int count_leading_zeros(uint64_t v) { return v ? __builtin_clzll(v) : 64; }
#endif

#endif

// returns rounded down logarithm to base 2
inline uint32_t bit_log2(uint32_t v) { return uint32_t(8 * sizeof(uint32_t) - count_leading_zeros(v) - 1); }
inline uint64_t bit_log2(uint64_t v) { return uint64_t(8 * sizeof(uint64_t) - count_leading_zeros(v) - 1); }

// ceils to the nearest power of 2
inline uint32_t ceil_pow2(uint32_t v) { return uint32_t(1) << (bit_log2(v - uint32_t(1)) + 1); }
inline uint64_t ceil_pow2(uint64_t v) { return uint64_t(1) << (bit_log2(v - uint64_t(1)) + 1); }

// returns true if v is a power of 2
constexpr bool is_pow2(uint32_t v) { return ((v & (v - uint32_t(1))) == 0); }
constexpr bool is_pow2(uint64_t v) { return ((v & (v - uint64_t(1))) == 0); }

// computes v % divisor, divisor must be a power of 2
constexpr uint32_t mod_pow2(uint32_t v, uint32_t divisor) { return v & (divisor - 1); }
constexpr uint64_t mod_pow2(uint64_t v, uint64_t divisor) { return v & (divisor - 1); }

// computes floor(v / divisor), divisor must be a power of 2
inline uint32_t div_pow2_floor(uint32_t v, uint32_t divisor) { return v >> bit_log2(divisor); }
inline uint64_t div_pow2_floor(uint64_t v, uint64_t divisor) { return v >> bit_log2(divisor); }

// computes ceil(v / divisor), v > 0, divisor must be a power of 2
inline uint32_t div_pow2_ceil(uint32_t v, uint32_t divisor) { return ((v - uint32_t(1)) >> bit_log2(divisor)) + uint32_t(1); }
inline uint64_t div_pow2_ceil(uint64_t v, uint64_t divisor) { return ((v - uint64_t(1)) >> bit_log2(divisor)) + uint64_t(1); }

constexpr void set_bit(uint8_t& val, uint32_t bit_idx) { val |= (uint8_t(1) << bit_idx); }
constexpr void set_bit(uint16_t& val, uint32_t bit_idx) { val |= (uint16_t(1) << bit_idx); }
constexpr void set_bit(uint32_t& val, uint32_t bit_idx) { val |= (uint32_t(1) << bit_idx); }
constexpr void set_bit(uint64_t& val, uint32_t bit_idx) { val |= (uint64_t(1) << bit_idx); }

constexpr void unset_bit(uint8_t& val, uint32_t bit_idx) { val &= ~(uint8_t(1) << bit_idx); }
constexpr void unset_bit(uint16_t& val, uint32_t bit_idx) { val &= ~(uint16_t(1) << bit_idx); }
constexpr void unset_bit(uint32_t& val, uint32_t bit_idx) { val &= ~(uint32_t(1) << bit_idx); }
constexpr void unset_bit(uint64_t& val, uint32_t bit_idx) { val &= ~(uint64_t(1) << bit_idx); }

constexpr void flip_bit(uint8_t& val, uint32_t bit_idx) { val ^= (uint8_t(1) << bit_idx); }
constexpr void flip_bit(uint16_t& val, uint32_t bit_idx) { val ^= (uint16_t(1) << bit_idx); }
constexpr void flip_bit(uint32_t& val, uint32_t bit_idx) { val ^= (uint32_t(1) << bit_idx); }
constexpr void flip_bit(uint64_t& val, uint32_t bit_idx) { val ^= (uint64_t(1) << bit_idx); }

constexpr bool has_bit(uint8_t val, uint32_t bit_idx) { return (val & (uint8_t(1) << bit_idx)) != 0; }
constexpr bool has_bit(uint16_t val, uint32_t bit_idx) { return (val & (uint16_t(1) << bit_idx)) != 0; }
constexpr bool has_bit(uint32_t val, uint32_t bit_idx) { return (val & (uint32_t(1) << bit_idx)) != 0; }
constexpr bool has_bit(uint64_t val, uint32_t bit_idx) { return (val & (uint64_t(1) << bit_idx)) != 0; }
}
