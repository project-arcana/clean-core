#pragma once

#include <cstdint>

#include <clean-core/macros.hh>

namespace cc
{
CC_FORCE_INLINE int64_t get_high_precision_time();
CC_FORCE_INLINE int64_t get_high_precision_frequency();
}

#ifdef CC_OS_WINDOWS

#include <clean-core/native/win32_sanitized.hh>

CC_FORCE_INLINE int64_t cc::get_high_precision_time()
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

#elif defined(CC_OS_LINUX) || defined(CC_OS_APPLE)

#include <pthread.h>
#include <sys/time.h>
#include <sys/types.h>

CC_FORCE_INLINE int64_t cc::get_high_precision_time()
{
    ::timespec ts;
    ::clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000000000LL + ts.tv_nsec;
}

CC_FORCE_INLINE int64_t cc::get_high_precision_frequency() { return 1000000000LL; }

#else
#error "Unsupported platform"
#endif
