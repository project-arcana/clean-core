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
CC_FORCE_INLINE uint64_t intrin_rdtsc()
{
#ifdef CC_COMPILER_GCC
    unsigned int lo, hi;
    __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) | lo;
#else // Clang, MSVC
    return __rdtsc();
#endif
}

CC_FORCE_INLINE uint32_t intrin_cas(uint32_t volatile* dst, uint32_t cmp, uint32_t exc)
{
#ifdef CC_COMPILER_MSVC
    return _InterlockedCompareExchange((volatile long*)dst, exc, cmp);
#else
    return __sync_val_compare_and_swap(dst, cmp, exc);
#endif
}

CC_FORCE_INLINE uint64_t intrin_cas(uint64_t volatile* dst, uint64_t cmp, uint64_t exc)
{
#ifdef CC_COMPILER_MSVC
    return _InterlockedCompareExchange64((volatile long long*)dst, exc, cmp);
#else
    return __sync_val_compare_and_swap(dst, cmp, exc);
#endif
}

CC_FORCE_INLINE int32_t intrin_atomic_add(int32_t volatile* counter, int32_t value)
{
#ifdef CC_COMPILER_MSVC
    return _InterlockedExchangeAdd((volatile long*)counter, value);
#else
    return __sync_fetch_and_add(counter, value);
#endif
}

CC_FORCE_INLINE int64_t intrin_atomic_add(int64_t volatile* counter, int64_t value)
{
#ifdef CC_COMPILER_MSVC
    return _InterlockedExchangeAdd64(counter, value);
#else
    return __sync_fetch_and_add(counter, value);
#endif
}

// approximate inverse square root
// maximum relative error < 0.000366
// about 5x faster than 1.f / std::sqrt(x)
CC_FORCE_INLINE float intrin_rsqrt(float x)
{
    // docs:
    // https://software.intel.com/sites/landingpage/IntrinsicsGuide/#text=_mm_rsqrt_ss&expand=4805

    // performance and codegen vs full computation:
    // https://godbolt.org/z/GhxG6fo3j https://quick-bench.com/q/_YlmyvyGpu4zb9lavwzsDy8VqWk

    __m128 val = _mm_set_ss(x);
    val = _mm_rsqrt_ss(val);
    return _mm_cvtss_f32(val);
}

// approximate inverse square root with one Newton-Raphson iteration
// about 2.5x faster than 1.f / std::sqrt(x)
CC_FORCE_INLINE float intrin_rsqrt_nr1(float x)
{
    __m128 const point_five = _mm_set_ss(0.5f);
    __m128 y_0 = _mm_set_ss(x);
    __m128 x_0 = _mm_rsqrt_ss(y_0);
    __m128 x_half = _mm_mul_ss(y_0, point_five);

    // doing the following in SSE regs as well gives slightly better codegen
    // Newton-Raphson iteration
    __m128 x_1 = _mm_mul_ss(x_0, x_0);
    x_1 = _mm_sub_ss(point_five, _mm_mul_ss(x_half, x_1));
    x_1 = _mm_add_ss(x_0, _mm_mul_ss(x_0, x_1));

    float res;
    _mm_store_ss(&res, x_1);
    return res;
}

// approximate inverse square root with two Newton-Raphson iterations
// about 1.5x faster than 1.f / std::sqrt(x)
CC_FORCE_INLINE float intrin_rsqrt_nr2(float x)
{
    __m128 const point_five = _mm_set_ss(0.5f);
    __m128 y_0 = _mm_set_ss(x);
    __m128 x_0 = _mm_rsqrt_ss(y_0);
    __m128 x_half = _mm_mul_ss(y_0, point_five);

    // doing the following in SSE regs as well gives slightly better codegen
    // Newton-Raphson iteration 1
    __m128 x_1 = _mm_mul_ss(x_0, x_0);
    x_1 = _mm_sub_ss(point_five, _mm_mul_ss(x_half, x_1));
    x_1 = _mm_add_ss(x_0, _mm_mul_ss(x_0, x_1));

    // Newton-Raphson iteration 2
    __m128 x_2 = _mm_mul_ss(x_1, x_1);
    x_2 = _mm_sub_ss(point_five, _mm_mul_ss(x_half, x_2));
    x_2 = _mm_add_ss(x_1, _mm_mul_ss(x_1, x_2));

    float res;
    _mm_store_ss(&res, x_2);
    return res;
}

inline bool test_cpuid_register(int level, int register_index, int bit_index)
{
#ifdef CC_COMPILER_MSVC
    int info[4];
    __cpuid(info, level);
    return (info[register_index] >> bit_index) != 0;
#else
    unsigned info[4];
    __get_cpuid(level, &info[0], &info[1], &info[2], &info[3]);
    return (info[register_index] >> bit_index) != 0;
#endif
}

// returns true if the executing CPU has support for LZCNT
// Intel: Haswell (4th gen Core-i, 2013), AMD: Piledriver (ABM, 2012)
inline bool test_cpu_support_lzcnt() { return test_cpuid_register(0x80000001, 2, 5); }

// returns true if the executing CPU has support for POPCNT
inline bool test_cpu_support_popcount() { return test_cpuid_register(0x00000001, 2, 23); }


}
