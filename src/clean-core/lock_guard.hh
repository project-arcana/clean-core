#pragma once

#include <clean-core/macros.hh>

namespace cc
{
template <typename T>
struct [[nodiscard]] lock_guard
{
    CC_FORCE_INLINE explicit lock_guard(T& mutex) : _lock(mutex) { _lock.lock(); }
    CC_FORCE_INLINE ~lock_guard() { _lock.unlock(); }

    lock_guard(lock_guard const&) = delete;
    lock_guard(lock_guard&&) noexcept = delete;
    lock_guard& operator=(lock_guard const&) = delete;
    lock_guard& operator=(lock_guard&&) noexcept = delete;

private:
    T& _lock;
};
}
