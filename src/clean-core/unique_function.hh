#pragma once

#include <clean-core/allocator.hh>
#include <clean-core/always_false.hh>
#include <clean-core/forward.hh>
#include <clean-core/function_ptr.hh>

namespace cc
{
/**
 * std::function replacement, not copyable, with allocator support
 *
 * NOTE: empty unique_functions can be created but not called!
 *
 * same codegen at execute as STL (minus exceptions):
 * https://godbolt.org/z/8fY3a5
 * https://quick-bench.com/q/sCMOpZNIacJcwOPcCYt0_95Efoc
 *
 * TODO: sbo_ and capped_ versions
 * TODO: deduction guides
 * TODO: member functions
 */
template <class Signature>
struct unique_function;
template <class Result, class... Args>
struct unique_function<Result(Args...)>
{
public:
    unique_function() = default;
    unique_function(decltype(nullptr)) {}

    template <class T>
    unique_function(T&& callable, cc::allocator* alloc = cc::system_allocator)
    {
        using CallableT = std::decay_t<T>;
        static_assert(std::is_invocable_r_v<Result, CallableT, Args...>, "argument to cc::unique_function is not callable or has the wrong "
                                                                         "signature");

        _func = [](void* ctx, Args&&... args) -> Result { return (*static_cast<CallableT*>(ctx))(cc::forward<Args>(args)...); };
        _deleter = [](void* ctx, cc::allocator* alloc) { alloc->delete_t<CallableT>(static_cast<CallableT*>(ctx)); };
        _alloc = alloc;
        _context = alloc->new_t<CallableT>(cc::forward<CallableT>(callable));
    }

    Result operator()(Args... args) const
    {
        CC_ASSERT(_context && "invoked a null cc::unique_function");
        return (*_func)(_context, cc::forward<Args>(args)...);
    }

    bool is_valid() const { return _context != nullptr; }
    explicit operator bool() const { return _context != nullptr; }

    ~unique_function() { _destroy(); }

    unique_function(unique_function&& rhs) noexcept : _func(rhs._func), _deleter(rhs._deleter), _context(rhs._context), _alloc(rhs._alloc)
    {
        rhs._context = nullptr;
    }

    unique_function& operator=(unique_function&& rhs) noexcept
    {
        _destroy();
        _func = rhs._func;
        _deleter = rhs._deleter;
        _alloc = rhs._alloc;
        _context = rhs._context;
        rhs._context = nullptr;
        return *this;
    }

private:
    cc::function_ptr<Result(void*, Args&&...)> _func = nullptr;
    cc::function_ptr<void(void*, cc::allocator*)> _deleter = nullptr;
    cc::allocator* _alloc = nullptr;
    void* _context = nullptr;

    void _destroy()
    {
        if (_context != nullptr)
        {
            (*_deleter)(_context, _alloc);
        }
    }

    unique_function(unique_function const&) = delete;
    unique_function& operator=(unique_function const&) = delete;
};

template <class T>
struct unique_function
{
    static_assert(always_false<T>, "cc::unique_function expects a function signature type");
};
}
