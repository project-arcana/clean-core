#pragma once

#include <clean-core/enable_if.hh>
#include <clean-core/forward.hh>
#include <clean-core/function_ptr.hh>
#include <clean-core/fwd.hh>

#include <type_traits>

namespace cc
{
/**
 * A non-owning view on a callable object
 *
 * NOTE: this "captures by reference" in the sense that it will not extend lifetimes (similar to span)
 *
 * TODO: pointer to member and member functions
 */
template <class Result, class... Args>
struct function_ref<Result(Args...)>
{
private:
    union storage {
        void* obj;
        function_ptr<Result(Args...)> fun;

        storage() {}
    };

public:
    constexpr Result operator()(Args... args) const { return _fun(_data, cc::forward<Args>(args)...); }

    template <class F, enable_if<std::is_invocable_r_v<Result, F, Args...> && !std::is_same_v<std::decay_t<F>, function_ref>> = true>
    constexpr function_ref(F&& f)
    {
        if constexpr (std::is_assignable_v<void*&, decltype(&f)>)
        {
            _data.obj = &f;
            _fun = [](storage const& s, Args... args) -> Result {
                return (*static_cast<std::remove_reference_t<F>*>(s.obj))(cc::forward<Args>(args)...);
            };
        }
        else // function pointers
        {
            _data.fun = &f;
            _fun = [](storage const& s, Args... args) -> Result { return s.fun(cc::forward<Args>(args)...); };
        }
    }

    constexpr function_ref(function_ref const&) = default;
    constexpr function_ref& operator=(function_ref const&) = default;

private:
    storage _data;
    function_ptr<Result(storage const&, Args...)> _fun = nullptr;
};

template <class Result, class... Args>
function_ref(Result (*)(Args...))->function_ref<Result(Args...)>;
}
