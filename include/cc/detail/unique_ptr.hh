#pragma once

#include <cc/assert>
#include <cc/detail/error.hh>
#include <cc/fwd/hash.hh>
#include <cc/fwd/unique_ptr.hh>
#include <cc/swap>
#include <cc/typedefs>

namespace cc
{
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
        delete ptr;
        ptr = p;
    }

    T* release() noexcept
    {
        auto p = ptr;
        ptr = nullptr;
        return p;
    }

    T* get() const noexcept { return ptr; }
    T* operator->() const noexcept { return ptr; }
    T& operator*() const noexcept
    {
        CC_ASSERT_NOT_NULL(ptr);
        return *ptr;
    }

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
    hash_t operator()(unique_ptr<T> const& v) const noexcept { return hash_t(v.get()); }
};
} // namespace cc
