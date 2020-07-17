#pragma once

#include <clean-core/allocator.hh>
#include <clean-core/always_false.hh>
#include <clean-core/forward.hh>

namespace cc
{
// std::function replacement, not copyable, with allocator support
// slightly faster than unique_function, same codegen as STL (minus exceptions):
// https://godbolt.org/z/8fY3a5
// https://quick-bench.com/q/sCMOpZNIacJcwOPcCYt0_95Efoc
template<class Signature>
struct alloc_function;
template <class Result, class... Args>
struct alloc_function<Result(Args...)>
{
public:
    alloc_function() = default;
    alloc_function(decltype(nullptr)) {}

    template <class T>
    alloc_function(T&& callable, cc::allocator* alloc = cc::system_allocator)
    {
        static_assert(std::is_invocable_r_v<Result, T, Args...>, "argument to cc::function is not callable or has the wrong signature");
        _func = [](void* ctx, Args&&... args) -> Result { return (*static_cast<T*>(ctx))(cc::forward<Args>(args)...); };
        _deleter = [](void* ctx, cc::allocator* alloc) { alloc->delete_t<T>(static_cast<T*>(ctx)); };
        _alloc = alloc;
        _context = alloc->new_t<T>(cc::forward<T>(callable));
    }

    Result operator()(Args... args) const
    {
        CC_ASSERT(_context && "invoked a null cc::function");
        return (*_func)(_context, cc::forward<Args>(args)...);
    }

    bool is_valid() const { return _context != nullptr; }
    explicit operator bool() const { return _context != nullptr; }

    ~alloc_function() { _destroy(); }

    alloc_function(alloc_function&& rhs) noexcept : _func(rhs._func), _deleter(rhs._deleter), _context(rhs._context), _alloc(rhs._alloc)
    {
        rhs._context = nullptr;
    }

    alloc_function& operator=(alloc_function&& rhs) noexcept
    {
        if (this != &rhs)
        {
            _destroy();
            _func = rhs._func;
            _deleter = rhs._deleter;
            _alloc = rhs._alloc;
            _context = rhs._context;
            rhs._context = nullptr;
        }
        return *this;
    }

private:
    Result (*_func)(void*, Args&&...) = nullptr;
    void (*_deleter)(void*, cc::allocator*) = nullptr;
    cc::allocator* _alloc = nullptr;
    void* _context = nullptr;

    void _destroy()
    {
        if (_context != nullptr)
        {
            (*_deleter)(_context, _alloc);
        }
    }

    alloc_function(alloc_function const&) = delete;
    alloc_function& operator=(alloc_function const&) = delete;
};

template <class T>
struct alloc_function
{
    static_assert(always_false<T>, "cc::function expects a function signature type");
};
}
