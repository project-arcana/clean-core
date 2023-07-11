#pragma once

#include <cstdint>
#include <type_traits>

#include <clean-core/allocate.hh>
#include <clean-core/assert.hh>
#include <clean-core/forward.hh>
#include <clean-core/fwd.hh>
#include <clean-core/macros.hh>

namespace cc
{
template <class T, class... Args>
shared_ptr<T> make_shared(Args&&... args);

namespace detail
{
template <class T>
struct control_t
{
    // must stay first arg, so that _control == &_control->value
    T value;

    // int -> make overflow UB
    int refcount;

    template <class... Args>
    control_t(Args&&... args) : value(cc::forward<Args>(args)...), refcount(1)
    {
    }
};
} // namespace detail

/// a high-performance, basic ref-counted pointer
/// in particular:
///  - no thread-safety (no atomic access required)
///  - no weak_ptr (simplifies control block)
///  - only make_shared supported (less code, less indirection)
///  - no custom allocation / deleter (allows cc::alloc / cc::free)
///  - no shared_from_this (consequence of no-weak-ptr)
///  - no polymorphic casts (consequence of alloc/free)
///
/// enables limited forms of refcount merging and in general produces nice-ish code:
///   https://godbolt.org/z/TMafWdM49 (full refcount++ merging, register refcount--)
template <class T>
struct shared_ptr
{
    static_assert(!std::is_reference_v<T>, "cannot created a shared_ptr of a reference");

    using control = detail::control_t<std::remove_const_t<T>>;

    bool is_valid() const { return _control != nullptr; }
    explicit operator bool() const { return is_valid(); }

    T& operator*() const
    {
        CC_ASSERT(is_valid());
        return _control->value;
    }

    T* operator->() const
    {
        CC_ASSERT(is_valid());
        return &_control->value;
    }

    T* get() const
    {
        return reinterpret_cast<T*>(_control); // value always first
    }

    bool is_unique() const { return refcount() == 1; }

    uint32_t refcount() const { return _control ? _control->refcount : 0; }

    void reset()
    {
        if (_control)
        {
            dec_refcount();
            _control = nullptr;
        }
    }

    shared_ptr() = default;
    shared_ptr(shared_ptr&& rhs) noexcept
    {
        _control = rhs._control;
        rhs._control = nullptr;
    }
    shared_ptr(shared_ptr const& rhs)
    {
        _control = rhs._control;

        if (_control)
            _control->refcount++;
    }
    shared_ptr& operator=(shared_ptr&& rhs) noexcept
    {
        if (_control)
            dec_refcount(); // might set rhs._control to 0

        _control = rhs._control;
        rhs._control = nullptr;

        return *this;
    }
    shared_ptr& operator=(shared_ptr const& rhs)
    {
        if (this == &rhs)
            return *this;

        if (_control)
            dec_refcount();

        _control = rhs._control;

        if (_control)
            _control->refcount++;

        return *this;
    }
    ~shared_ptr()
    {
        if (_control)
            dec_refcount();
    }

    // nullptr is same as default constructed shared_ptr
    shared_ptr(std::nullptr_t) {}
    shared_ptr& operator=(std::nullptr_t)
    {
        if (_control)
            dec_refcount();
        _control = nullptr;
        return *this;
    }

    // can convert to const
    operator shared_ptr<T const>() const
    {
        shared_ptr<T const> r;
        r._control = _control;
        if (_control)
            _control->refcount++;
        return r;
    }

    bool operator==(shared_ptr const& rhs) const { return _control == rhs._control; }
    bool operator!=(shared_ptr const& rhs) const { return _control != rhs._control; }
    bool operator<(shared_ptr const& rhs) const { return _control < rhs._control; }
    bool operator<=(shared_ptr const& rhs) const { return _control <= rhs._control; }
    bool operator>(shared_ptr const& rhs) const { return _control > rhs._control; }
    bool operator>=(shared_ptr const& rhs) const { return _control >= rhs._control; }

private:
    void dec_refcount()
    {
        if CC_CONDITION_UNLIKELY (--_control->refcount == 0)
        {
            cc::free(_control);
            _control = nullptr;
        }
    }

    control* _control = nullptr;

    friend shared_ptr<std::remove_const_t<T>>;

    template <class U, class... Args>
    friend shared_ptr<U> make_shared(Args&&... args);
};

template <class T, class... Args>
shared_ptr<T> make_shared(Args&&... args)
{
    shared_ptr<T> s;
    s._control = cc::alloc<typename shared_ptr<T>::control>(cc::forward<Args>(args)...);
    return s;
}

// hash
template <class T>
struct hash<shared_ptr<T>>
{
    [[nodiscard]] constexpr uint64_t operator()(shared_ptr<T> const& a) const noexcept { return hash<T const*>{}(a.get()); }
};
} // namespace cc
