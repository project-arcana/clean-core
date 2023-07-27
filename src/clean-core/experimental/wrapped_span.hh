#pragma once

#include <cstdint> // int64_t

#include <clean-core/span.hh>

namespace cc
{
/// a non-owning view of a contiguous array of Ts
/// works exactly like span, except for access via the subscript operator
/// Indices wrap around, there are no bounds checks
/// The last element can be accessed with my_span[-1], the second last element with my_span[-2] and so on,
/// while the first element will be returned when using my_span[my_span.size()]
template <class T>
struct wrapped_span : public span<T>
{
public:
    // ctors
    using span<T>::span;

    // container
public:
    constexpr T& operator[](int64_t i) const
    {
        i = i % int64_t(this->size());
        if (i < 0)
            i += int64_t(this->size());
        return this->data()[i];
    }
};

// deduction guide for containers
template <class Container, cc::enable_if<is_any_contiguous_range<Container>> = true>
wrapped_span(Container& c) -> wrapped_span<std::remove_reference_t<decltype(*c.data())>>;
template <class Container, cc::enable_if<is_any_contiguous_range<Container>> = true>
wrapped_span(Container&& c) -> wrapped_span<std::remove_reference_t<decltype(*c.data())>>;
} // namespace cc
