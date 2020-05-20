#pragma once

#include <clean-core/array.hh>
#include <clean-core/detail/srange.hh>
#include <clean-core/equal_to.hh>
#include <clean-core/forward_list.hh>
#include <clean-core/fwd.hh>
#include <clean-core/hash.hh>
#include <clean-core/sentinel.hh>

namespace cc
{
template <class KeyT, class ValueT, class HashT, class EqualT>
struct map
{
    static_assert(!std::is_reference_v<KeyT>, "keys cannot be references");

    // container
public:
    size_t size() const { return _size; }
    bool empty() const { return _size == 0; }

    template <class T = KeyT>
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
    template <class T = KeyT>
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

    /// looks up the given key and returns the element
    /// UB if key is not present
    template <class T = KeyT>
    ValueT& get(T const& key)
    {
        auto idx = this->_get_location(key);
        for (auto& e : _entries[idx])
            if (EqualT{}(e.key, key))
                return e.value;

        CC_UNREACHABLE("key not found");
    }
    template <class T = KeyT>
    ValueT const& get(T const& key) const
    {
        auto idx = this->_get_location(key);
        for (auto& e : _entries[idx])
            if (EqualT{}(e.key, key))
                return e.value;

        CC_UNREACHABLE("key not found");
    }

    /// reserves internal resources to hold at least n elements without forcing a rehash
    void reserve(size_t n) { _reserve(n); }

    void clear()
    {
        _size = 0;
        for (auto& l : _entries)
            l.clear();
    }

    // iteration
private:
    struct entry;
    template <class ValueRefT>
    struct entry_ref
    {
        KeyT const& key;
        ValueRefT value;
    };

public:
    template <class EntryListT>
    struct iterator_base
    {
        using it_t = std::decay_t<decltype(std::declval<EntryListT>().begin())>;

        friend struct map;

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

        iterator_base(EntryListT* curr, EntryListT* last) : curr(curr), last(last) { find_next_it(); }

    protected:
        EntryListT* curr;
        EntryListT* last;
        it_t it;

        void find_next_it()
        {
            while (curr != last)
            {
                it = curr->begin();

                if (it != cc::sentinel{})
                    break; // found valid

                ++curr; // otherwise go to next
            }
        }
    };
    struct iterator : iterator_base<cc::forward_list<entry>>
    {
        using iterator_base<cc::forward_list<entry>>::iterator_base;
        entry_ref<ValueT&> operator*() { return {(*this->it).key, (*this->it).value}; }
    };
    struct const_iterator : iterator_base<cc::forward_list<entry> const>
    {
        using iterator_base<cc::forward_list<entry> const>::iterator_base;
        entry_ref<ValueT const&> operator*() { return {(*this->it).key, (*this->it).value}; }
    };
    struct key_iterator : iterator_base<cc::forward_list<entry> const>
    {
        using iterator_base<cc::forward_list<entry> const>::iterator_base;
        KeyT const& operator*() { return (*this->it).key; }
    };
    struct value_iterator : iterator_base<cc::forward_list<entry>>
    {
        using iterator_base<cc::forward_list<entry>>::iterator_base;
        ValueT& operator*() { return (*this->it).value; }
    };
    struct value_const_iterator : iterator_base<cc::forward_list<entry> const>
    {
        using iterator_base<cc::forward_list<entry> const>::iterator_base;
        ValueT const& operator*() { return (*this->it).value; }
    };
    iterator begin() { return {_entries.begin(), _entries.end()}; }
    const_iterator begin() const { return {_entries.begin(), _entries.end()}; }
    cc::sentinel end() const { return {}; }
    auto keys() const { return detail::srange<key_iterator>(_entries.begin(), _entries.end()); }
    auto values() { return detail::srange<value_iterator>(_entries.begin(), _entries.end()); }
    auto values() const { return detail::srange<value_const_iterator>(_entries.begin(), _entries.end()); }

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

    /// resizes _entries to the given amount of buckets
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

        entry(KeyT key) : key(cc::move(key)), value() {}
        entry(KeyT key, ValueT value) : key(cc::move(key)), value(cc::move(value)) {}
    };
    cc::array<cc::forward_list<entry>> _entries;
    size_t _size = 0;
};
}
