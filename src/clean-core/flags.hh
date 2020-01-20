#pragma once

#include <type_traits>

#include <clean-core/assert.hh>
#include <clean-core/bits.hh>
#include <clean-core/detail/compact_size_t.hh>
#include <clean-core/explicit_bool.hh>
#include <clean-core/fwd.hh>
#include <clean-core/typedefs.hh>

namespace cc
{
#define CC_FLAGS_ENUM(_enum_t_)                                                                   \
    constexpr ::cc::flags<_enum_t_> operator|(_enum_t_ a, _enum_t_ b) noexcept { return {a, b}; } \
    CC_FORCE_SEMICOLON

#define CC_FLAGS_ENUM_SIZED(_enum_t_, _size_)                                                             \
    constexpr ::cc::flags<_enum_t_, _size_> operator|(_enum_t_ a, _enum_t_ b) noexcept { return {a, b}; } \
    CC_FORCE_SEMICOLON

namespace detail
{
template <class EnumT, class ValueT>
struct flags_iterator;
struct flags_iterator_sentinel
{
};
}

template <class EnumT, size_t Bits = 8 * sizeof(EnumT)>
struct flags
{
    using storage_t = detail::compact_size_t_by_bits<Bits>;

    static_assert(std::is_enum_v<EnumT>, "this class is designed for enum types");

    // helper
private:
    static constexpr storage_t _stored_value_of(EnumT e) { return storage_t(1 << storage_t(e)); }
    static constexpr storage_t _stored_value_of(flags f) { return f._value; }

    // properties
public:
    constexpr storage_t value() const { return _value; }

    [[nodiscard]] constexpr bool has(EnumT e) const { return bool(operator&(e)); }
    [[nodiscard]] constexpr bool has_any() const { return _value != 0; }
    [[nodiscard]] constexpr bool has_all_of(flags f) const { return (_value & f._value) == f._value; }
    [[nodiscard]] constexpr bool has_any_of(flags f) const { return (_value & f._value) != 0; }

    bool is_single() const { return cc::popcount(_value) == 1; }
    EnumT single() const
    {
        CC_CONTRACT(is_single());
        return EnumT(cc::count_trailing_zeros(_value));
    }

    constexpr detail::flags_iterator<EnumT, storage_t> begin() const { return _value; }
    constexpr detail::flags_iterator_sentinel end() const { return {}; }

    // ctors and factories
public:
    constexpr flags() = default;
    template <class... Args>
    constexpr flags(EnumT f, Args... other_flags)
    {
        _value = (_stored_value_of(f) | ... | _stored_value_of(other_flags));
    }

    [[nodiscard]] static constexpr flags from_value(storage_t value)
    {
        flags f;
        f._value = value;
        return f;
    }

    // operators
public:
    constexpr explicit_bool operator&(EnumT f) const
    {
        return (_value & _stored_value_of(f)) != 0; //
    }
    constexpr flags operator&(flags f) const
    {
        return from_value(_value & f._value); //
    }
    constexpr flags& operator&=(EnumT f)
    {
        _value &= _stored_value_of(f);
        return *this;
    }
    constexpr flags& operator&=(flags f)
    {
        _value &= f._value;
        return *this;
    }

    constexpr flags operator|(EnumT f) const
    {
        return from_value(_value | _stored_value_of(f)); //
    }
    constexpr flags operator|(flags f) const
    {
        return from_value(_value | f._value); //
    }
    constexpr flags& operator|=(EnumT f)
    {
        _value |= _stored_value_of(f);
        return *this;
    }
    constexpr flags& operator|=(flags f)
    {
        _value |= f._value;
        return *this;
    }

    constexpr bool operator<(flags f) const { return _value < f._value; }
    constexpr bool operator<=(flags f) const { return _value <= f._value; }
    constexpr bool operator>(flags f) const { return _value > f._value; }
    constexpr bool operator>=(flags f) const { return _value >= f._value; }
    constexpr bool operator==(flags f) const { return _value == f._value; }
    constexpr bool operator!=(flags f) const { return _value != f._value; }
    constexpr bool operator==(EnumT f) const { return _value == _stored_value_of(f); }
    constexpr bool operator!=(EnumT f) const { return _value != _stored_value_of(f); }

    constexpr explicit operator bool() const { return _value != 0; }

    // storage member
private:
    storage_t _value = 0;
};

template <class EnumT, size_t Bits>
auto to_string(flags<EnumT, Bits> f) -> decltype(to_string(std::declval<EnumT>()))
{
    using string_t = decltype(to_string(std::declval<EnumT>()));

    string_t s = "{";
    auto first = true;
    for (auto i = 0u; i < Bits; ++i)
        if (f & EnumT(i))
        {
            if (first)
                first = false;
            else
                s += ", ";
            s += to_string(EnumT(i));
        }
    s += '}';
    return s;
}

template <class EnumT, size_t Bits>
constexpr flags<EnumT, Bits> operator|(EnumT a, flags<EnumT, Bits> b)
{
    return {a, b};
}

template <size_t Bits = 0, class EnumT, class... Args>
constexpr auto make_flags(EnumT e, Args... args)
{
    if constexpr (Bits == 0)
        return flags<EnumT>(e, args...);
    else
        return flags<EnumT, Bits>(e, args...);
}

template <class EnumT, size_t Bits>
struct hash<flags<EnumT, Bits>>
{
    constexpr hash_t operator()(flags<EnumT, Bits> const& f) const noexcept { return hash_t(f.value()); }
};

template <class EnumT, class ValueT>
struct detail::flags_iterator
{
    size_t curr_idx;
    size_t last_idx;
    ValueT value;

    constexpr flags_iterator(ValueT v)
    {
        value = v;
        if (value == 0)
        {
            curr_idx = 0;
            last_idx = 0;
        }
        else
        {
            curr_idx = cc::count_trailing_zeros(value);
            last_idx = 8 * sizeof(value) - cc::count_leading_zeros(value);
        }
    }

    constexpr bool operator!=(flags_iterator_sentinel) const { return curr_idx != last_idx; }

    constexpr EnumT operator*() const { return EnumT(curr_idx); }

    constexpr void operator++()
    {
        ++curr_idx;
        while (curr_idx < last_idx && (value & (1 << curr_idx)) == 0)
            ++curr_idx;
    }
};
}
