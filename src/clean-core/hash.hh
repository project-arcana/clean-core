#pragma once

#include <clean-core/always_false.hh>
#include <clean-core/fwd.hh>
#include <clean-core/typedefs.hh>

#include <cstring>
#include <type_traits>

namespace cc
{
template <class T, class>
struct hash
{
    static_assert(always_false<T>, "no hash<> specialization found");

    // [[nodiscard]] hash_t operator()(T const& value) const noexcept { ... }
};


// ============== hash_combine ==============

constexpr inline hash_t hash_combine() { return 0x2a5114b5c6133408uLL; }
constexpr inline hash_t hash_combine(hash_t a) { return a; }
constexpr inline hash_t hash_combine(hash_t a, hash_t b) { return a * 6364136223846793005ULL + b + 0xda3e39cb94b95bdbULL; }

template <class... Args>
constexpr hash_t hash_combine(hash_t a, hash_t b, hash_t c, Args... rest)
{
    static_assert((std::is_same_v<Args, hash_t> && ...), "extra arguments need to be hash_t as well");
    auto h = hash_combine(a, b);
    h = hash_combine(h, c);
    ((h = hash_combine(h, rest)), ...);
    return h;
}


// ============== make_hash ==============

template <class... Args>
constexpr hash_t make_hash(Args const&... values)
{
    return cc::hash_combine(hash<Args>{}(values)...);
}


// ============== default specializations ==============

template <class T>
struct hash<T, std::enable_if_t<std::is_trivially_copyable_v<T> && std::has_unique_object_representations_v<T>>>
{
    [[nodiscard]] hash_t operator()(T const& value) const noexcept
    {
        enum
        {
            wcnt = (sizeof(value) + sizeof(hash_t) - 1) / sizeof(hash_t)
        };

        hash_t words[wcnt] = {}; // zero-init
        std::memcpy(words, &value, sizeof(value));
        auto h = words[0];
        for (auto i = 1; i < wcnt; ++i)
            h = cc::hash_combine(h, words[i]);
        return h;
    }
};
}
