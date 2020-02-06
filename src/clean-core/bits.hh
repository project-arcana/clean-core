#pragma once

#include <clean-core/macros.hh>
#include <clean-core/typedefs.hh>

#ifdef CC_COMPILER_MSVC
#include <intrin.h>
#endif

namespace cc
{
// Divide ints and round up
// a > 0, b > 0
template <class T>
constexpr T int_div_ceil(T a, T b)
{
    return 1 + ((a - 1) / b);
}

#ifdef CC_COMPILER_MSVC

inline int popcount(uint8 v) { return static_cast<int>(__popcnt16(v)); }
inline int popcount(uint16 v) { return static_cast<int>(__popcnt16(v)); }
inline int popcount(uint32 v) { return static_cast<int>(__popcnt(v)); }
inline int popcount(uint64 v) { return static_cast<int>(__popcnt64(v)); }

inline int count_trailing_zeros(uint8 v)
{
    unsigned long idx;
    _BitScanForward(&idx, v);
    return static_cast<int>(idx);
}
inline int count_trailing_zeros(uint16 v)
{
    unsigned long idx;
    _BitScanForward(&idx, v);
    return static_cast<int>(idx);
}
inline int count_trailing_zeros(uint32 v)
{
    unsigned long idx;
    _BitScanForward(&idx, v);
    return static_cast<int>(idx);
}
inline int count_trailing_zeros(uint64 v)
{
    unsigned long idx;
    _BitScanForward64(&idx, v);
    return static_cast<int>(idx);
}

inline int count_leading_zeros(uint8 v) { return static_cast<int>(__lzcnt16(v) - 8); }
inline int count_leading_zeros(uint16 v) { return static_cast<int>(__lzcnt16(v)); }
inline int count_leading_zeros(uint32 v) { return static_cast<int>(__lzcnt(v)); }
inline int count_leading_zeros(uint64 v) { return static_cast<int>(__lzcnt64(v)); }

#else

inline int popcount(uint8 v) { return __builtin_popcount(v); }
inline int popcount(uint16 v) { return __builtin_popcount(v); }
inline int popcount(uint32 v) { return __builtin_popcount(v); }
inline int popcount(uint64 v) { return __builtin_popcountll(v); }

inline int count_trailing_zeros(uint8 v) { return __builtin_ctz(v); }
inline int count_trailing_zeros(uint16 v) { return __builtin_ctz(v); }
inline int count_trailing_zeros(uint32 v) { return __builtin_ctz(v); }
inline int count_trailing_zeros(uint64 v) { return __builtin_ctzll(v); }

inline int count_leading_zeros(uint8 v) { return __builtin_clz(v) - 24; }
inline int count_leading_zeros(uint16 v) { return __builtin_clz(v) - 16; }
inline int count_leading_zeros(uint32 v) { return __builtin_clz(v); }
inline int count_leading_zeros(uint64 v) { return __builtin_clzll(v); }

#endif
}
