#pragma once

#include <clean-core/allocate.hh>
#include <clean-core/forward.hh>
#include <clean-core/move.hh>

namespace cc
{
namespace detail
{
struct forward_list_sentinel
{
};
}

template <class T>
struct forward_list
{
private:
    struct node;

    // container
public:
    size_t size() const { return _size; }
    bool empty() const { return _size == 0; }
    T& front()
    {
        CC_CONTRACT(!empty());
        return _first->value;
    }
    T const& front() const
    {
        CC_CONTRACT(!empty());
        return _first->value;
    }

    // methods
public:
    template <class... Args>
    T& emplace_front(Args&&... args)
    {
        auto n = cc::alloc<node>(cc::forward<Args>(args)...);
        n->next = _first;
        _first = n;
        _size++;
        return n->value;
    }
    void push_front(T const& v) { emplace_front(v); }
    void push_front(T&& v) { emplace_front(cc::move(v)); }

    void clear()
    {
        if (_first)
            cc::free(_first);
        _first = nullptr;
        _size = 0;
    }

    // iteration
public:
    struct iterator
    {
        void operator++() { n = n->next; }
        T& operator*() { return n->value; }
        bool operator!=(detail::forward_list_sentinel) { return n; }

    private:
        iterator(node* n) : n(n) {}
        node* n;

        friend forward_list;
    };
    struct const_iterator
    {
        void operator++() { n = n->next; }
        T const& operator*() { return n->value; }
        bool operator!=(detail::forward_list_sentinel) { return n; }

    private:
        const_iterator(node const* n) : n(n) {}
        node const* n;

        friend forward_list;
    };

    iterator begin() { return {_first}; }
    const_iterator begin() const { return {_first}; }
    detail::forward_list_sentinel end() const { return {}; }

    // ctors
public:
    forward_list() = default;
    forward_list(forward_list const& rhs)
    {
        _size = rhs._size;
        auto rn = rhs._first;
        node* pn = nullptr;
        for (size_t i = 0; i < _size; ++i)
        {
            auto n = cc::alloc<node>(rn->value);
            (pn ? pn->next : _first) = n;
            rn = rn->next;
        }
    }
    forward_list(forward_list&& rhs) noexcept
    {
        _first = rhs._first;
        _size = rhs._size;
        rhs._first = nullptr;
    }
    forward_list& operator=(forward_list const& rhs)
    {
        if (_first)
        {
            cc::free(_first);
            _first = nullptr;
        }

        _size = rhs._size;
        auto rn = rhs._first;
        node* pn = nullptr;
        for (size_t i = 0; i < _size; ++i)
        {
            auto n = cc::alloc<node>(rn->value);
            (pn ? pn->next : _first) = n;
            rn = rn->next;
        }
    }
    forward_list& operator=(forward_list&& rhs) noexcept
    {
        if (_first)
            cc::free(_first);
        _first = rhs._first;
        _size = rhs._size;
        rhs._first = nullptr;
    }

    ~forward_list()
    {
        if (_first)
            cc::free(_first);
    }

    // internal types
private:
    struct node
    {
        T value;
        node* next = nullptr;

        template <class... Args>
        node(Args&&... args) : value(cc::forward<Args>(args)...)
        {
        }

        node(node const&) = delete;
        node(node&&) = delete;
        node& operator=(node const&) = delete;
        node& operator=(node&&) = delete;

        ~node()
        {
            if (next)
                cc::free(next);
        }
    };

private:
    node* _first = nullptr;
    size_t _size = 0;
};
}
