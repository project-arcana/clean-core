#pragma once

#include <initializer_list>

#include <clean-core/array.hh>
#include <clean-core/equal_to.hh>
#include <clean-core/forward_list.hh>
#include <clean-core/fwd.hh>
#include <clean-core/hash.hh>
#include <clean-core/is_range.hh>
#include <clean-core/sentinel.hh>

namespace cc
{
template <class T, class HashT, class EqualT>
struct set
{
    // container
public:
    size_t size() const { return _size; }
    bool empty() const { return _size == 0; }

    template <class U = T>
    bool contains(U const& value) const
    {
        if (_size == 0)
            return false;

        auto idx = this->_get_location(value);
        for (auto const& e : _entries[idx])
            if (EqualT{}(e, value))
                return true;

        return false;
    }

    // ctors
public:
    set() = default;
    
    set(set&&) = default;
    set(set const&) = default;
    set& operator=(set&&) = default;
    set& operator=(set const&) = default;

    /// constructs a set by adding all elements of the range
    /// TODO: proper support for move-only types
    template <class Range, cc::enable_if<cc::is_range<Range, T const>> = true>
    explicit set(Range&& range)
    {
        for (auto&& e : range)
            this->add(e);
    }

    /// constructs a set by adding all elements of the range
    set(std::initializer_list<T> values)
    {
        for (auto&& e : values)
            this->add(e);
    }

    // operators
public:
    /// adds a value to the set
    /// returns true if already contained
    /// TODO: proper support for move-only types
    bool add(T const& value)
    {
        if (_size >= _entries.size())
            _reserve(_size == 0 ? 4 : _size * 2);

        auto idx = this->_get_location(value);
        auto& l = _entries[idx];
        for (auto& e : l)
            if (EqualT{}(e, value))
                return false; // already contained

        ++_size;
        l.emplace_front(value);
        return true;
    }

    /// removes an element from the set
    /// returns true iff something was removed
    /// supports heterogeneous lookup
    template <class U = T>
    bool remove(U const& value)
    {
        if (_size == 0)
            return false;

        auto idx = this->_get_location(value);
        auto& list = _entries[idx];
        auto it = list.begin();

        if (EqualT{}(*it, value))
        {
            list.pop_front();
            --_size;
            return true;
        }

        auto prev = it;
        auto end = list.end();
        while (it != end)
        {
            if (EqualT{}(*it, value))
            {
                list.erase_after(prev);
                --_size;
                return true;
            }

            prev = it;
            ++it;
        }

        return false;
    }

    /// adds a value to this set
    set& operator|=(T const& value)
    {
        add(value);
        return *this;
    }
    /// adds all values of the range to this set
    /// TODO: proper support for move-only types
    template <class Range, cc::enable_if<cc::is_range<Range, T const>> = true>
    set& operator|=(Range&& range)
    {
        for (auto&& e : range)
            this->add(e);
        return *this;
    }
    /// adds all values of the range to this set
    set& operator|=(std::initializer_list<T> range)
    {
        for (auto&& e : range)
            this->add(e);
        return *this;
    }
    /// returns a set that is the union of lhs and rhs
    template <class Range, cc::enable_if<cc::is_range<Range, T const>> = true>
    set operator|(Range&& rhs) const
    {
        auto r = *this; // copy
        r |= cc::forward<Range>(rhs);
        return r; // guaranteed copy elision
    }

    /// reserves internal resources to hold at least n elements without forcing a rehash
    void reserve(size_t n) { _reserve(n); }

    template <class U>
    bool operator==(set<U> const& rhs) const
    {
        if (_size != rhs._size)
            return false;

        for (auto const& l : rhs._entries)
            for (auto const& v : l)
                if (!this->contains(v))
                    return false;

        return true;
    }
    template <class U>
    bool operator!=(set<U> const& rhs) const
    {
        return !this->operator==(rhs);
    }

    void clear()
    {
        _size = 0;
        for (auto& l : _entries)
            l.clear();
    }

    // iteration
public:
    struct iterator
    {
        friend struct set;

        T const& operator*() { return *it; }
        void operator++()
        {
            ++it;

            if (!(it != cc::sentinel{}))
            {
                ++curr;
                find_next_it();
            }
        }
        bool operator!=(cc::sentinel) const { return curr != last; }

    private:
        iterator(set const& s)
        {
            curr = s._entries.begin();
            last = s._entries.end();
            find_next_it();
        }
        cc::forward_list<T> const* curr;
        cc::forward_list<T> const* last;
        typename cc::forward_list<T>::const_iterator it;

        void find_next_it()
        {
            while (curr != last)
            {
                it = curr->begin();

                if (it != cc::sentinel{})
                    break; // found valid

                curr++; // otherwise go to next
            }
        }
    };
    iterator begin() const { return {*this}; }
    cc::sentinel end() const { return {}; }

    // helper
private:
    template <class U>
    size_t _get_location(U const& value) const
    {
        CC_ASSERT(_entries.size() > 0);
        auto hash = HashT{}(value);
        hash = cc::hash_combine(hash, 0); // scramble a bit
        return hash % _entries.size();
    }

    void _reserve(size_t new_cap)
    {
        auto old_entries = cc::move(_entries);
        _entries = cc::array<cc::forward_list<T>>::defaulted(new_cap);
        for (auto& l : old_entries)
            for (auto& e : l)
            {
                auto idx = this->_get_location(e);
                _entries[idx].emplace_front(cc::move(e));
            }
    }

    // member
private:
    cc::array<cc::forward_list<T>> _entries;
    size_t _size = 0;
};
}
