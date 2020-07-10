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
template <class T = void, class = bool> // SFINAE-friendly for cc::enable_if
struct equal_to;

struct nullopt_t;
template <class T>
struct optional;

template <class A, class B>
struct pair;
template <class... Types>
struct tuple;

// containers and ranges
template <class T>
struct span;
template <class T>
struct strided_span;

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

template <class KeyT, class ValueT, class HashT = cc::hash<KeyT>, class EqualT = cc::equal_to<void>>
struct map;
template <class T, class HashT = cc::hash<T>, class EqualT = cc::equal_to<void>>
struct set;

// values
template <class T>
struct box;
template <class T>
struct fwd_box;
template <class T>
struct poly_box;
template <class T, size_t MaxSize = sizeof(T)>
struct capped_box;
template <class T>
using pimpl = fwd_box<T>;

// strings
struct string_view;
template <size_t sbo_capacity>
struct sbo_string;
using string = sbo_string<15>;
struct string_stream;

// streams
template <class T>
struct stream_ref;

// functional
template <class Signature>
struct unique_function;
template <class Signature>
struct function_ref;

// smart pointer
template <class T>
struct unique_ptr;
template <class T>
struct poly_unique_ptr;
}
