#pragma once

#include <clean-core/assert.hh>
#include <clean-core/char_predicates.hh>
#include <clean-core/enable_if.hh>
#include <clean-core/forward.hh>
#include <clean-core/fwd.hh>
#include <clean-core/is_contiguous_container.hh>
#include <clean-core/move.hh>
#include <clean-core/sentinel.hh>
#include <clean-core/typedefs.hh>

namespace cc
{
enum class split_options
{
    keep_empty,
    skip_empty
};

// a view on an utf-8 string
// is NON-OWNING
// is a view and CANNOT modify the content of the string
// this class is cheap to copy, passing it by reference has no benefits
struct string_view
{
    constexpr string_view() = default;
    constexpr string_view(char const* data)
    {
        _data = data;
        _size = 0;
        while (data[_size] != '\0')
            ++_size;
    }
    constexpr string_view(char const* data, size_t size) : _data(data), _size(size) {}
    constexpr string_view(char const* begin, char const* end) : _data(begin), _size(end - begin) {}

    template <class ContainerT, cc::enable_if<is_contiguous_container<ContainerT, char const>> = true>
    constexpr string_view(ContainerT const& c) : _data(c.data()), _size(c.size())
    {
    }

private:
    template <class Pred>
    struct string_split_range;

    // container
public:
    constexpr char const* begin() const { return _data; }
    constexpr char const* end() const { return _data + _size; }
    constexpr bool empty() const { return _size == 0; }
    constexpr char const* data() const { return _data; }
    constexpr size_t size() const { return _size; }
    constexpr size_t length() const { return _size; }
    constexpr char const& front() const
    {
        CC_CONTRACT(_size > 0);
        return _data[0];
    }
    constexpr char const& back() const
    {
        CC_CONTRACT(_size > 0);
        return _data[_size - 1];
    }

    constexpr char const& operator[](size_t idx) const
    {
        CC_CONTRACT(idx < _size);
        return _data[idx];
    }

    // functions
public:
    constexpr string_view subview(size_t offset, size_t size) const
    {
        CC_CONTRACT(offset <= _size);
        CC_CONTRACT(offset + size <= _size);
        return {_data + offset, size};
    }
    constexpr string_view subview(size_t offset) const
    {
        CC_CONTRACT(offset <= _size);
        return {_data + offset, _size - offset};
    }

    constexpr bool contains(char c) const
    {
        for (auto d : *this)
            if (d == c)
                return true;
        return false;
    }
    constexpr bool contains(string_view s) const
    {
        if (s.size() > _size)
            return false;

        for (size_t i = 0; i < _size - s.size(); ++i)
            if (subview(i, s.size()) == s)
                return true;

        return false;
    }

    constexpr bool starts_with(char c) const { return _size > 0 && front() == c; }
    constexpr bool starts_with(string_view s) const { return _size >= s.size() && subview(0, s.size()) == s; }

    constexpr bool ends_with(char c) const { return _size > 0 && back() == c; }
    constexpr bool ends_with(string_view s) const { return _size >= s.size() && subview(_size - s.size(), s.size()) == s; }

    [[nodiscard]] constexpr auto split(char sep, split_options opts = split_options::keep_empty) const
    {
        return string_split_range(_data, _data + _size, opts, cc::is_equal_fun(sep));
    }
    template <class Pred>
    [[nodiscard]] constexpr auto split(Pred&& pred, split_options opts = split_options::keep_empty) const
    {
        return string_split_range(_data, _data + _size, opts, cc::forward<Pred>(pred));
    }
    [[nodiscard]] constexpr auto split() const { return split(cc::is_space, split_options::skip_empty); }

    template <class Pred>
    [[nodiscard]] constexpr string_view trim_start(Pred&& pred) const
    {
        auto d = _data;
        auto s = _size;
        while (s > 0 && pred(d[0]))
        {
            ++d;
            --s;
        }
        return {d, s};
    }
    [[nodiscard]] constexpr string_view trim_start(char c) const { return trim_start(cc::is_equal_fun(c)); }
    [[nodiscard]] constexpr string_view trim_start() const { return trim_start(cc::is_space); }

    template <class Pred>
    [[nodiscard]] constexpr string_view trim_end(Pred&& pred) const
    {
        auto d = _data;
        auto s = _size;
        while (s > 0 && pred(d[s - 1]))
            --s;
        return {d, s};
    }
    [[nodiscard]] constexpr string_view trim_end(char c) const { return trim_end(cc::is_equal_fun(c)); }
    [[nodiscard]] constexpr string_view trim_end() const { return trim_end(cc::is_space); }

