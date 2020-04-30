#pragma once

#include <clean-core/macros.hh>
#include <clean-core/typedefs.hh>

#ifdef CC_COMPILER_MSVC
#include <intrin.h>
#endif

namespace cc
{
#ifdef CC_COMPILER_MSVC

inline int popcount(uint8 v) { return int(__popcnt16(v)); }
inline int popcount(uint16 v) { return int(__popcnt16(v)); }
inline int popcount(uint32 v) { return int(__popcnt(v)); }
inline int popcount(uint64 v) { return int(__popcnt64(v)); }

inline int count_trailing_zeros(uint8 v)
{
    unsigned long idx;
    _BitScanForward(&idx, v);
    return int(idx);
}
inline int count_trailing_zeros(uint16 v)
{
    unsigned long idx;
    _BitScanForward(&idx, v);
    return int(idx);
}
inline int count_trailing_zeros(uint32 v)
{
    unsigned long idx;
    _BitScanForward(&idx, v);
    return int(idx);
}
inline int count_trailing_zeros(uint64 v)
{
    unsigned long idx;
    _BitScanForward64(&idx, v);
    return int(idx);
}

inline int count_leading_zeros(uint8 v) { return int(__lzcnt16(v) - 8); }
inline int count_leading_zeros(uint16 v) { return int(__lzcnt16(v)); }
inline int count_leading_zeros(uint32 v) { return int(__lzcnt(v)); }
inline int count_leading_zeros(uint64 v) { return int(__lzcnt64(v)); }

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

// returns rounded down logarithm to base 2
inline uint32 bit_log2(uint32 v) { return uint32(8 * sizeof(uint32) - count_leading_zeros(v) - 1); }
inline uint64 bit_log2(uint64 v) { return uint64(8 * sizeof(uint64) - count_leading_zeros(v) - 1); }
}
