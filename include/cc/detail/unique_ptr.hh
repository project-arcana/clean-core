#pragma once

#include <cc/assert>
#include <cc/detail/error.hh>
#include <cc/fwd/hash.hh>
#include <cc/fwd/less.hh>
#include <cc/fwd/unique_ptr.hh>
#include <cc/move>
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
        if (this != &rhs)
        {
            delete ptr;
            ptr = rhs.ptr;
            rhs.ptr = nullptr;
        }
        return *this;
    }

    ~unique_ptr()
    {
        static_assert(sizeof(T) > 0, "cannot delete incomplete class");
        delete ptr;
    }

    void reset(T* p = nullptr)
    {
        CC_ASSERT_CONTRACT(p == nullptr || p != ptr); // no self-reset
        delete ptr;
        ptr = p;
    }

    [[nodiscard]] T* get() const { return ptr; }

    [[nodiscard]] T* release()
    {
        auto p = ptr;
        ptr = nullptr;
        return p;
    }

    [[nodiscard]] T* operator->() const
    {
        CC_ASSERT_NOT_NULL(ptr);
        return ptr;
    }
    [[nodiscard]] T& operator*() const
    {
        CC_ASSERT_NOT_NULL(ptr);
        return *ptr;
    }

    [[nodiscard]] bool operator==(unique_ptr const& rhs) const { return ptr == rhs.ptr; }
    [[nodiscard]] bool operator!=(unique_ptr const& rhs) const { return ptr != rhs.ptr; }
    [[nodiscard]] bool operator==(T const* rhs) const { return ptr == rhs; }
    [[nodiscard]] bool operator!=(T const* rhs) const { return ptr != rhs; }

private:
    T* ptr = nullptr;
};

template <class T>
struct unique_ptr<T[]>
{
    static_assert(detail::error<T>::value, "unique_ptr does not support arrays, use a vector instead");
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
    p.reset(new T(cc::forward<Args>(args)...));
    return p;
}

template <class T>
struct hash<unique_ptr<T>>
{
    [[nodiscard]] hash_t operator()(unique_ptr<T> const& v) const { return hash_t(v.get()); }
};

template <class T>
struct less<unique_ptr<T>>
{
    [[nodiscard]] bool operator()(unique_ptr<T> const& a, unique_ptr<T> const& b) const { return a.get() < b.get(); }
};
} // namespace cc
