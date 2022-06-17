#pragma once

#include <clean-core/array.hh>
#include <clean-core/detail/srange.hh>
#include <clean-core/equal_to.hh>
#include <clean-core/forward_list.hh>
#include <clean-core/fwd.hh>
#include <clean-core/hash.hh>
#include <clean-core/is_range.hh>
#include <clean-core/pair.hh>
#include <clean-core/sentinel.hh>

namespace cc
{
namespace experimental
{
/**
 * compute an indicator of how far from optimal the distribution of keys in the map is
 *
 * a value of 0 means close-to-optimal
 * a value of 1 means twice as many comparisons than optimal
 * a value of 10 means 11 times as many comparisons than optimal
 *
 * see https://artificial-mind.net/blog/2021/10/09/unordered-map-badness
 */
template <class KeyT, class ValueT, class HashT, class EqualT>
double compute_hash_badness(map<KeyT, ValueT, HashT, EqualT> const& map);
}

// Bucket Interface, similar to std::unordered_map
// NOTE: this is unstable, private API used for testing and enthusiasts
//       it might get replaced or removed without notice or replacement
namespace detail
{
template <class KeyT, class ValueT, class HashT, class EqualT>
size_t bucket_count(map<KeyT, ValueT, HashT, EqualT> const& map);
template <class KeyT, class ValueT, class HashT, class EqualT>
size_t bucket_idx(map<KeyT, ValueT, HashT, EqualT> const& map, KeyT const& key);
template <class KeyT, class ValueT, class HashT, class EqualT>
size_t bucket_size(map<KeyT, ValueT, HashT, EqualT> const& map, size_t bucket_idx);
}

/**
 * A general-purpose hash-based map
 * - hash function and comparison are customizable
 * - provides heterogeneous key lookup by default
 *
 * NOTE:
 * - currently guarantees pointer stability for values (TODO: should we keep this?)
 * - hash is currently NOT transparent (until we resolve the int/float issue)
 *
 * TODO:
 * - emplace functions
 */
template <class KeyT, class ValueT, class HashT, class EqualT>
struct map
{
    using key_t = KeyT;
    using value_t = ValueT;
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
    map(map const&) = default;
    map(map&&) = default;
    map& operator=(map const&) = default;
    map& operator=(map&&) = default;

    /// creates a map and adds all key-value pairs
    map(std::initializer_list<pair<KeyT const, ValueT>> entries)
    {
        reserve(entries.size());
        for (auto&& kvp : entries)
            operator[](kvp.first) = kvp.second;
    }

    /// creates a map from a range with pair-like entries
    /// TODO: more specific SFINAE?
    /// TODO: make sure it doesn't interfere with copy ctor?
    /// TODO: use emplace and move if range is rvalue ref
    template <class Range, cc::enable_if<cc::is_any_range<Range>> = true>
    explicit map(Range&& range)
    {
        for (auto&& [key, value] : range)
            operator[](key) = value;
    }

    // operators
public:
    template <class T = KeyT>
    ValueT& operator[](T const& key)
    {
        // Note: if _size == _entries.size(), we don't want to reserve
        //       because if the key is found in the next step, it would be waste
        //       especially the pattern "reserve(n)" followed by n times op[] is dangerous then
        if (_entries.empty() || _size > _entries.size())
            reserve(_size == 0 ? 4 : _size + 1); // reserve internally pads to power-of-two

        auto idx = this->_get_location(key);
        auto& l = _entries[idx];
        for (auto& e : l)
            if (EqualT{}(e.key, key))
                return e.value;

        ++_size;
        return l.emplace_front(KeyT(key)).value;
    }

    /// looks up the given key
    /// if it does not exists, creates it via "create()"
    /// ("create()" must return something that can construct T)
    /// returns the value in either case
    /// NOTE: create must not modify this map
    ///       during create, the old size is still valid
    template <class T = KeyT, class CreateF>
    ValueT& get_or_create(T const& key, CreateF&& create)
    {
        if (_entries.empty() || _size > _entries.size())
            reserve(_size == 0 ? 4 : _size + 1);

        auto idx = this->_get_location(key);
        auto& l = _entries[idx];
        for (auto& e : l)
            if (EqualT{}(e.key, key))
                return e.value;

        auto& val = l.emplace_front(KeyT(key), create()).value;
        ++_size; // AFTER create
        return val;
    }

    /// looks up the given key and returns the element
    /// UB if key is not present
    template <class T = KeyT>
    ValueT& get(T const& key)
    {
        CC_ASSERT(_size > 0 && "cannot get from an empty map");

        auto idx = this->_get_location(key);
        for (auto& e : _entries[idx])
            if (EqualT{}(e.key, key))
                return e.value;

        CC_UNREACHABLE("key not found");
    }
    template <class T = KeyT>
    ValueT const& get(T const& key) const
    {
        CC_ASSERT(_size > 0 && "cannot get from an empty map");

        auto idx = this->_get_location(key);
        for (auto& e : _entries[idx])
            if (EqualT{}(e.key, key))
                return e.value;

        CC_UNREACHABLE("key not found");
    }

