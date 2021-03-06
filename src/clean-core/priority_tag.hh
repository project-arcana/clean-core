#pragma once

#include <cstddef>

namespace cc
{
/**
 * Priorities for overloads
 */
template <size_t P>
struct priority_tag : priority_tag<P - 1>
{
};
template <>
struct priority_tag<0>
{
};
}
