#pragma once

#include <clean-core/typedefs.hh>

namespace cc
{
template <class EnumT, size_t Bits = 8 * sizeof(EnumT)>
struct flags
{
};
}
