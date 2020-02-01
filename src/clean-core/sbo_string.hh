#pragma once

#include <cstring>

#include <clean-core/assert.hh>
#include <clean-core/fwd.hh>
#include <clean-core/macros.hh>
#include <clean-core/string_view.hh>
#include <clean-core/typedefs.hh>

namespace cc
{
/**
 * utf8 null-terminated sbo_string with small-sbo_string-optimizations
 *
 * TODO: maybe a different SBO strategy
 *       https://stackoverflow.com/questions/27631065/why-does-libcs-implementation-of-stdstring-take-up-3x-memory-as-libstdc/28003328#28003328
 *       https://stackoverflow.com/questions/10315041/meaning-of-acronym-sso-in-the-context-of-stdstring/10319672#10319672
 *       maybe via template arg the sbo capacity?
 */
template <size_t sbo_capacity>
struct sbo_string
{
    // properties
public:
    char* data() { return _data; }
    char const* data() const { return _data; }

    char const* c_str() const { return _data; }

    char* begin() { return _data; }
    char const* begin() const { return _data; }
    char* end() { return _data + _size; }
    char const* end() const { return _data + _size; }

    size_t size() const { return _size; }
    size_t capacity() const { return _is_short() ? sbo_capacity : _capacity; }

    bool empty() const { return _size == 0; }

    char& front()
    {
        CC_CONTRACT(_size > 0);
        return _data[0];
    }
    char const& front() const
    {
        CC_CONTRACT(_size > 0);
        return _data[0];
    }

    char& back()
    {
        CC_CONTRACT(_size > 0);
        return _data[_size - 1];
    }
    char const& back() const
    {
        CC_CONTRACT(_size > 0);
        return _data[_size - 1];
    }

    char& operator[](size_t i)
    {
        CC_CONTRACT(i < _size);
        return _data[i];
    }
    char const& operator[](size_t i) const
    {
        CC_CONTRACT(i < _size);
        return _data[i];
    }

    // ctors
public:
    sbo_string()
    {
        _size = 0;
        _capacity = 0; // clear first word of sbo
        _data = _sbo;
    }

    sbo_string(string_view s)
    {
        _size = s.size();

        if (_size <= sbo_capacity)
            _data = _sbo;
        else
        {
            _capacity = _size;
            _data = new char[_size + 1];
        }

        std::memcpy(_data, s.data(), _size);
        _data[_size] = '\0';
    }
    sbo_string(char const* s)
    {
        _size = s == nullptr ? 0 : std::strlen(s);

        if (_size <= sbo_capacity)
            _data = _sbo;
        else
        {
            _capacity = _size;
            _data = new char[_size + 1];
        }

        if (s != nullptr)
        {
            std::memcpy(_data, s, _size);
        }
        _data[_size] = '\0';
    }

    [[nodiscard]] static sbo_string uninitialized(size_t size)
    {
        sbo_string s;
        s.reserve(size);
        s._size = size;
        s._data[size] = '\0';
        return s;
    }

    [[nodiscard]] static sbo_string filled(size_t size, char value)
    {
        sbo_string s;
        s.resize(size, value);
        return s;
    }

    sbo_string(sbo_string const& rhs)
    {
        if (rhs._is_short())
        {
            _data = _sbo;
            _size = rhs._size;
            _sbo_words = rhs._sbo_words;
        }
        else if (rhs.size() <= sbo_capacity)
        {
            _data = _sbo;
            _size = rhs.size();
            std::memcpy(_data, rhs._data, _size + 1);
        }
        else
        {
            _size = rhs._size;
            _capacity = rhs._size;
            _data = new char[_size + 1];
            std::memcpy(_data, rhs._data, _size + 1);
        }
    }
    sbo_string(sbo_string&& rhs) noexcept
    {
        _size = rhs._size;
        if (rhs._is_short())
        {
            _data = _sbo;
            _sbo_words = rhs._sbo_words;
        }
        else
        {
            _data = rhs._data;
            _capacity = rhs._capacity;
            rhs._data = nullptr;
        }
    }

    sbo_string& operator=(sbo_string const& rhs)
    {
        if (this != &rhs)
        {
            if (!_is_short())
                delete[] _data;

            if (rhs._is_short())
            {
                _data = _sbo;
                _size = rhs._size;
                _sbo_words = rhs._sbo_words;
            }
            else if (rhs.size() <= sbo_capacity)
            {
                _data = _sbo;
                _size = rhs.size();
                std::memcpy(_data, rhs._data, _size + 1);
            }
            else
            {
                _size = rhs._size;
                _capacity = rhs._size;
                _data = new char[_size + 1];
                std::memcpy(_data, rhs._data, _size + 1);
            }
        }
        return *this;
    }
    sbo_string& operator=(sbo_string&& rhs) noexcept
    {
        if (!_is_short())
        {
            delete[] _data;
            _data = _sbo;
            _capacity = 0; // make sure self-move doesn't crash
        }

        _size = rhs._size;
        if (rhs._is_short())
        {
            _data = _sbo;
            _sbo_words = rhs._sbo_words;
        }
        else
        {
            _data = rhs._data;
            _capacity = rhs._capacity;
            rhs._data = nullptr;
        }

        return *this;
    }

