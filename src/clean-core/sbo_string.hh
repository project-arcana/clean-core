#pragma once

#include <cstring>

#include <clean-core/assert.hh>
#include <clean-core/fwd.hh>
#include <clean-core/hash_combine.hh>
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
        _sbo_words = {}; // also sets capacity to 0
        _data = _sbo;
    }

    sbo_string(char const* s)
    {
        _size = std::strlen(s);
        _sbo_words = {}; // also sets capacity to 0

        if (_size <= sbo_capacity)
            _data = _sbo;
        else
        {
            _capacity = _size;
            _data = new char[_size + 1];
        }

        std::memcpy(_data, s, _size);
        _data[_size] = '\0';
    }
    sbo_string(char const* s, size_t size)
    {
        _size = size;
        _sbo_words = {}; // also sets capacity to 0

        if (_size <= sbo_capacity)
            _data = _sbo;
        else
        {
            _capacity = _size;
            _data = new char[_size + 1];
        }

        if (_size > 0)
            std::memcpy(_data, s, _size);
        _data[_size] = '\0';
    }
    sbo_string(string_view s) : sbo_string(s.data(), s.size()) {}

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
        _sbo_words = {}; // also sets capacity to 0

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
        _sbo_words = {}; // also sets capacity to 0

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
        if (this == &rhs)
            return *this;

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

    // string processing
