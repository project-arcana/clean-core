#pragma once

#include <clean-core/macros.hh>
#include <cstring>
#include <cstdint>

#ifdef CC_ARCH_X86_64

#ifdef CC_COMPILER_MSVC
#include <intrin.h>

#else // clang, gcc

#ifndef __cpuid
// NOTE: this file does not (always) have include guards
#include <cpuid.h>
#endif

#include <x86intrin.h>

#endif

#elif defined(CC_ARCH_ARM64)
#include <arm_neon.h>
#include <cstddef>
#include <cstdint>

#endif

namespace cc
{
CC_FORCE_INLINE uint64_t intrin_rdtsc()
{
#ifdef CC_ARCH_X86_64

#ifdef CC_COMPILER_GCC
    unsigned int lo, hi;
    __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) | lo;
#else // Clang, MSVC
    return __rdtsc();
#endif

#elif defined(CC_ARCH_ARM64)
    uint64_t val;
    asm volatile("mrs %0, cntvct_el0" : "=r"(val));
    return val;
#else
#error "unsupported architecture"
#endif
}

CC_FORCE_INLINE int32_t intrin_cas(int32_t volatile* destination, int32_t comparand, int32_t exchange)
{
#ifdef CC_COMPILER_MSVC
    return _InterlockedCompareExchange((long volatile*)destination, exchange, comparand);
#else
    return __sync_val_compare_and_swap(destination, comparand, exchange);
#endif
}

CC_FORCE_INLINE int64_t intrin_cas(int64_t volatile* destination, int64_t comparand, int64_t exchange)
{
#ifdef CC_COMPILER_MSVC
    return _InterlockedCompareExchange64(destination, exchange, comparand);
#else
    return __sync_val_compare_and_swap(destination, comparand, exchange);
#endif
}

CC_FORCE_INLINE void* intrin_cas_pointer(void* volatile* destination, void* comparand, void* exchange)
{
#ifdef CC_COMPILER_MSVC
    return _InterlockedCompareExchangePointer(destination, exchange, comparand);
#else
    return __sync_val_compare_and_swap(destination, comparand, exchange);
#endif
}

CC_FORCE_INLINE int32_t intrin_atomic_swap(int32_t volatile* destination, int32_t value)
{
#ifdef CC_COMPILER_MSVC
    return _InterlockedExchange((long volatile*)destination, value);
#else
    int32_t res;
    __atomic_exchange(destination, &value, &res, __ATOMIC_SEQ_CST);
    return res;
#endif
}

CC_FORCE_INLINE int64_t intrin_atomic_swap(int64_t volatile* destination, int64_t value)
{
#ifdef CC_COMPILER_MSVC
    return _InterlockedExchange64(destination, value);
#else
    int64_t res;
    __atomic_exchange(destination, &value, &res, __ATOMIC_SEQ_CST);
    return res;
#endif
}

CC_FORCE_INLINE void* intrin_atomic_swap_pointer(void* volatile* destination, void* value)
{
#ifdef CC_COMPILER_MSVC
    return _InterlockedExchangePointer(destination, value);
#else
    void* res;
    __atomic_exchange(destination, &value, &res, __ATOMIC_SEQ_CST);
    return res;
#endif
}

CC_FORCE_INLINE int8_t intrin_atomic_add(int8_t volatile* counter, int8_t value)
{
#ifdef CC_COMPILER_MSVC
    return _InterlockedExchangeAdd8((char volatile*)counter, value);
#else
    return __sync_fetch_and_add(counter, value);
#endif
}

CC_FORCE_INLINE int16_t intrin_atomic_add(int16_t volatile* counter, int16_t value)
{
#ifdef CC_COMPILER_MSVC
    return _InterlockedExchangeAdd16((short volatile*)counter, value);
#else
    return __sync_fetch_and_add(counter, value);
#endif
}

