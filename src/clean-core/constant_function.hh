#pragma once

namespace cc
{
/// a function object that always returns a given constant
template <auto C>
struct constant_function
{
    template <class... Args>
    constexpr auto operator()(Args&&...) const noexcept
    {
        return C;
    }
};
}