public:
    string_view subview(size_t offset, size_t size) const { return string_view(_data, _size).subview(offset, size); }
    string_view subview(size_t offset) const { return string_view(_data, _size).subview(offset); }
    sbo_string substring(size_t offset, size_t size) const { return subview(offset, size); }
    sbo_string substring(size_t offset) const { return subview(offset); }
    bool contains(char c) const { return string_view(_data, _size).contains(c); }
    bool contains(string_view s) const { return string_view(_data, _size).contains(s); }
    bool starts_with(char c) const { return string_view(_data, _size).starts_with(c); }
    bool starts_with(string_view s) const { return string_view(_data, _size).starts_with(s); }
    bool ends_with(char c) const { return string_view(_data, _size).ends_with(c); }
    bool ends_with(string_view s) const { return string_view(_data, _size).ends_with(s); }

    [[nodiscard]] auto split(char sep, split_options opts = split_options::keep_empty) const { return string_view(_data, _size).split(sep, opts); }
    template <class Pred>
    [[nodiscard]] auto split(Pred&& pred, split_options opts = split_options::keep_empty) const
    {
        return string_view(_data, _size).split(pred, opts);
    }
    [[nodiscard]] auto split() const { return string_view(_data, _size).split(cc::is_space, split_options::skip_empty); }

    void fill(char c, size_t n = dynamic_size)
    {
        if (n <= capacity())
        {
            _size = n;
            _data[n] = '\0';
        }
        else if (n != dynamic_size)
        {
            if (!_is_short())
                delete[] _data;

            CC_ASSERT(n >= sbo_capacity);
            _data = new char[n + 1];
            _data[n] = '\0';
            _size = n;
            _capacity = n;
        }

        for (size_t i = 0; i < _size; ++i)
            _data[i] = c;
    }

    [[nodiscard]] sbo_string to_lower() const
    {
        auto r = uninitialized(_size);
        for (size_t i = 0; i < _size; ++i)
            r._data[i] = cc::to_lower(_data[i]);
        return r;
    }
    [[nodiscard]] sbo_string to_upper() const
    {
        auto r = uninitialized(_size);
        for (size_t i = 0; i < _size; ++i)
            r._data[i] = cc::to_upper(_data[i]);
        return r;
    }
    void capitalize()
    {
        if (_size > 0)
            _data[0] = cc::to_upper(_data[0]);
        for (size_t i = 1; i < _size; ++i)
            _data[i] = cc::to_lower(_data[i]);
    }
    [[nodiscard]] sbo_string capitalized() const
    {
        auto r = uninitialized(_size);
        if (_size > 0)
            r._data[0] = cc::to_upper(_data[0]);
        for (size_t i = 1; i < _size; ++i)
            r._data[i] = cc::to_lower(_data[i]);
        return r;
    }

    void remove_prefix(size_t n)
    {
        CC_CONTRACT(_size >= n);
        for (size_t i = 0; i < _size - n; ++i)
            _data[i] = _data[i + n];
        _size -= n;
    }
    void remove_prefix(string_view s)
    {
        CC_CONTRACT(starts_with(s));
        remove_prefix(s.size());
    }

    void remove_suffix(size_t n)
    {
        CC_CONTRACT(_size >= n);
        _size -= n;
    }
    void remove_suffix(string_view s)
    {
        CC_CONTRACT(ends_with(s));
        _size -= s.size();
    }

    [[nodiscard]] sbo_string removed_prefix(size_t n) const { return string_view(_data, _size).remove_prefix(n); }
    [[nodiscard]] sbo_string removed_prefix(string_view s) const { return string_view(_data, _size).remove_prefix(s); }
    [[nodiscard]] sbo_string removed_suffix(size_t n) const { return string_view(_data, _size).remove_suffix(n); }
    [[nodiscard]] sbo_string removed_suffix(string_view s) const { return string_view(_data, _size).remove_suffix(s); }

    [[nodiscard]] sbo_string first(size_t n) const { return string_view(_data, _size).first(n); }
    [[nodiscard]] sbo_string last(size_t n) const { return string_view(_data, _size).last(n); }

    template <class Pred>
    void trim_start(Pred&& pred)
    {
        size_t n = 0;
        while (n < _size && pred(_data[n]))
            ++n;

        if (n == 0)
            return; // no change

        for (size_t i = 0; i < _size - n; ++i)
            _data[i] = _data[i + n];
        _size -= n;
        _data[_size] = '\0';
    }
    void trim_start(char c) { return trim_start(cc::is_equal_fun(c)); }
    void trim_start() { return trim_start(cc::is_space); }

    template <class Pred>
    void trim_end(Pred&& pred)
    {
        while (_size > 0 && pred(_data[_size - 1]))
            --_size;
        _data[_size] = '\0';
    }
    void trim_end(char c) { trim_end(cc::is_equal_fun(c)); }
    void trim_end() { trim_end(cc::is_space); }

    template <class Pred>
    void trim(Pred&& pred)
    {
        size_t n = 0;
        while (n < _size && pred(_data[n]))
            ++n;
        size_t s = _size - n;
        while (s > 0 && pred(_data[n + s - 1]))
            --s;
        for (size_t i = 0; i < s; ++i)
            _data[i] = _data[i + n];
        _data[s] = '\0';
        _size = s;
    }
    void trim(char c) { trim(cc::is_equal_fun(c)); }
    void trim() { trim(cc::is_space); }

    template <class Pred>
    [[nodiscard]] sbo_string trimmed_start(Pred&& pred) const
    {
        return string_view(_data, _size).trim_start(pred);
    }
    [[nodiscard]] sbo_string trimmed_start(char c) const { return string_view(_data, _size).trim_start(c); }
    [[nodiscard]] sbo_string trimmed_start() const { return string_view(_data, _size).trim_start(cc::is_space); }

    template <class Pred>
    [[nodiscard]] sbo_string trimmed_end(Pred&& pred) const
    {
        return string_view(_data, _size).trim_end(pred);
    }
    [[nodiscard]] sbo_string trimmed_end(char c) const { return string_view(_data, _size).trim_end(c); }
    [[nodiscard]] sbo_string trimmed_end() const { return string_view(_data, _size).trim_end(cc::is_space); }

    template <class Pred>
    [[nodiscard]] sbo_string trimmed(Pred&& pred) const
    {
        return string_view(_data, _size).trim(pred);
    }
    [[nodiscard]] sbo_string trimmed(char c) const { return string_view(_data, _size).trim(c); }
    [[nodiscard]] sbo_string trimmed() const { return string_view(_data, _size).trim(cc::is_space); }

    void pad_start(size_t length, char c = ' ')
    {
        if (_size >= length)
            return;

        reserve(length);

        for (size_t i = 1; i <= _size; ++i)
            _data[length - i] = _data[_size - i];
        for (size_t i = 0; i < length - _size; ++i)
            _data[i] = c;

        _size = length;
        _data[_size] = '\0';
    }
    void pad_end(size_t length, char c = ' ')
    {
        if (_size >= length)
            return;

        reserve(length);

        for (size_t i = _size; i < length; ++i)
            _data[i] = c;

        _size = length;
        _data[_size] = '\0';
    }

    void replace(char old, char replacement)
    {
        for (auto& c : *this)
            if (c == old)
                c = replacement;
    }
    void replace(size_t pos, size_t count, string_view replacement)
    {
        CC_CONTRACT(pos <= _size);
        CC_CONTRACT(pos + count <= _size);

        // early out: replace same size
        if (count == replacement.size())
        {
            std::memcpy(_data + pos, replacement.data(), replacement.size());
            return;
        }

        auto new_size = _size - count + replacement.size();
        auto new_cap = capacity();

        if (new_size > new_cap) // realloc
        {
            // set to new size but at least double
            new_cap = new_cap << 1;
            if (new_size > new_cap)
                new_cap = new_size;

            auto new_data = new char[new_cap + 1];

            std::memcpy(new_data, _data, pos);
            std::memcpy(new_data + pos, replacement.data(), replacement.size());
            std::memcpy(new_data + pos + replacement.size(), _data + pos + count, _size - count - pos);
            new_data[new_size] = '\0';

            if (!_is_short())
                delete[] _data;

            _size = new_size;
            _data = new_data;
            _capacity = new_cap;
        }
        else
        {
            if (replacement.size() > count) // enlarge
            {
                for (size_t i = 1; i <= _size - count - pos; ++i)
                    _data[new_size - i] = _data[_size - i];
            }
            else // shrink
            {
                for (size_t i = 0; i < _size - count - pos; ++i)
                    _data[pos + replacement.size() + i] = _data[pos + count + i];
            }
            std::memcpy(_data + pos, replacement.data(), replacement.size());

            _size = new_size;
            _data[new_size] = '\0';
        }
    }
    void replace(string_view old, string_view replacement)
    {
        CC_CONTRACT(!old.empty());

        if (old.size() > _size)
            return; // cannot replace anything

        size_t i = 0;
        auto os = old.size();
        while (i + os <= _size)
        {
            if (subview(i, os) == old) // found
            {
                replace(i, os, replacement);
                i += replacement.size();
            }
            else
                ++i;
        }
    }

    [[nodiscard]] sbo_string replaced(char old, char replacement) const
    {
        auto r = uninitialized(_size);
        for (size_t i = 0; i < _size; ++i)
        {
            auto c = _data[i];
            r._data[i] = c == old ? replacement : c;
        }
        return r;
    }
    [[nodiscard]] sbo_string replaced(size_t pos, size_t count, string_view replacement) const
    {
        CC_CONTRACT(pos <= _size);
        CC_CONTRACT(pos + count <= _size);
        auto r = uninitialized(_size - count + replacement.size());
        std::memcpy(r._data, _data, pos);
        std::memcpy(r._data + pos, replacement.data(), replacement.size());
        std::memcpy(r._data + pos + replacement.size(), _data + pos + count, _size - count - pos);
        return r;
    }
    [[nodiscard]] sbo_string replaced(string_view old, string_view replacement) const
    {
        CC_CONTRACT(!old.empty());
        if (old.size() > _size)
            return *this; // early out

        sbo_string r;
        size_t i = 0;
        auto const os = old.size();
        while (i < _size)
        {
            if (subview(i, os) == old) // found
            {
                r += replacement;
                i += old.size();
            }
            else
            {
                r += _data[i];
                ++i;
            }
        }
        return r;
    }

    void insert(size_t pos, string_view s) { replace(pos, 0, s); }

    // operators
