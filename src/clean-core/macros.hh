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

// from CMake:
// CC_DEBUG is defined in debug configurations
// CC_RELEASE in release configurations
// CC_RELWITHDEBINFO in release-with-debug

// =========
// operating systems
// CC_OS_WINDOWS, CC_OS_LINUX, CC_OS_APPLE, or CC_OS_BSD

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32)
#define CC_OS_WINDOWS
#elif defined(__APPLE__) || defined(__MACH__) || defined(macintosh)
#define CC_OS_APPLE
#elif defined(__linux__) || defined(linux)
#define CC_OS_LINUX
#elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
#define CC_OS_BSD
#else
#error "Unknown platform"
#endif

// =========
// targets

#if defined(CC_OS_WINDOWS)
#if defined(_DURANGO)
#define CC_TARGET_XBOX
#else
#define CC_TARGET_PC
#endif
#endif

#if defined(CC_OS_APPLE)
#include "TargetConditionals.h"
#if TARGET_OS_MAC
#define CC_TARGET_MACOS
#elif TARGET_OS_IOS
#define CC_TARGET_IOS
#elif TARGET_OS_TV
#define CC_TARGET_TVOS
#else
#error "Unknown Apple platform"
#endif
#endif

#if defined(__ANDROID__)
#define CC_TARGET_ANDROID
#endif

#if defined(__ORBIS__)
#define CC_TARGET_ORBIS
#endif

#if defined(__NX__)
#define CC_TARGET_NX
#endif

#if defined(CC_TARGET_IOS) || defined(CC_TARGET_TVOS) || defined(CC_TARGET_ANDROID)
#define CC_TARGET_MOBILE
#endif

#if defined(CC_TARGET_ORBIS) || defined(CC_TARGET_NX) || defined(CC_TARGET_XBOX)
#define CC_TARGET_CONSOLE
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
#define CC_HOT_FUNC

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

#ifdef CC_DEBUG
#define CC_FORCE_INLINE_DEBUGGABLE inline
#else
#define CC_FORCE_INLINE_DEBUGGABLE CC_FORCE_INLINE
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
