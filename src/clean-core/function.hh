#pragma once

#include <clean-core/allocator.hh>
#include <clean-core/forward.hh>

namespace cc
{
// std::function replacement, not copyable

template <typename T>
class function
{
    static_assert(sizeof(T*) == 0, "cc::function expects a function signature type");
};

template <class Result, class... Args>
class function<Result(Args...)>
{
public:
    function() = default;
    function(decltype(nullptr)) {}

    /// constructs a function from a callable
    template <class T>
    function(T&& callable, cc::allocator* alloc = cc::system_allocator)
    {
        static_assert(std::is_invocable_r_v<Result, T, Args...>, "argument to cc::function is not callable or has the wrong signature");
        _func = [](void* ctx, Args&&... args) -> Result { return (*static_cast<T*>(ctx))(cc::forward<Args>(args)...); };
        _deleter = [](void* ctx, cc::allocator* alloc) { alloc->delete_t<T>(static_cast<T*>(ctx)); };
        _alloc = alloc;
        _context = alloc->new_t<T>(cc::forward<T>(callable));
    }

    /// invokes the contained callable
    Result operator()(Args... args) const
    {
        CC_ASSERT(is_valid() && "invoked a null cc::function");
        return (*_func)(_context, cc::forward<Args>(args)...);
    }

    bool is_valid() const { return _context != nullptr; }

    ~function() { _destroy(); }

    function(function&& rhs) noexcept : _func(rhs._func), _deleter(rhs._deleter), _context(rhs._context), _alloc(rhs._alloc)
    {
        rhs._context = nullptr;
    }

    function& operator=(function&& rhs) noexcept
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

    function(function const&) = delete;
    function& operator=(function const&) = delete;
};
}
