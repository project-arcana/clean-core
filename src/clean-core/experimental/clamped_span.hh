#pragma once

#include <cstdint> // int64_t

#include <clean-core/span.hh>

namespace cc
{
/// a non-owning view of a contiguous array of Ts
/// works exaclty like span, except there are no bounds checks and indices are instead clamped to the valid range
/// For example, my_clamped_span[-1] == my_clamped_span[0] and my_clamped_span[my_clamped_span.size()] == my_clamped_span[my_clamped_span.size() -1]
template <class T>
struct clamped_span : public span<T>
{
public:
    // ctors
    using span<T>::span;

    // container
public:
    constexpr T& operator[](int64_t i) const { return this->data()[clamp(i, int64_t(0), int64_t(this->size() - 1))]; }
};

// deduction guide for containers
template <class Container, cc::enable_if<is_any_contiguous_range<Container>> = true>
clamped_span(Container& c) -> clamped_span<std::remove_reference_t<decltype(*c.data())>>;
template <class Container, cc::enable_if<is_any_contiguous_range<Container>> = true>
clamped_span(Container&& c) -> clamped_span<std::remove_reference_t<decltype(*c.data())>>;
} // namespace cc
