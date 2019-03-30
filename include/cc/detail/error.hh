#pragma once

namespace cc::detail
{
template <class... E>
struct error
{
    static constexpr bool value = false;
};
} // namespace cc::detail
