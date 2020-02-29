#pragma once

#include <clean-core/array.hh>
#include <clean-core/equal_to.hh>
#include <clean-core/forward_list.hh>
#include <clean-core/fwd.hh>
#include <clean-core/hash.hh>

namespace cc
{
template <class KeyT, class ValueT, class HashT, class EqualT>
struct map
{
    // container
public:
    size_t size() const { return _size; }
    bool empty() const { return _size == 0; }

    template <class T>
    bool contains_key(T const& key) const
    {
        if (_size == 0)
            return false;

        auto idx = this->_get_location(key);
        for (auto const& e : _entries[idx])
            if (EqualT{}(e.key, key))
                return true;

        return false;
    }

    // ctors
public:
    map() = default;

    // operators
public:
    template <class T>
    ValueT& operator[](T const& key)
    {
        if (_size >= _entries.size())
            _reserve(_size == 0 ? 4 : _size * 2);

        auto idx = this->_get_location(key);
        auto& l = _entries[idx];
        for (auto& e : l)
            if (EqualT{}(e.key, key))
                return e.value;

        ++_size;
        return l.emplace_front(KeyT(key)).value;
    }

    // helper
private:
    template <class T>
    size_t _get_location(T const& key) const
    {
        CC_ASSERT(_entries.size() > 0);
        auto hash = HashT{}(key);
        hash = cc::hash_combine(hash, 0); // scramble a bit
        return hash % _entries.size();
    }

    void _reserve(size_t new_cap)
    {
        auto old_entries = cc::move(_entries);
        _entries = cc::array<cc::forward_list<entry>>::defaulted(new_cap);
        for (auto& l : old_entries)
            for (auto& e : l)
            {
                auto idx = this->_get_location(e.key);
                _entries[idx].emplace_front(cc::move(e.key), cc::move(e.value));
            }
    }

    // member
private:
    struct entry
    {
        KeyT key;
        ValueT value;

        entry(KeyT&& key) : key(cc::move(key)) {}
        entry(KeyT&& key, ValueT&& value) : key(cc::move(key)), value(cc::move(value)) {}
    };
    cc::array<cc::forward_list<entry>> _entries;
    size_t _size = 0;
};
}
