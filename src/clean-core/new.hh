#pragma once

#include <cstddef>

namespace cc
{
namespace detail
{
struct placement_new_tag
{
};
}
[[maybe_unused]] static constexpr cc::detail::placement_new_tag placement_new;
}

/// Usage:
/// T* ptr = new(cc::placement_new, memory) T();
inline void* operator new(size_t, cc::detail::placement_new_tag, void* buffer) { return buffer; }
inline void* operator new[](size_t, cc::detail::placement_new_tag, void* buffer) { return buffer; }
inline void operator delete(void*, cc::detail::placement_new_tag, void*) {}
