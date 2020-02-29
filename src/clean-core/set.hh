#pragma once

#include <clean-core/array.hh>
#include <clean-core/equal_to.hh>
#include <clean-core/forward_list.hh>
#include <clean-core/fwd.hh>
#include <clean-core/hash.hh>

namespace cc
{
template <class T, class HashT, class EqualT>
struct set
{
    // container
public:
    size_t size() const { return _size; }
    bool empty() const { return _size == 0; }

    template <class U>
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

    // operators
public:
    void add(T const& value)
    {
        if (_size >= _entries.size())
            _reserve(_size == 0 ? 4 : _size * 2);

        auto idx = this->_get_location(value);
        auto& l = _entries[idx];
        for (auto& e : l)
            if (EqualT{}(e, value))
                return; // already contained

        ++_size;
        l.emplace_front(value);
    }

    /// removes an element from the set
    /// returns true iff something was removed
    /// supports heterogeneous lookup
    template <class U>
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