    ~sbo_string()
    {
        if (!_is_short())
            delete[] _data;
    }

    // methods
public:
    void push_back(char c)
    {
        if (_size == capacity())
            _grow();

        _data[_size] = c;
        _size++;
        _data[_size] = '\0';
    }

    void reserve(size_t new_capacity)
    {
        if (new_capacity <= capacity())
            return;

        _reserve_force(new_capacity);
    }

    void pop_back()
    {
        CC_CONTRACT(!empty());

        _size -= 1;
        _data[_size] = '\0';
    }

    void resize(size_t new_size, char fill = '\0')
    {
        reserve(new_size);

        if (new_size > _size)
            std::memset(_data + _size, fill, new_size - _size);
        _data[new_size] = '\0';

        _size = new_size;
    }

    void clear()
    {
        if (!_is_short())
            delete[] _data;

        _data = _sbo;
        _size = 0;
        _capacity = 0;
    }

    void shrink_to_fit()
    {
        if (_is_short())
            return; // noop

        auto const s = _size;
        auto const c = capacity();
        if (s == c)
            return; // fit

        if (s <= sbo_capacity)
        {
            std::memcpy(_sbo, _data, s + 1);

            if (!_is_short())
                delete[] _data;

            _data = _sbo;
        }
        else
        {
            auto new_data = new char[s + 1];
            std::memcpy(new_data, _data, s + 1);

            if (!_is_short())
                delete[] _data;

            _data = new_data;
            _capacity = s;
        }
    }

    // operators
public:
    bool operator==(string_view rhs) const { return string_view(_data, _size) == rhs; }
    bool operator!=(string_view rhs) const { return string_view(_data, _size) != rhs; }

    // TODO: op<, >, ...

    sbo_string& operator+=(char c)
    {
        push_back(c);
        return *this;
    }
    sbo_string& operator+=(string_view s)
    {
        auto new_size = _size + s.size();
        auto new_cap = capacity();

        if (new_size <= new_cap)
        {
            std::memcpy(_data + _size, s.data(), s.size());
            _size = new_size;
            _data[new_size] = '\0';
        }
        else
        {
            CC_ASSERT(new_cap >= sbo_capacity);
            while (new_cap < new_size)
                new_cap = new_cap << 1;
            auto new_data = new char[new_cap + 1];

            std::memcpy(new_data, _data, _size);
            std::memcpy(new_data + _size, s.data(), s.size());
            new_data[new_size] = '\0';

            if (!_is_short())
                delete[] _data;

            _size = new_size;
            _data = new_data;
            _capacity = new_cap;
        }

        return *this;
    }

    operator string_view() const { return string_view(_data, _size); }

    // helper
private:
    bool _is_short() const { return _data == _sbo; }

    void _reserve_force(size_t new_capacity)
    {
        auto new_data = new char[new_capacity + 1];
        auto old_data = _data;

        std::memcpy(new_data, old_data, _size + 1);

        if (!_is_short())
            delete[] _data;

        _data = new_data;
        _capacity = new_capacity;
    }

    void _grow() { _reserve_force(capacity() << 1); }

    // members
private:
    static_assert(sizeof(void*) == 8, "only 64bit supported");
    static_assert((sbo_capacity + 1) % 8 == 0, "only capacities that are one below multiples of 8 make sense");

    struct words
    {
        size_t data[(sbo_capacity + 1) / 8];
    };

    char* _data;
    size_t _size;
    union {
        size_t _capacity;
        char _sbo[sbo_capacity + 1];
        words _sbo_words;
    };
};

template <size_t C>
bool operator==(string_view lhs, sbo_string<C> const& rhs)
{
    return lhs == string_view(rhs);
}
template <size_t C>
bool operator!=(string_view lhs, sbo_string<C> const& rhs)
{
    return lhs != string_view(rhs);
}
template <size_t A, size_t B>
bool operator==(sbo_string<A> const& lhs, sbo_string<B> const& rhs)
{
    return string_view(lhs) == string_view(rhs);
}
template <size_t A, size_t B>
bool operator!=(sbo_string<A> const& lhs, sbo_string<B> const& rhs)
{
    return string_view(lhs) != string_view(rhs);
}

template <size_t C>
[[nodiscard]] sbo_string<C> operator+(sbo_string<C> lhs, string_view rhs)
{
    lhs += rhs;
    return lhs;
}
template <size_t C1, size_t C2>
[[nodiscard]] sbo_string<C1> operator+(sbo_string<C1> lhs, sbo_string<C2> const& rhs)
{
    lhs += rhs;
    return lhs;
}
template <size_t C>
[[nodiscard]] sbo_string<C> operator+(sbo_string<C> lhs, char rhs)
{
    lhs += rhs;
    return lhs;
}
template <size_t C>
[[nodiscard]] sbo_string<C> operator+(string_view lhs, sbo_string<C> const& rhs)
{
    auto r = sbo_string<C>::uninitialized(lhs.size() + rhs.size());
    auto d = r.data();
    std::memcpy(d, lhs.data(), lhs.size());
    std::memcpy(d + lhs.size(), rhs.data(), rhs.size());
    return r;
}
}
