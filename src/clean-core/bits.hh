#pragma once

#include <clean-core/macros.hh>
#include <clean-core/typedefs.hh>

#ifdef CC_COMPILER_MSVC
#include <intrin.h>
#else
#include <cpuid.h>
#include <x86intrin.h>
#endif


namespace cc
{
#ifdef CC_COMPILER_MSVC

inline int popcount(uint8 v) { return int(__popcnt16(v)); }
inline int popcount(uint16 v) { return int(__popcnt16(v)); }
inline int popcount(uint32 v) { return int(__popcnt(v)); }
inline int popcount(uint64 v) { return int(__popcnt64(v)); }

// NOTE: the _lzcnt_u32/_tzcnt_u32 intrinsics don't care about compilation target arch, they always output T/LZCNT
// however LZCNT, on an assembly level, falls back to plain BSR if LZCNT isn't supported
// if unsupported, we work around this using (explicit) BSR, XOR, SUB and a branch on 0
// LZCNT support: Intel: Haswell (4th gen Core-i, 2013), AMD: Piledriver (ABM, 2012)
// TZCNT is de-facto always supported if LZCNT is, but it's nominally part of BMI1
// MSVC has no macro for LZCNT/BMI1 specifically, but AVX2 always includes it
// only a few AMD CPUs after piledriver (pre-Zen) are incorrectly excluded by this approximation
#ifdef __AVX2__
inline int count_trailing_zeros(uint32 v) { return int(_tzcnt_u32(v)); }
inline int count_trailing_zeros(uint64 v) { return int(_tzcnt_u64(v)); }

inline int count_leading_zeros(uint8 v) { return int(_lzcnt_u32(v) - 24); }
inline int count_leading_zeros(uint16 v) { return int(_lzcnt_u32(v) - 16); }
inline int count_leading_zeros(uint32 v) { return int(_lzcnt_u32(v)); }
inline int count_leading_zeros(uint64 v) { return int(_lzcnt_u64(v)); }
#else
inline int count_trailing_zeros(uint32 v)
{
    if (v == 0)
        return 32;
    unsigned long idx;
    _BitScanForward(&idx, v);
    return int(idx);
}
inline int count_trailing_zeros(uint64 v)
{
    if (v == 0)
        return 64;
    unsigned long idx;
    _BitScanForward64(&idx, v);
    return int(idx);
}

inline int count_leading_zeros(uint8 v)
{
    if (v == 0)
        return 8;
    unsigned long idx;
    _BitScanReverse(&idx, v);
    return int(idx ^ 31u) - 24;
}
inline int count_leading_zeros(uint16 v)
{
    if (v == 0)
        return 16;
    unsigned long idx;
    _BitScanReverse(&idx, v);
    return int(idx ^ 31u) - 16;
}
inline int count_leading_zeros(uint32 v)
{
    if (v == 0)
        return 32;
    unsigned long idx;
    _BitScanReverse(&idx, v);
    return int(idx ^ 31u);
}
inline int count_leading_zeros(uint64 v)
{
    if (v == 0)
        return 64;
    unsigned long idx;
    _BitScanReverse64(&idx, v);
    return int(idx ^ 63u);
}
#endif

inline bool test_cpuid_register(int level, int register_index, int bit_index)
{
    int info[4];
    __cpuid(info, level);
    return (info[register_index] >> bit_index) != 0;
}

#else

inline int popcount(uint8 v) { return __builtin_popcount(v); }
inline int popcount(uint16 v) { return __builtin_popcount(v); }
inline int popcount(uint32 v) { return __builtin_popcount(v); }
inline int popcount(uint64 v) { return __builtin_popcountll(v); }

#ifdef __BMI__
inline int count_trailing_zeros(uint32 v) { return int(_tzcnt_u32(v)); }
inline int count_trailing_zeros(uint64 v) { return int(_tzcnt_u64(v)); }
#else
inline int count_trailing_zeros(uint32 v) { return v ? __builtin_ctz(v) : 32; }
inline int count_trailing_zeros(uint64 v) { return v ? __builtin_ctzll(v) : 64; }
#endif

// NOTE: __builtin_clz by default compiles to "BSR <ret>, <val>; XOR <ret>, 31"
// Only with -mlzcnt (or -march=...) does it compile to LZCNT proper.
// This causes inconsistent behavior with 0 as BSRs result is unspecified if <val> is zero
// __LZCNT__ is defined if compiling with a target arch that supports LZCNT (like AVX2)
// if not, we work around this using BSR, XOR (as part of __builtin_clz), SUB, and a branch on 0 (explicit)
#ifdef __LZCNT__
inline int count_leading_zeros(uint8 v) { return int(__lzcnt16(v) - 8); }
inline int count_leading_zeros(uint16 v) { return int(__lzcnt16(v)); }
inline int count_leading_zeros(uint32 v) { return int(__lzcnt32(v)); }
inline int count_leading_zeros(uint64 v) { return int(__lzcnt64(v)); }
#else
inline int count_leading_zeros(uint8 v) { return v ? __builtin_clz(v) - 24 : 8; }
inline int count_leading_zeros(uint16 v) { return v ? __builtin_clz(v) - 16 : 16; }
inline int count_leading_zeros(uint32 v) { return v ? __builtin_clz(v) : 32; }
inline int count_leading_zeros(uint64 v) { return v ? __builtin_clzll(v) : 64; }
#endif

inline bool test_cpuid_register(int level, int register_index, int bit_index)
{
    unsigned info[4];
    __get_cpuid(level, &info[0], &info[1], &info[2], &info[3]);
    return (info[register_index] >> bit_index) != 0;
}
#endif

// returns true if the executing CPU has support for LZCNT
// Intel: Haswell (4th gen Core-i, 2013), AMD: Piledriver (ABM, 2012)
inline bool test_cpu_support_lzcnt() { return test_cpuid_register(0x80000001, 2, 5); }
// returns true if the executing CPU has support for POPCNT
inline bool test_cpu_support_popcount() { return test_cpuid_register(0x00000001, 2, 23); }

// returns rounded down logarithm to base 2
inline uint32 bit_log2(uint32 v) { return uint32(8 * sizeof(uint32) - count_leading_zeros(v) - 1); }
inline uint64 bit_log2(uint64 v) { return uint64(8 * sizeof(uint64) - count_leading_zeros(v) - 1); }

// ceils to the nearest power of 2
inline uint32 ceil_pow2(uint32 v) { return uint32(1) << (bit_log2(v - uint32(1)) + 1); }
inline uint64 ceil_pow2(uint64 v) { return uint64(1) << (bit_log2(v - uint64(1)) + 1); }

// returns true if v is a power of 2
constexpr bool is_pow2(uint32 v) { return ((v & (v - uint32(1))) == 0); }
constexpr bool is_pow2(uint64 v) { return ((v & (v - uint64(1))) == 0); }

// computes v % divisor, divisor must be a power of 2
constexpr uint32 mod_pow2(uint32 v, uint32 divisor) { return v & (divisor - 1); }
constexpr uint64 mod_pow2(uint64 v, uint64 divisor) { return v & (divisor - 1); }

constexpr void set_bit(uint8& val, uint32 bit_idx) { val |= (uint8(1) << bit_idx); }
constexpr void set_bit(uint16& val, uint32 bit_idx) { val |= (uint16(1) << bit_idx); }
constexpr void set_bit(uint32& val, uint32 bit_idx) { val |= (uint32(1) << bit_idx); }
constexpr void set_bit(uint64& val, uint32 bit_idx) { val |= (uint64(1) << bit_idx); }

constexpr void unset_bit(uint8& val, uint32 bit_idx) { val &= ~(uint8(1) << bit_idx); }
constexpr void unset_bit(uint16& val, uint32 bit_idx) { val &= ~(uint16(1) << bit_idx); }
constexpr void unset_bit(uint32& val, uint32 bit_idx) { val &= ~(uint32(1) << bit_idx); }
constexpr void unset_bit(uint64& val, uint32 bit_idx) { val &= ~(uint64(1) << bit_idx); }

constexpr void flip_bit(uint8& val, uint32 bit_idx) { val ^= (uint8(1) << bit_idx); }
constexpr void flip_bit(uint16& val, uint32 bit_idx) { val ^= (uint16(1) << bit_idx); }
constexpr void flip_bit(uint32& val, uint32 bit_idx) { val ^= (uint32(1) << bit_idx); }
constexpr void flip_bit(uint64& val, uint32 bit_idx) { val ^= (uint64(1) << bit_idx); }
}