    /// looks up the given key and (if found) returns a pointer to the value
    /// returns nullptr if not found
    template <class T = KeyT>
    ValueT* get_ptr(T const& key)
    {
        if (_size == 0)
            return nullptr;

        auto idx = this->_get_location(key);
        for (auto& e : _entries[idx])
            if (EqualT{}(e.key, key))
                return &e.value;

        return nullptr;
    }
    template <class T = KeyT>
    ValueT const* get_ptr(T const& key) const
    {
        if (_size == 0)
            return nullptr;

        auto idx = this->_get_location(key);
        for (auto& e : _entries[idx])
            if (EqualT{}(e.key, key))
                return &e.value;

        return nullptr;
    }

    /// looks up the given key and returns the element
    /// returns default_val if key not found
    template <class T = KeyT>
    ValueT const& get_or(T const& key, ValueT const& default_val) const
    {
        if (auto v = this->get_ptr(key))
            return *v;
        else
            return default_val;
    }

    /// looks up the given key and (if found) writes the value to 'out_val'
    /// returns true if key was found
    template <class T = KeyT>
    bool get_to(T const& key, ValueT& out_val) const
    {
        if (auto v = this->get_ptr(key))
        {
            out_val = *v;
            return true;
        }
        else
            return false;
    }

    /// removes a key from the map
    /// returns true iff something was removed
    /// supports heterogeneous lookup
    template <class U = KeyT>
    bool remove_key(U const& key)
    {
        if (_size == 0)
            return false;

        auto idx = this->_get_location(key);
        auto& list = _entries[idx];
        auto it = list.begin();
        auto end = list.end();

        if (!(it != end))
            return false;

        if (EqualT{}((*it).key, key))
        {
            list.pop_front();
            --_size;
            return true;
        }

        auto prev = it;
        while (it != end)
        {
            if (EqualT{}((*it).key, key))
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

    /// reserves internal resources to hold at least n elements without forcing a rehash
    void reserve(size_t n)
    {
        if (n <= _entries.size())
            return; // already enough

        size_t new_cap = 4;
        while (new_cap < n) // TODO: replace by bit magic, but without pulling in expensive headers
            new_cap <<= 1;

        _reserve(new_cap);
    }

    void clear()
    {
        _size = 0;
        for (auto& l : _entries)
            l.clear();
    }

    // operators
public:
    bool operator==(map const& rhs) const
    {
        if (_size != rhs._size)
            return false;

        for (auto const& kvp : *this)
        {
            // TODO: can be slightly improved by only computing the rhs entry once

            if (!rhs.contains_key(kvp.key))
                return false;

            if (rhs.get(kvp.key) != kvp.value)
                return false;
        }

        return true;
    }
    bool operator!=(map const& rhs) const { return !operator==(rhs); }

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
    /// NOTE: only works for non-empty maps
    template <class T>
    size_t _get_location(T const& key) const
    {
        CC_ASSERT(_entries.size() > 0);
        auto hash = HashT{}(key) ^ 0x0bd64917a71585aduLL;

        // a cheap but effective way to make hash distribution more uniform
        // this corresponds to a single round of xorshift32
        // see https://artificial-mind.net/blog/2021/10/09/unordered-map-badness
        hash = hash * 0xd989bcacc137dcd5uLL >> 32u;

        // capacity is always power-of-two
        return hash & (_entries.size() - 1);
    }

    /// resizes _entries to the given amount of buckets
    void _reserve(size_t new_cap)
    {
        CC_ASSERT((new_cap & (new_cap - 1)) == 0 && "capacity not power-of-two");

        auto old_entries = cc::move(_entries);
        _entries = cc::array<cc::forward_list<entry>>::defaulted(new_cap);
        for (auto& l : old_entries)
        {
            auto n = l._first;
            while (n)
            {
                auto nn = n->next;

                // add to new loc
                auto idx = this->_get_location(n->value.key);
                auto& nl = _entries[idx];
                n->next = nl._first;
                nl._first = n;

                n = nn;
            }

            l._first = nullptr; // all added to new lists, make sure they are not deleted
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

    friend double experimental::compute_hash_badness<KeyT, ValueT, HashT, EqualT>(map const& map);

    friend size_t detail::bucket_count<KeyT, ValueT, HashT, EqualT>(map const& map);
    friend size_t detail::bucket_idx<KeyT, ValueT, HashT, EqualT>(map const& map, KeyT const& key);
    friend size_t detail::bucket_size<KeyT, ValueT, HashT, EqualT>(map const& map, size_t bucket_idx);
};

template <class KeyT, class ValueT, class HashT, class EqualT>
double experimental::compute_hash_badness(map<KeyT, ValueT, HashT, EqualT> const& map)
{
    if (map.empty())
        return 0;

    auto const lambda = map.size() / double(map._entries.size());

    auto cost = 0.;
    for (auto const& bucket : map._entries)
    {
        auto const bucket_size = bucket.size();
        cost += bucket_size * bucket_size;
    }
    cost /= map.size();

    auto overhead = cost / (1 + lambda) - 1;
    return overhead < 0 ? 0 : overhead;
}

template <class KeyT, class ValueT, class HashT, class EqualT>
size_t detail::bucket_count(map<KeyT, ValueT, HashT, EqualT> const& map)
{
    return map._entries.size();
}
template <class KeyT, class ValueT, class HashT, class EqualT>
size_t detail::bucket_idx(map<KeyT, ValueT, HashT, EqualT> const& map, KeyT const& key)
{
    return map._get_location(key);
}
template <class KeyT, class ValueT, class HashT, class EqualT>
size_t detail::bucket_size(map<KeyT, ValueT, HashT, EqualT> const& map, size_t bucket_idx)
{
    return map._entries[bucket_idx].size();
}
}
