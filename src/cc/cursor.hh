#pragma once

#include <utility>

#include <cc/assert.hh>

namespace cc
{
struct end_cursor
{
};

/**
 * cursor-based container facade:
 *   struct my_cursor : cc::cursor<my_cursor>
 *   {
 *      T get() const { return ...; }
 *      void advance() { ... }
 *      bool is_valid() const { return ...; }
 *   };
 *
 * guarantees:
 *  - advance() is never called if is_valid is false
 *  - get() is never called if is_valid is false
 */
template <class this_t>
struct cursor
{
    [[nodiscard]] decltype(auto) operator*() const { return static_cast<this_t const*>(this)->get(); }
    void operator++() { static_cast<this_t*>(this)->advance(); }
    void operator++(int) { static_cast<this_t*>(this)->advance(); }
    explicit operator bool() const { return static_cast<this_t const*>(this)->is_valid(); }

    [[nodiscard]] bool operator==(end_cursor) const { return !static_cast<this_t const*>(this)->is_valid(); }
    [[nodiscard]] bool operator!=(end_cursor) const { return static_cast<this_t const*>(this)->is_valid(); }

    // TODO: can we prevent the copy?
    [[nodiscard]] this_t begin() const { return *static_cast<this_t const*>(this); }
    [[nodiscard]] end_cursor end() const { return {}; }
};

// adaptor for legacy begin/end iterators
template <class iterator_t, class sentinel_t>
struct iterator_cursor : cursor<iterator_cursor<iterator_t, sentinel_t>>
{
    iterator_cursor(iterator_t begin, sentinel_t end) : _curr(std::move(begin)), _end(std::move(end)) {}

    [[nodiscard]] constexpr decltype(auto) get() const
    {
        CC_CONTRACT(_curr != nullptr);
        CC_CONTRACT(is_valid());
        return *_curr;
    }
    constexpr void advance()
    {
        CC_CONTRACT(is_valid());
        _curr++;
    }
    [[nodiscard]] constexpr bool is_valid() const { return _curr != _end; }

private:
    iterator_t _curr;
    sentinel_t _end;
};

// NOTE: this should always be fully qualified (cc::to_cursor)
//       otherwise it might get ADL-jacked
template <class Range>
[[nodiscard]] constexpr auto to_cursor(Range& range)
{
    // TODO: some partial specialization point for custom to_cursor
    return cc::iterator_cursor(range.begin(), range.end());
}
template <class Range>
void to_cursor(Range&& range) = delete; // this is a lifetime hazard
}
