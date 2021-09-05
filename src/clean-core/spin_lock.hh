#pragma once

#include <atomic>

#include <immintrin.h>

#include <clean-core/macros.hh>

namespace cc
{
// test-and-test-and-set (TTAS) spinlock
struct spin_lock
{
    CC_FORCE_INLINE void lock() noexcept
    {
        while (true)
        {
            // immediately try to exchange
            // memory order: locking acquires, unlocking releases
            if (_is_locked.exchange(true, std::memory_order_acquire) == false)
            {
                // exhange returned false, meaning the lock was previously unlocked, success
                return;
            }

            // exchange failed, wait until the value is false without forcing cache misses
            while (_is_locked.load(std::memory_order_relaxed))
            {
                // x86 PAUSE to signal spin-wait, improve interleaving
                _mm_pause();
            }
        }
    }

    CC_FORCE_INLINE bool try_lock() noexcept
    {
        // early out using a relaxed load to improve performance when spinning on try_lock()
        // ref: https://rigtorp.se/spinlock/
        return !_is_locked.load(std::memory_order_relaxed) //
               && !_is_locked.exchange(true, std::memory_order_acquire);
    }

    CC_FORCE_INLINE void unlock() noexcept
    {
        // release
        _is_locked.store(false, std::memory_order_release);
    }

    spin_lock() = default;
    spin_lock(spin_lock const& other) = delete;
    spin_lock(spin_lock&& other) noexcept = delete;
    spin_lock& operator=(spin_lock const& other) = delete;
    spin_lock& operator=(spin_lock&& other) noexcept = delete;

private:
    std::atomic_bool _is_locked = {false};
};

// simple replacement for std::lock_guard
template <typename T>
struct lock_guard
{
    CC_FORCE_INLINE explicit lock_guard(T& mutex) : _mutex(mutex) { _mutex.lock(); }
    CC_FORCE_INLINE ~lock_guard() { _mutex.unlock(); }

    lock_guard(lock_guard const&) = delete;
    lock_guard(lock_guard&&) noexcept = delete;
    lock_guard& operator=(lock_guard const&) = delete;
    lock_guard& operator=(lock_guard&&) noexcept = delete;

private:
    T& _mutex;
};
}