public:
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

            // take new size but at least double cap
            new_cap = new_cap << 1;
            if (new_cap < new_size)
                new_cap = new_size;

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

    template <size_t B>
    bool operator==(sbo_string<B> const& rhs) const
    {
        return string_view(*this) == string_view(rhs);
    }
    template <size_t B>
    bool operator!=(sbo_string<B> const& rhs) const
    {
        return string_view(*this) != string_view(rhs);
    }

    friend sbo_string operator+(sbo_string lhs, string_view rhs)
    {
        lhs += rhs;
        return lhs;
    }
    template <size_t C2>
    friend sbo_string operator+(sbo_string lhs, sbo_string<C2> const& rhs)
    {
        lhs += rhs;
        return lhs;
    }
    friend sbo_string operator+(sbo_string lhs, char rhs)
    {
        lhs += rhs;
        return lhs;
    }
    friend sbo_string operator+(char lhs, sbo_string const& rhs)
    {
        auto r = uninitialized(1 + rhs.size());
        auto d = r.data();
        d[0] = lhs;
        std::memcpy(d + 1, rhs.data(), rhs.size());
        return r;
    }
    friend sbo_string operator+(string_view lhs, sbo_string const& rhs)
    {
        auto r = uninitialized(lhs.size() + rhs.size());
        auto d = r.data();
        std::memcpy(d, lhs.data(), lhs.size());
        std::memcpy(d + lhs.size(), rhs.data(), rhs.size());
        return r;
    }

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

// hash
template <size_t sbo_capacity>
struct hash<sbo_string<sbo_capacity>>
{
    [[nodiscard]] constexpr hash_t operator()(sbo_string<sbo_capacity> const& a) const noexcept
    {
        // TODO: better string hash
        size_t h = 0;
        for (auto const& c : a)
            h = cc::hash_combine(h, c);
        return h;
    }
};
}