CC_FORCE_INLINE int32_t intrin_atomic_add(int32_t volatile* counter, int32_t value)
{
#ifdef CC_COMPILER_MSVC
    return _InterlockedExchangeAdd((long volatile*)counter, value);
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

CC_FORCE_INLINE uint8_t intrin_atomic_or(uint8_t volatile* counter, uint8_t value)
{
#ifdef CC_COMPILER_MSVC
    return _InterlockedOr8((char volatile*)counter, value);
#else
    return __sync_fetch_and_or(counter, value);
#endif
}

CC_FORCE_INLINE uint16_t intrin_atomic_or(uint16_t volatile* counter, uint16_t value)
{
#ifdef CC_COMPILER_MSVC
    return _InterlockedOr16((short volatile*)counter, value);
#else
    return __sync_fetch_and_or(counter, value);
#endif
}

CC_FORCE_INLINE uint32_t intrin_atomic_or(uint32_t volatile* counter, uint32_t value)
{
#ifdef CC_COMPILER_MSVC
    return _InterlockedOr((long volatile*)counter, value);
#else
    return __sync_fetch_and_or(counter, value);
#endif
}

CC_FORCE_INLINE uint64_t intrin_atomic_or(uint64_t volatile* counter, uint64_t value)
{
#ifdef CC_COMPILER_MSVC
    return _InterlockedOr64((long long volatile*)counter, value);
#else
    return __sync_fetch_and_or(counter, value);
#endif
}

template <class T>
CC_FORCE_INLINE T* intrin_cas_pointer_t(T* volatile* destination, T* comparand, T* exchange)
{
    return static_cast<T*>(intrin_cas_pointer(reinterpret_cast<void* volatile*>(destination), comparand, exchange));
}

template <class T>
CC_FORCE_INLINE T* intrin_atomic_swap_pointer_t(T* volatile* destination, T* value)
{
    return static_cast<T*>(intrin_atomic_swap_pointer(reinterpret_cast<void* volatile*>(destination), value));
}

// PAUSE to signal spin-wait, improve interleaving
CC_FORCE_INLINE void intrin_pause()
{
#if defined(CC_ARCH_X86_64)
    // x86 PAUSE to signal spin-wait, improve interleaving
    _mm_pause();

#elif defined(CC_ARCH_ARM64)
    asm volatile("yield");
#endif
}

// Currently only supported for x86_64
#ifdef CC_ARCH_X86_64

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
#endif

inline bool test_cpuid_register(int level, int register_index, int bit_index)
{
#ifdef CC_ARCH_X86_64
#ifdef CC_COMPILER_MSVC
    int info[4];
    __cpuid(info, level);
    return (info[register_index] >> bit_index) != 0;
#else
    unsigned info[4];
    __get_cpuid(level, &info[0], &info[1], &info[2], &info[3]);
    return (info[register_index] >> bit_index) != 0;
#endif
#elif defined(CC_ARCH_ARM64)
    // not supported on arm
    return false;
#endif
}

// returns true if the executing CPU has support for LZCNT
// Intel: Haswell (4th gen Core-i, 2013), AMD: Piledriver (ABM, 2012)
inline bool test_cpu_support_lzcnt() { return test_cpuid_register(0x80000001, 2, 5); }

// returns true if the executing CPU has support for POPCNT
inline bool test_cpu_support_popcount() { return test_cpuid_register(0x00000001, 2, 23); }

/// computes *result = a + b + carry_in, returning the carry_out (0 or 1)
/// carry_in must be 0 or 1; carry_out is 1 iff the 64-bit sum overflows
CC_FORCE_INLINE uint64_t add_with_carry(uint64_t carry_in, uint64_t a, uint64_t b, uint64_t* result)
{
#if defined(CC_ARCH_X86_64)
    return _addcarry_u64((unsigned char)carry_in, a, b, result);
#elif defined(CC_ARCH_ARM64) // clang only
    uint64_t carry_out;
    *result = __builtin_addcll(a, b, carry_in, &carry_out);
    return carry_out;
#endif
}

/// computes *result = a - b - borrow_in, returning the borrow_out (0 or 1)
/// borrow_in must be 0 or 1; borrow_out is 1 iff the 64-bit subtraction underflows
CC_FORCE_INLINE uint64_t sub_with_borrow(uint64_t borrow_in, uint64_t a, uint64_t b, uint64_t* result)
{
#if defined(CC_ARCH_X86_64)
    return _subborrow_u64((unsigned char)borrow_in, a, b, result);
#elif defined(CC_ARCH_ARM64) // clang only
    uint64_t borrow_out;
    *result = __builtin_subcll(a, b, borrow_in, &borrow_out);
    return borrow_out;
#endif
}

/// computes the full 128-bit product of two unsigned 64-bit integers
/// returns the low 64 bits and writes the high 64 bits to *high_out
CC_FORCE_INLINE uint64_t umulh64(uint64_t a, uint64_t b, uint64_t* high_out)
{
#if defined(CC_COMPILER_MSVC)
    return _umul128(a, b, high_out);
#elif defined(CC_ARCH_X86_64)
    return _mulx_u64(a, b, high_out);
#elif defined(CC_ARCH_ARM64)
    auto ia = __uint128_t(a);
    auto ib = __uint128_t(b);
    auto i = ia * ib;
    *high_out = uint64_t(i >> 64);
    return uint64_t(i);
#endif
}

/// computes the full 128-bit product of two signed 64-bit integers
/// returns the low 64 bits and writes the high 64 bits to *high_out
CC_FORCE_INLINE uint64_t imulh64(uint64_t a, uint64_t b, uint64_t* high_out)
{
#if defined(CC_COMPILER_MSVC)
    int64_t high_signed;
    return _mul128(static_cast<int64_t>(a), static_cast<int64_t>(b), &high_signed);
    *high_out = static_cast<uint64_t>(high_signed);
#else // clang, gcc
    auto ia = __int128_t(int64_t(a));
    auto ib = __int128_t(int64_t(b));
    auto i = ia * ib;
    *high_out = uint64_t(i >> 64);
    return uint64_t(i);
#endif
}
} // namespace cc
