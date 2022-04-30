#pragma once

// =========
// compiler

#if defined(_MSC_VER)
#define CC_COMPILER_MSVC
#elif defined(__clang__)
#define CC_COMPILER_CLANG
#elif defined(__GNUC__)
#define CC_COMPILER_GCC
#elif defined(__MINGW32__) || defined(__MINGW64__)
#define CC_COMPILER_MINGW
#else
#error "Unknown compiler"
#endif

#if defined(CC_COMPILER_CLANG) || defined(CC_COMPILER_GCC) || defined(CC_COMPILER_MINGW)
#define CC_COMPILER_POSIX
#endif

// =========
// compilation modes

#ifdef CC_COMPILER_MSVC
#ifdef _CPPRTTI
#define CC_HAS_RTTI
#endif
#ifdef _CPPUNWIND
#define CC_HAS_CPP_EXCEPTIONS
#endif
#elif defined(CC_COMPILER_CLANG)
#if __has_feature(cxx_rtti)
#define CC_HAS_RTTI
#endif
#if __EXCEPTIONS && __has_feature(cxx_exceptions)
#define CC_HAS_CPP_EXCEPTIONS
#endif
#elif defined(CC_COMPILER_GCC)
#ifdef __GXX_RTTI
#define CC_HAS_RTTI
#endif
#if __EXCEPTIONS
#define CC_HAS_CPP_EXCEPTIONS
#endif
#endif

// =========
// operating system

// CC_OS_WINDOWS, CC_OS_LINUX, CC_OS_OSX, or CC_OS_IOS
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32)
#define CC_OS_WINDOWS
#elif defined(__APPLE__)
#include "TargetConditionals.h"
#if defined(TARGET_OS_MAC)
#define CC_OS_OSX
#elif defined(TARGET_OS_IPHONE)
#define CC_OS_IOS
#else
#error "Unknown Apple platform"
#endif
#elif defined(__linux__)
#define CC_OS_LINUX
#else
#error "Unknown platform"
#endif


// =========
// compiler specific builtins

#if defined(CC_COMPILER_MSVC)

#define CC_PRETTY_FUNC __FUNCTION__

#define CC_FORCE_INLINE __forceinline
#define CC_DONT_INLINE __declspec(noinline)

// WARNING: deprecated, use CC_CONDITION_LIKELY
#define CC_LIKELY(x) x
#define CC_UNLIKELY(x) x

// these are supported in MSVC since March 2021, valid in C++14 and up (custom attributes)
// usage: if CC_CONDITION_LIKELY(foo) { /*...*/ }
#define CC_CONDITION_LIKELY(x) (x) [[msvc::likely]]
#define CC_CONDITION_UNLIKELY(x) (x) [[msvc::unlikely]]
#define CC_COLD_FUNC
#define CC_HOT_FUNC __declspec(safebuffers, spectre(nomitigation))

#define CC_BUILTIN_UNREACHABLE __assume(0)
#define CC_COUNTOF(arr) _countof(arr)
#define CC_ASSUME(x) __assume(x)

#elif defined(CC_COMPILER_POSIX)

#define CC_PRETTY_FUNC __PRETTY_FUNCTION__

// additional 'inline' is required on gcc and makes no difference on clang
#define CC_FORCE_INLINE __attribute__((always_inline)) inline
#define CC_DONT_INLINE __attribute__((noinline))

// WARNING: deprecated, use CC_CONDITION_LIKELY
#define CC_LIKELY(x) __builtin_expect((x), 1)
#define CC_UNLIKELY(x) __builtin_expect((x), 0)

// usage: if CC_CONDITION_LIKELY(foo) { /*...*/ }
#define CC_CONDITION_LIKELY(x) (__builtin_expect((x), 1))
#define CC_CONDITION_UNLIKELY(x) (__builtin_expect((x), 0))
#define CC_COLD_FUNC __attribute__((cold))
#define CC_HOT_FUNC __attribute__((hot))

#define CC_BUILTIN_UNREACHABLE __builtin_unreachable()
#define CC_COUNTOF(arr) (sizeof(arr) / sizeof(arr[0]))
#if defined(CC_COMPILER_CLANG)
#define CC_ASSUME(x) __builtin_assume(x)
#else
#define CC_ASSUME(x) ((!x) ? __builtin_unreachable() : void(0))
#endif

#else
#error "Unknown compiler"
#endif

// =========
// compiler/code model helpers

#if defined(CC_COMPILER_POSIX) || defined(__clang__) || defined(__GNUC__)
// even if code isn't compiled on a POSIX compiler, these helpers can still
// be active in a clang code model (eg MSVC via Qt Creator)

// enables warnings/errors on malformed calls to a printf-like function
// indices start at 1 and count the implicit 'this'-argument of methods
#define CC_PRINTF_FUNC(_fmt_index_) __attribute__((format(printf, _fmt_index_, _fmt_index_ + 1)))

#else

#define CC_PRINTF_FUNC(_fmt_index_)

#endif

// =========
// common helper

#define CC_DETAIL_MACRO_JOIN(arg1, arg2) arg1##arg2
#define CC_MACRO_JOIN(arg1, arg2) CC_DETAIL_MACRO_JOIN(arg1, arg2)

#define CC_UNUSED(expr) void(sizeof((expr))) // force ;

#define CC_FORCE_SEMICOLON static_assert(true) // force ;
