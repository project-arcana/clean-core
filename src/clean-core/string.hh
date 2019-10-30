#pragma once

#include <cstring>

#include <clean-core/assert.hh>
#include <clean-core/macros.hh>
#include <clean-core/string_view.hh>
#include <clean-core/typedefs.hh>

namespace cc
{
/**
 * utf8 null-terminated string with small-string-optimizations
 *
 * TODO: maybe a different SBO strategy
 *       https://stackoverflow.com/questions/27631065/why-does-libcs-implementation-of-stdstring-take-up-3x-memory-as-libstdc/28003328#28003328
 *       https://stackoverflow.com/questions/10315041/meaning-of-acronym-sso-in-the-context-of-stdstring/10319672#10319672
 *       maybe via template arg the sbo capacity?
 */
struct string
{
private:
    enum
    {
        sbo_capacity = 23,
        short_flag = 0x01
    };

    // properties
public:
    char* data() { return _is_short() ? &_rep.s.data[0] : _rep.l.data; }
    char const* data() const { return _is_short() ? &_rep.s.data[0] : _rep.l.data; }

    char const* c_str() const { return data(); }

    char* begin() { return data(); }
    char const* begin() const { return data(); }
    char* end() { return data() + size(); }
    char const* end() const { return data() + size(); }

    size_t size() const { return _is_short() ? _rep.s.size >> 1 : _rep.l.size; }
    size_t capacity() const { return _is_short() ? sbo_capacity - 1 : _rep.l.capacity; }

    bool empty() { return size() == 0; }

    char& front()
    {
        CC_CONTRACT(size() > 0);
        return data()[0];
    }
    char const& front() const
    {
        CC_CONTRACT(size() > 0);
        return data()[0];
    }

    char& back()
    {
        CC_CONTRACT(size() > 0);
        return data()[size() - 1];
    }
    char const& back() const
    {
        CC_CONTRACT(size() > 0);
        return data()[size() - 1];
    }

    char& operator[](size_t i)
    {
        CC_CONTRACT(i < size());
        return data()[i];
    }
    char const& operator[](size_t i) const
    {
        CC_CONTRACT(i < size());
        return data()[i];
    }

    // ctors
public:
    string() { _rep.r = {}; }

    // TODO: more constructors (char const*, string_view)

    [[nodiscard]] static string uninitialized(size_t size)
    {
        string s;
        if (size < sbo_capacity)
            s._rep.s.size = uint8(short_flag + (size << 1));
        else
        {
            s._rep.l.size = size;
            s._rep.l.capacity = size;
            s._rep.l.data = new char[size];
        }
        return s;
    }

    [[nodiscard]] static string filled(size_t size, char value)
    {
        string s;
        s.resize(size, value);
        return s;
    }

    string(string const& rhs)
    {
        if (rhs._is_short())
            _rep.r = rhs._rep.r;
        else if (rhs._rep.l.data == nullptr)
            _rep.r = {};
        else
        {
            _rep.l.data = new char[rhs._rep.l.size];
            std::memcpy(_rep.l.data, rhs._rep.l.data, rhs._rep.l.size);
            _rep.l.size = rhs._rep.l.size;
            _rep.l.capacity = rhs._rep.l.size;
        }
    }
    string(string&& rhs) noexcept
    {
        _rep.r = rhs._rep.r;
        rhs._rep.r = {};
    }

    string& operator=(string const& rhs)
    {
        if (this != &rhs)
        {
            if (!_is_short())
                delete[] _rep.l.data;

            if (rhs._is_short())
                _rep.r = rhs._rep.r;
            else if (rhs._rep.l.data == nullptr)
                _rep.r = {};
            else
            {
                _rep.l.data = new char[rhs._rep.l.size];
                std::memcpy(_rep.l.data, rhs._rep.l.data, rhs._rep.l.size);
                _rep.l.size = rhs._rep.l.size;
                _rep.l.capacity = rhs._rep.l.size;
            }
        }
        return *this;
    }
    string& operator=(string&& rhs) noexcept
    {
        if (!_is_short())
        {
            delete[] _rep.l.data;
            _rep.r = {}; // make sure self-move doesn't crash
        }

        _rep.r = rhs._rep.r;
        rhs._rep.r = {};

        return *this;
    }

    ~string()
    {
        if (!_is_short())
            delete[] _rep.l.data;
    }

    // methods
public:
    void push_back(char c)
    {
        if (size() == capacity())
            _grow();

        if (_is_short())
        {
            auto s = _rep.s.size >> 1;
            _rep.s.data[s] = c;
            _rep.s.data[s + 1] = '\0';
            _rep.s.size += 2; // actually +1 because of flag
        }
        else
        {
            auto d = _rep.l.data;
            auto s = _rep.l.size;
            d[s] = c;
            d[s + 1] = '\0';
            _rep.l.size += 1;
        }
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

        if (_is_short())
        {
            _rep.s.size -= 2; // actually -1 because of flag
            _rep.s.data[_rep.s.size >> 1] = '\0';
        }
        else
        {
            _rep.l.size -= 1;
            _rep.l.data[_rep.l.size] = '\0';
        }
    }

    void resize(size_t new_size, char fill = '\0')
    {
        reserve(new_size);
        auto old_size = size();
        auto d = data();
        if (new_size > old_size)
            std::memset(d + old_size, fill, new_size - old_size);
        d[new_size] = '\0';
    }

    void clear()
    {
        if (!_is_short())
            delete[] _rep.l.data;
        _rep.r = {};
    }

    void shrink_to_fit()
    {
        if (_is_short())
            return; // noop

        auto s = size();
        auto c = capacity();
        if (s == c)
            return; // fit

        auto old_data = data();

        if (s < sbo_capacity)
        {
            _rep.s.size = uint8(short_flag + (s << 1));
            std::memcpy(_rep.s.data, old_data, s + 1);
        }
        else
        {
            _rep.l.data = new char[s + 1];
            std::memcpy(_rep.l.data, old_data, s + 1);
            _rep.l.size = s;
            _rep.l.capacity = s;
        }
    }

    // operators
public:
    bool operator==(string_view rhs) const { return string_view(data(), size()) == rhs; }
    bool operator!=(string_view rhs) const { return string_view(data(), size()) != rhs; }

    // TODO: op+, op+=, comparisons

    // helper
private:
    bool _is_short() const { return _rep.s.size & short_flag; }

    void _reserve_force(size_t new_capacity)
    {
        auto old_size = size();
        auto new_data = new char[new_capacity + 1];

        std::memcpy(new_data, data(), old_size + 1);

        _rep.l.data = new_data;
        _rep.l.size = old_size;
        _rep.l.capacity = new_capacity;
    }

    void _grow()
    {
        if (_is_short())
            _reserve_force(sbo_capacity << 1);
        else if (_rep.l.data == nullptr)
            _rep.s.size = short_flag; // make short
        else
            _reserve_force(_rep.l.capacity << 1);
    }

    // members
private:
    static_assert(sizeof(void*) == 8, "only 64bit supported");
    struct _long
    {
        char* data;
        size_t size;
        size_t capacity;
    };
    struct _short
    {
        uint8 size;
        char data[sbo_capacity];
    };
    struct _raw
    {
        size_t words[3];
    };
    union {
        _long l;
        _short s;
        _raw r;
    } _rep;
};

static_assert(sizeof(string) == 3 * 8, "wrong architecture?");
}
