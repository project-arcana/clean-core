#pragma once

#include <cstdint>

#include <clean-core/macros.hh>

namespace cc
{
// returns a high precision platform specific tick counter
// divide by frequency to convert to seconds
CC_FORCE_INLINE int64_t get_high_precision_ticks();

// returns the amount of ticks per second
CC_FORCE_INLINE int64_t get_high_precision_frequency();
} // namespace cc

#ifdef CC_OS_WINDOWS

#include <clean-core/native/win32_sanitized.hh>

CC_FORCE_INLINE int64_t cc::get_high_precision_ticks()
{
    ::LARGE_INTEGER timestamp;
    ::QueryPerformanceCounter(&timestamp);
    return timestamp.QuadPart;
}

CC_FORCE_INLINE int64_t cc::get_high_precision_frequency()
{
    ::LARGE_INTEGER frequency;
    ::QueryPerformanceFrequency(&frequency);
    return frequency.QuadPart;
}

#elif defined(CC_OS_LINUX)

#include <pthread.h>
#include <sys/time.h>
#include <sys/types.h>

CC_FORCE_INLINE int64_t cc::get_high_precision_ticks()
{
    ::timespec ts;
    ::clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000000000LL + ts.tv_nsec;
}

CC_FORCE_INLINE int64_t cc::get_high_precision_frequency() { return 1000000000LL; }

#elif defined(CC_OS_APPLE)

#include <sys/time.h>
#include <time.h>

#ifdef __MACH__
#include <mach/clock.h>
#include <mach/mach.h>
#endif

CC_FORCE_INLINE int64_t cc::get_high_precision_ticks()
{
#ifdef __MACH__ // OS X does not have clock_gettime, use clock_get_time
    // https://developer.apple.com/documentation/kernel/1646199-mach_continuous_time
    return mach_continuous_time();
#else
    ::timespec ts;
    ::clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000000000LL + ts.tv_nsec;
#endif
}

CC_FORCE_INLINE int64_t cc::get_high_precision_frequency() { return 1000000000LL; }

#else
#error "Unsupported platform"
#endif
