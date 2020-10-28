#pragma once

#include <clean-core/assert.hh>
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
public:
    constexpr Result operator()(Args... args) const
    {
        CC_ASSERT(_fun != nullptr && "cannot call null function");
        return _fun(_data, cc::forward<Args>(args)...);
    }

    template <class F, enable_if<std::is_invocable_r_v<Result, F, Args...> && !std::is_same_v<std::decay_t<F>, function_ref>> = true>
    constexpr function_ref(F&& f)
    {
        if constexpr (std::is_function_v<std::remove_pointer_t<std::remove_reference_t<F>>>) // function pointer
        {
            // comes first since in some architectures function pointers are void* convertible as well
            _data.fun = f;
            CC_CONTRACT(_data.fun != nullptr && "null function pointers not allowed in cc::function_ref");
            _fun = [](storage const& s, Args... args) -> Result { return s.fun(cc::forward<Args>(args)...); };
        }
        else if constexpr (std::is_assignable_v<void*&, decltype(&f)>) // ptr / ref to callable
        {
            _data.obj = &f;
            _fun = [](storage const& s, Args... args) -> Result {
                return (*static_cast<std::remove_reference_t<F>*>(s.obj))(cc::forward<Args>(args)...);
            };
        }
        else if constexpr (std::is_assignable_v<void const*&, decltype(&f)>) // ptr / ref to const callable
        {
            _data.obj_const = &f;
            _fun = [](storage const& s, Args... args) -> Result {
                return (*static_cast<std::remove_reference_t<F>*>(s.obj))(cc::forward<Args>(args)...);
            };
        }
        else
        {
            static_assert(cc::always_false<F>, "argument cannot be converted to cc::function_ref");
        }
    }

    /// NOTE: this function_ref is invalid and must not be called
    ///       it may only be assigned to
    constexpr function_ref() = default;

    constexpr function_ref(function_ref const&) = default;
    constexpr function_ref& operator=(function_ref const&) = default;

private:
    union storage {
        void* obj;                         // pointer to callable
        void const* obj_const;             // pointer to const callable
        function_ptr<Result(Args...)> fun; // original function pointer

        storage() { obj = nullptr; }
    };

    storage _data;                                                // storage for a pointer to the callable
    function_ptr<Result(storage const&, Args...)> _fun = nullptr; // the function used to invoke the callable
};

template <class Result, class... Args>
function_ref(Result (*)(Args...))->function_ref<Result(Args...)>;
}
