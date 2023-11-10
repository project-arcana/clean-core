#pragma once

#include <type_traits>

#include <clean-core/assert.hh>
#include <clean-core/move.hh>
#include <clean-core/new.hh>

namespace cc
{
/// a result type that is conceptually either a value or an error
///
/// TODO:
/// - comparison operations
template <class Value, class Error>
struct result
{
    static_assert(std::is_default_constructible_v<Error>, "the error type must be default constructible");

public:
    [[nodiscard]] constexpr bool is_value() const { return _is_value; }
    [[nodiscard]] constexpr bool is_error() const { return !_is_value; }
    [[nodiscard]] constexpr bool is_value(Value const& v) const { return _is_value && _storage._value == v; }
    [[nodiscard]] constexpr bool is_error(Error const& e) const { return !_is_value && _storage._error == e; }

    [[nodiscard]] constexpr Value& value()
    {
        CC_ASSERT(_is_value);
        return _storage._value;
    }
    [[nodiscard]] constexpr Value const& value() const
    {
        CC_ASSERT(_is_value);
        return _storage._value;
    }
    [[nodiscard]] constexpr Value value_or(Value const& val_on_error) const { return _is_value ? _storage._value : val_on_error; }

    [[nodiscard]] constexpr Error& error()
    {
        CC_ASSERT(!_is_value);
        return _storage._error;
    }
    [[nodiscard]] constexpr Error const& error() const
    {
        CC_ASSERT(!_is_value);
        return _storage._error;
    }

    // ctors
public:
    constexpr result() { new (cc::placement_new, &_storage._error) Error(); }
    constexpr ~result() { impl_dtor(); }

    constexpr result(Value v)
    {
        new (cc::placement_new, &_storage._value) Value(cc::move(v));
        _is_value = true;
    }
    constexpr result(Error e) { new (cc::placement_new, &_storage._error) Error(cc::move(e)); }

    constexpr result(result const& rhs)
    {
        _is_value = rhs._is_value;
        if (rhs._is_value)
            new (cc::placement_new, &_storage._value) Value(rhs._storage._value);
        else
            new (cc::placement_new, &_storage._error) Error(rhs._storage._error);
    }
    constexpr result(result&& rhs)
    {
        _is_value = rhs._is_value;
        if (rhs._is_value)
            new (cc::placement_new, &_storage._value) Value(cc::move(rhs._storage._value));
        else
            new (cc::placement_new, &_storage._error) Error(cc::move(rhs._storage._error));
    }

    constexpr result& operator=(result const& rhs)
    {
        if (this != &rhs)
        {
            impl_dtor();

            _is_value = rhs._is_value;
            if (rhs._is_value)
                new (cc::placement_new, &_storage._value) Value(rhs._storage._value);
            else
                new (cc::placement_new, &_storage._error) Error(rhs._storage._error);
        }

        return *this;
    }
    constexpr result& operator=(result&& rhs)
    {
        // no self-check required because move-on-self is allowed to be moved-from
        impl_dtor();

        _is_value = rhs._is_value;
        if (rhs._is_value)
            new (cc::placement_new, &_storage._value) Value(cc::move(rhs._storage._value));
        else
            new (cc::placement_new, &_storage._error) Error(cc::move(rhs._storage._error));

        return *this;
    }

private:
    union storage_t
    {
        Error _error;
        Value _value;

        // handled in result
        storage_t() {}
        ~storage_t() {}
    } _storage;
    bool _is_value = false;

    constexpr void impl_dtor()
    {
        if (_is_value)
            _storage._value.~Value();
        else
            _storage._error.~Error();
    }
};
} // namespace cc
