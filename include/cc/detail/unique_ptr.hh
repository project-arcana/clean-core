#pragma once

#include <cc/assert>
#include <cc/detail/error.hh>
#include <cc/fwd/hash.hh>
#include <cc/fwd/less.hh>
#include <cc/fwd/unique_ptr.hh>
#include <cc/typedefs>

namespace cc
{
/**
 * changes to std::unique_ptr<T>:
 * - no custom deleter
 * - no allocators
 * - no operator<
 * - no operator bool
 * - no T[]
 */
template <class T>
struct unique_ptr
{
    unique_ptr() = default;

    unique_ptr(unique_ptr const&) = delete;
    unique_ptr& operator=(unique_ptr const&) = delete;

    unique_ptr(unique_ptr&& rhs) noexcept
    {
        ptr = rhs.ptr;
        rhs.ptr = nullptr;
    }
    unique_ptr& operator=(unique_ptr&& rhs) noexcept
    {
        delete ptr;
        ptr = rhs.ptr;
        rhs.ptr = nullptr;
    }

    ~unique_ptr() { delete ptr; }

    void reset(T* p = nullptr) noexcept
    {
        CC_ASSERT_CONTRACT(p == nullptr || p != ptr); // no self-reset
        delete ptr;
        ptr = p;
    }

    [[nodiscard]] T* release() noexcept
    {
        auto p = ptr;
        ptr = nullptr;
        return p;
    }

    [[nodiscard]] T* get() const noexcept { return ptr; }

    T* operator->() const noexcept
    {
        CC_ASSERT_NOT_NULL(ptr);
        return ptr;
    }
    T& operator*() const noexcept
    {
        CC_ASSERT_NOT_NULL(ptr);
        return *ptr;
    }

    [[nodiscard]] bool operator==(unique_ptr const& rhs) const noexcept { return ptr == rhs.ptr; }
    [[nodiscard]] bool operator!=(unique_ptr const& rhs) const noexcept { return ptr != rhs.ptr; }
    [[nodiscard]] bool operator==(T const* rhs) const noexcept { return ptr == rhs; }
    [[nodiscard]] bool operator!=(T const* rhs) const noexcept { return ptr != rhs; }

private:
    T* ptr = nullptr;
};

template <class T>
struct unique_ptr<T[]>
{
    static_assert(detail::error<T>::value, "unique_ptr does not support arrays, use a vector instead");
};

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
} // namespace cc
