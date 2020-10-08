#pragma once

#include <clean-core/enable_if.hh>
#include <clean-core/fwd.hh>
#include <clean-core/hash_combine.hh>
#include <clean-core/typedefs.hh>

#include <cstring>
#include <type_traits>

namespace cc
{
// prototypical hash class
template <class T, class>
struct hash
{
    // Custom hashes should specify:
    // [[nodiscard]] constexpr hash_t operator()(T const& value) const noexcept { ... }

    template <class U = T, cc::enable_if<std::is_trivially_copyable_v<U> && std::has_unique_object_representations_v<U>> = true>
    [[nodiscard]] hash_t operator()(T const& value) const noexcept
    {
        if constexpr (sizeof(T) <= sizeof(hash_t))
        {
            hash_t h = 0;
            std::memcpy(&h, &value, sizeof(value));
            return h;
        }
        else
        {
            // full words
            auto constexpr wcnt = sizeof(value) / sizeof(hash_t);
            auto constexpr rest_size = sizeof(value) - wcnt * sizeof(hash_t);
            static_assert(0 <= rest_size && rest_size < sizeof(hash_t));

            auto h_ptr = reinterpret_cast<hash_t const*>(&value);

            // hash full words
            hash_t r = 0;
            for (size_t i = 0; i < wcnt; ++i)
                r = cc::hash_combine(r, h_ptr[i]);

            // hash rest
            if constexpr (rest_size > 0)
            {
                hash_t h = 0;
                std::memcpy(&h, h_ptr + wcnt, rest_size);
                r = cc::hash_combine(r, h);
            }

            return r;
        }
    }
};

// transparent hash class
template <>
struct hash<void>
{
    template <class T>
    [[nodiscard]] constexpr hash_t operator()(T const& value) const noexcept
    {
        return hash<T>{}(value);
    }
};

namespace detail
{
template <class T, class Hasher, class = void>
struct can_hash_t : std::false_type
{
};
template <class T, class Hasher>
struct can_hash_t<T, Hasher, std::void_t<decltype(std::declval<Hasher>()(std::declval<T>()))>> : std::true_type
{
};
}

/// true if T can be hashed by Hasher
template <class T, class Hasher = hash<T>>
static constexpr bool can_hash = detail::can_hash_t<T, Hasher>::value;


// ============== make_hash ==============

template <class Hasher = hash<void>, class... Args>
constexpr hash_t make_hash(Args const&... values) noexcept
{
    static_assert((can_hash<Args, Hasher> && ...), "argument not hashable");
    return cc::hash_combine(Hasher{}(values)...);
}


// ============== default specializations ==============

template <>
struct hash<float>
{
    [[nodiscard]] hash_t operator()(float value) const noexcept
    {
        // make sure +- 0 is the same
        if (value == 0)
            return 0;

        // make sure NaN is consistently hashed
        if (value != value)
            return 0xFFFF'FFFF'FFFF'1234uLL;

        // otherwise take the bits
        hash_t h = 0;
        std::memcpy(&h, &value, sizeof(value));
        return h;
    }
};

template <>
struct hash<double>
{
    [[nodiscard]] hash_t operator()(double value) const noexcept
    {
        // make sure +- 0 is the same
        if (value == 0)
            return 0;

        // make sure NaN is consistently hashed
        if (value != value)
            return 0xFFFF'FFFF'FFFF'5678uLL;

        // otherwise take the bits
        hash_t h = 0;
        std::memcpy(&h, &value, sizeof(value));
        return h;
    }
};
}
