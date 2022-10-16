#pragma once

#include <clean-core/macros.hh>
#include <clean-core/new.hh>
#include <clean-core/span.hh>
#include <clean-core/vector.hh>

namespace cc
{
/// a chunk based buffer of values
/// basically a more efficient vector<T> where only push_back and for-each is allowed
/// there is no exponential resize involved
/// push_back is always O(1)
/// the main use case is accumulating output data, then iterating over it, no random access involved
///
/// TODO: maybe this can be cc::deque
///       but without subscript access (maybe via explicit fun)
///
/// TODO: for now, this is non-moveable (because I'm lazy). eventually this is copyable
/// TODO: for now, only chunk iteration is allowed
/// TODO: make chunk size exponentially increasing for a while so that small buffers are more efficient?
///       -> maybe userdefined is strict better
template <class T>
struct chunked_buffer
{
    size_t size() const { return _full_size + (_curr.head - _curr.begin); }
    bool empty() const { return size() == 0; }

    // TODO: might be able to formulate it so that f is only called once
    // F: (span<T>) -> void
    template <class F>
    void for_each_chunk(F&& f)
    {
        for (auto const& c : _full_chunks)
            f(c.as_span());
        if (_curr.is_valid())
            f(_curr.as_span());
    }
    // F: (span<T const>) -> void
    template <class F>
    void for_each_chunk(F&& f) const
    {
        for (auto const& c : _full_chunks)
            f(c.as_const_span());
        if (_curr.is_valid())
            f(_curr.as_const_span());
    }

    template <class... Args>
    T& emplace_back(Args&&... args)
    {
        // need new chunk?
        if CC_CONDITION_UNLIKELY (!_curr.is_valid())
        {
            if (_curr.begin != nullptr)
            {
                _full_size += _curr.head - _curr.begin;
                _full_chunks.push_back(_curr);
            }

            _curr.begin = _alloc(_chunk_size);
            _curr.head = _curr.begin;
            _curr.end = _curr.begin + _chunk_size;
        }

        CC_ASSERT(_curr.is_valid());
        return *new (cc::placement_new, _curr.head++) T(cc::forward<Args>(args)...);
    }
    /// adds an element at the end
    T& push_back(T const& value)
    {
        static_assert(std::is_copy_constructible_v<T>, "only works with copyable types. did you forget a cc::move?");
        return this->emplace_back(value);
    }
    /// adds an element at the end
    T& push_back(T&& value) { return this->emplace_back(cc::move(value)); }

    // TODO: begin/end (really? is slow-ish)
    // TODO: .chunks()
    // TODO: .elements() -> if no begin/end
    // TODO: push/pop front/back all

    size_t chunk_size() const { return _chunk_size; }
    void set_chunk_size(size_t s)
    {
        CC_ASSERT(s > 0);
        _chunk_size = s;
    }

    chunked_buffer() = default;
    chunked_buffer(chunked_buffer const&) = delete;
    chunked_buffer& operator=(chunked_buffer const&) = delete;

    ~chunked_buffer()
    {
        if (_curr.begin)
            _free(_curr.begin);
        for (auto const& c : _full_chunks)
            _free(c.begin);
    }

private:
    // TODO: use a doubly linked list for deque with push/pop front/back

    static T* _alloc(size_t size) { return reinterpret_cast<T*>(new std::byte[size * sizeof(T)]); }
    static void _free(T* p) { delete[] reinterpret_cast<std::byte*>(p); }

    struct chunk
    {
        T* head = nullptr;
        T* end = nullptr;
        T* begin = nullptr;

        bool is_valid() const { return head != end; }
        cc::span<T> as_span() const { return {begin, head}; }
        cc::span<T const> as_const_span() const { return {begin, head}; }
    };

    chunk _curr;
    cc::vector<chunk> _full_chunks;
    size_t _full_size = 0;
    size_t _chunk_size = 1024; // elements
};
}
