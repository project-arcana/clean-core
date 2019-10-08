#pragma once

#include <clean-core/typedefs.hh>

namespace cc
{
// constants
enum
{
    dynamic_size = size_t(-1)
};

// utility
template <class T, class = void> // SFINAE-friendly
struct hash;

template <class T = void, class = void> // SFINAE-friendly
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

template <class T, size_t N = dynamic_size>
struct array;

// smart pointer
template <class T>
struct unique_ptr;
}
