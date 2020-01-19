#pragma once

#include <clean-core/typedefs.hh>

namespace cc
{
// constants
enum : size_t
{
    dynamic_size = size_t(-1)
};

// utility
template <class T, class = bool> // SFINAE-friendly for cc::enable_if
struct hash;

template <class T = void, class = bool> // SFINAE-friendly for cc::enable_if
struct less;

struct nullopt_t;
template <class T>
struct optional;

template <class A, class B>
struct pair;

// containers and ranges
template <class T>
struct span;

template <class T>
struct vector;
template <class T, size_t N>
struct capped_vector;

template <class T, size_t N = dynamic_size>
struct array;
template <class T>
struct fwd_array;
template <class T, size_t N>
struct capped_array;

// strings
struct string_view;
template <size_t sbo_capacity>
struct sbo_string;
using string = sbo_string<15>;

// functional
template <class Signature>
struct unique_function;

// smart pointer
template <class T>
struct unique_ptr;
template <class T>
struct poly_unique_ptr;
}
