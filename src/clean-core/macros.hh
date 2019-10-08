#pragma once

// TODO: use [[likely]] and [[unlikely]] in C++20


// =========
// configurations

#if defined(_MSC_VER)
#define CC_COMPILER_MSVC
#elif defined(__clang__)
#define CC_COMPILER_CLANG
#elif defined(__GNUC__)
#define CC_COMPILER_GCC
#else
#error "Unknown compiler"
#endif


// =========
// compiler specific builtins

#if defined(CC_COMPILER_MSVC)

#define CC_PRETTY_FUNC __FUNCTION__

#define CC_FORCE_INLINE __forceinline
#define CC_DONT_INLINE __declspec(noinline)

#define CC_LIKELY(x) x
#define CC_UNLIKELY(x) x
#define CC_COLD_FUNC


#elif defined(CC_COMPILER_CLANG) || defined(CC_COMPILER_GCC)

#define CC_PRETTY_FUNC __PRETTY_FUNCTION__

#define CC_FORCE_INLINE __attribute__((always_inline))
#define CC_DONT_INLINE __attribute__((noinline))

#define CC_LIKELY(x) __builtin_expect((x), 1)
#define CC_UNLIKELY(x) __builtin_expect((x), 0)
#define CC_COLD_FUNC __attribute__((cold))

#else
#error "Unknown compiler"
#endif


// =========
// common helper

#define CC_DETAIL_MACRO_JOIN(arg1, arg2) arg1##arg2
#define CC_MACRO_JOIN(arg1, arg2) CC_DETAIL_MACRO_JOIN(arg1, arg2)

#define CC_UNUSED(expr) void(sizeof(bool((expr)))) // force ;

#define CC_FORCE_SEMICOLON static_assert(true) // force ;
