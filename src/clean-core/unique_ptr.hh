#pragma once

#include <clean-core/allocate.hh>
#include <clean-core/always_false.hh>
#include <clean-core/assert.hh>
#include <clean-core/forward.hh>
#include <clean-core/fwd.hh>
#include <clean-core/typedefs.hh>

namespace cc
{
/**
 * - single-owner heap-allocated object
 * - move-only type
 * - always uses cc::alloc/cc::free
 * - can only be constructed via make_unique<T>(...)
 * - no polymorphic behavior
 *
 * changes to std::unique_ptr<T>:
 * - no custom deleter
 * - no allocators
 * - no operator<
 * - no operator bool
 * - no T[]
 * - no reset/release
 */
template <class T>
struct unique_ptr
{
    unique_ptr() = default;

    unique_ptr(unique_ptr const&) = delete;
    unique_ptr& operator=(unique_ptr const&) = delete;

    unique_ptr(unique_ptr&& rhs) noexcept
    {
        _ptr = rhs._ptr;
        rhs._ptr = nullptr;
    }
    unique_ptr& operator=(unique_ptr&& rhs) noexcept
    {
        // self-move is reset
        if (_ptr)
            cc::free(_ptr);
        _ptr = rhs._ptr;
        rhs._ptr = nullptr;
        return *this;
    }

    ~unique_ptr()
    {
        static_assert(sizeof(T) > 0, "cannot delete incomplete class");
        if (_ptr)
            cc::free(_ptr);
    }

    [[nodiscard]] T* get() const { return _ptr; }

    [[nodiscard]] T* operator->() const
    {
        CC_ASSERT_NOT_NULL(_ptr);
        return _ptr;
    }
    [[nodiscard]] T& operator*() const
    {
        CC_ASSERT_NOT_NULL(_ptr);
        return *_ptr;
    }

    [[nodiscard]] bool operator==(unique_ptr const& rhs) const { return _ptr == rhs._ptr; }
    [[nodiscard]] bool operator!=(unique_ptr const& rhs) const { return _ptr != rhs._ptr; }
    [[nodiscard]] bool operator==(T const* rhs) const { return _ptr == rhs; }
    [[nodiscard]] bool operator!=(T const* rhs) const { return _ptr != rhs; }

    template <typename U, typename... Args>
    friend unique_ptr<U> make_unique(Args&&... args);

private:
    T* _ptr = nullptr;
};

template <class T>
struct unique_ptr<T[]>
{
    static_assert(always_false<T>, "unique_ptr does not support arrays, use cc::vector or cc::array instead");
};

template <class T>
[[nodiscard]] bool operator==(T const* lhs, unique_ptr<T> const& rhs)
{
    return lhs == rhs.get();
}
template <class T>
[[nodiscard]] bool operator==(nullptr_t, unique_ptr<T> const& rhs)
{
    return rhs.get() == nullptr;
}

template <typename T, typename... Args>
[[nodiscard]] unique_ptr<T> make_unique(Args&&... args)
{
    unique_ptr<T> p;
    p._ptr = cc::alloc<T>(cc::forward<Args>(args)...);
    return p;
}

template <class T>
struct hash<unique_ptr<T>>
{
    [[nodiscard]] hash_t operator()(unique_ptr<T> const& v) const noexcept { return hash_t(v.get()); }
};

template <class T>
struct less<unique_ptr<T>>
{
    [[nodiscard]] bool operator()(unique_ptr<T> const& a, unique_ptr<T> const& b) const noexcept { return a.get() < b.get(); }
};
}
