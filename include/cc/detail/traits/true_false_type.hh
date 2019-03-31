#pragma once

namespace cc
{
struct true_type
{
    static inline constexpr bool value = true;
};
struct false_type
{
    static inline constexpr bool value = false;
};
} // namespace cc
