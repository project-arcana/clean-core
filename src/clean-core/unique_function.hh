#pragma once

#include <clean-core/always_false.hh>
#include <clean-core/assert.hh>
#include <clean-core/forward.hh>
#include <clean-core/fwd.hh>
#include <clean-core/move.hh>

#include <type_traits>

namespace cc
{
/**
 * A move-only type-erased owning function wrapper
 *
 * NOTE: empty unique_functions can be created but not called!
 *
 * TODO: sbo_ and capped_ versions
 * TODO: deduction guides
 * TODO: member functions
 */
template <class Result, class... Args>
struct unique_function<Result(Args...)>
{
private:
    struct holder_base
    {
        virtual Result operator()(Args... args) const = 0;
        virtual ~holder_base() = default;
    };
    template <class F>
    struct holder : holder_base
    {
        F _fun;
        holder(F&& f) : _fun(cc::move(f)) {}
        Result operator()(Args... args) const override { return _fun(cc::forward<Args>(args)...); }
    };

    // operations
public:
    Result operator()(Args... args) const
    {
        CC_CONTRACT(_fun);
        return (*_fun)(cc::forward<Args>(args)...);
    }

    explicit operator bool() const { return _fun != nullptr; }

    template <class F, class = std::enable_if_t<!std::is_same_v<std::decay_t<F>, unique_function>>>
    unique_function(F&& f)
    {
        static_assert(std::is_invocable_r_v<Result, F, Args...>);
        _fun = new holder<F>(cc::move(f));
    }

    // move-only type
public:
    unique_function() = default;
    unique_function(unique_function&& rhs) noexcept
    {
        _fun = rhs._fun;
        rhs._fun = nullptr;
    }
    unique_function& operator=(unique_function&& rhs) noexcept
    {
        delete _fun;
        _fun = rhs._fun;
        rhs._fun = nullptr;
        return *this;
    }
    unique_function(unique_function const&) = delete;
    unique_function& operator=(unique_function const&) = delete;
    ~unique_function() { delete _fun; }

    // members
private:
    holder_base* _fun = nullptr;
};

template <class T>
struct unique_function
{
    static_assert(always_false<T>, "template arg is not a function signature");
};
}
