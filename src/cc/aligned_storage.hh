#pragma once

#include <cc/typedefs.hh>

namespace cc
{
template <uint64 Length, uint64 Align = (Length > sizeof(void*) ? sizeof(void*) : Length)>
struct aligned_storage
{
    alignas(Align) byte data[Length];
};
}
