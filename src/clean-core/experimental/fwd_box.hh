#pragma once

#include <clean-core/allocate.hh>
#include <clean-core/enable_if.hh>
#include <clean-core/forward.hh>
#include <clean-core/function_ptr.hh>
#include <clean-core/move.hh>

namespace cc
{
/// a non-polymorphic move-only forward-declaration friendly value type (allocated on the heap)
/// (basically a non-nullable unique_ptr with support for incomplete types)
template <class T>
struct fwd_box
{
    fwd_box(fwd_box&& b) noexcept
    {
        _data = b._data;
        _deleter = b._deleter;
        b._data = nullptr;
        b._deleter = nullptr;
    }
    fwd_box& operator=(fwd_box&& b) noexcept
    {
        if (_deleter)
            _deleter(_data);
        _data = b._data;
        _deleter = b._deleter;
        b._data = nullptr;
        b._deleter = nullptr;

        return *this;
    }

    fwd_box(fwd_box const&) = delete;
    fwd_box& operator=(fwd_box const&) = delete;

    ~fwd_box()
    {
        if (_deleter)
            _deleter(_data);
    }

    template <class U, class... Args>
    friend fwd_box<U> make_fwd_box(Args&&... args);

    [[nodiscard]] T* get() { return _data; }
    [[nodiscard]] T const* get() const { return _data; }

    T* operator->()
    {
        CC_ASSERT_NOT_NULL(_data); // moved-from state?
        return _data;
    }
    T const* operator->() const
    {
        CC_ASSERT_NOT_NULL(_data); // moved-from state?
        return _data;
    }
    T& operator*()
    {
        CC_ASSERT_NOT_NULL(_data); // moved-from state?
        return *_data;
    }
    T const& operator*() const
    {
        CC_ASSERT_NOT_NULL(_data); // moved-from state?
        return *_data;
    }

    operator T const&() const { return *_data; }
    operator T&() { return *_data; }

private:
    explicit fwd_box(T* data) : _data(data)
    {
        _deleter = [](T* data) { cc::free(data); };
    }

    T* _data;
    function_ptr<void(T*)> _deleter;
};

template <class T, class... Args>
fwd_box<T> make_fwd_box(Args&&... args)
{
    return fwd_box<T>(cc::alloc<T>(cc::forward<Args>(args)...));
}

}