    template <class Pred>
    [[nodiscard]] constexpr string_view trim(Pred&& pred) const
    {
        auto d = _data;
        auto s = _size;
        while (s > 0 && pred(d[0]))
        {
            ++d;
            --s;
        }
        while (s > 0 && pred(d[s - 1]))
            --s;
        return {d, s};
    }
    [[nodiscard]] constexpr string_view trim(char c) const { return trim(cc::is_equal_fun(c)); }
    [[nodiscard]] constexpr string_view trim() const { return trim(cc::is_space); }

    [[nodiscard]] constexpr string_view remove_prefix(size_t n) const
    {
        CC_CONTRACT(_size >= n);
        return {_data + n, _size - n};
    }
    [[nodiscard]] constexpr string_view remove_prefix(string_view s) const
    {
        CC_CONTRACT(starts_with(s));
        return {_data + s._size, _size - s._size};
    }

    [[nodiscard]] constexpr string_view remove_suffix(size_t n) const
    {
        CC_CONTRACT(_size >= n);
        return {_data, _size - n};
    }
    [[nodiscard]] constexpr string_view remove_suffix(string_view s) const
    {
        CC_CONTRACT(ends_with(s));
        return {_data, _size - s._size};
    }

    [[nodiscard]] constexpr string_view first(size_t n) const { return {_data, n < _size ? n : _size}; }
    [[nodiscard]] constexpr string_view last(size_t n) const { return n <= _size ? string_view(_data + _size - n, n) : *this; }

    // operators
public:
    constexpr bool operator==(string_view rhs) const
    {
        if (_size != rhs._size)
            return false;
        for (size_t i = 0; i != _size; ++i)
            if (_data[i] != rhs._data[i])
                return false;
        return true;
    }
    constexpr bool operator!=(string_view rhs) const
    {
        if (_size != rhs._size)
            return true;
        for (size_t i = 0; i != _size; ++i)
            if (_data[i] != rhs._data[i])
                return true;
        return false;
    }

    template <size_t N>
    constexpr bool operator==(char const (&rhs)[N]) const
    {
        if (N - 1 != _size)
            return false;
        for (size_t i = 0; i != _size; ++i)
            if (_data[i] != rhs[i])
                return false;
        return true;
    }
    template <size_t N>
    constexpr bool operator!=(char const (&rhs)[N]) const
    {
        if (N - 1 != _size)
            return true;
        for (size_t i = 0; i != _size; ++i)
            if (_data[i] != rhs[i])
                return true;
        return false;
    }

private:
    char const* _data = nullptr;
    size_t _size = 0;

    // implementation detail
private:
    template <class Pred>
    struct string_split_iterator
    {
        char const* _prev;
        char const* _curr;
        char const* _end;
        split_options _options;
        bool _finished = false;
        Pred& _pred;
        constexpr string_split_iterator(char const* begin, char const* end, split_options options, Pred& pred)
          : _prev(begin), _curr(begin), _end(end), _options(options), _pred(pred)
        {
            if (options == split_options::skip_empty)
            {
                while (_curr != _end && _pred(*_curr))
                    ++_curr;

                _prev = _curr;
            }

            if (_curr == end)
                _finished = true;
            else
                while (_curr != _end && !_pred(*_curr))
                    ++_curr;
        }

        constexpr void operator++()
        {
            if (_curr == _end)
            {
                _finished = true;
                return;
            }

            ++_curr;

            if (_options == split_options::skip_empty)
            {
                while (_curr != _end && _pred(*_curr))
                    ++_curr;
                if (_curr == _end)
                {
                    _finished = true;
                    return;
                }
            }

            _prev = _curr;
            while (_curr != _end && !_pred(*_curr))
                ++_curr;
        }
        constexpr string_view operator*() const { return {_prev, _curr}; }
        constexpr bool operator!=(cc::sentinel) const { return !_finished; }
    };
    template <class Pred>
    struct string_split_range
    {
        char const* _begin;
        char const* _end;
        split_options _options;
        Pred _pred;

        constexpr string_split_range(char const* b, char const* e, split_options o, Pred pred)
          : _begin(b), _end(e), _options(o), _pred(cc::move(pred))
        {
        }
        constexpr auto begin() { return string_split_iterator(_begin, _end, _options, _pred); }
        constexpr auto begin() const { return string_split_iterator(_begin, _end, _options, _pred); }
        constexpr cc::sentinel end() const { return {}; }
    };
};
}
