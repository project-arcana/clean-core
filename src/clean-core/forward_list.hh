#pragma once

#include <clean-core/allocate.hh>
#include <clean-core/forward.hh>
#include <clean-core/move.hh>
#include <clean-core/sentinel.hh>

namespace cc
{
template <class T>
struct forward_list
{
    struct iterator;
    struct const_iterator;

private:
    struct node;

    // container
public:
    size_t size() const
    {
        size_t s = 0;
        auto p = _first;
        while (p != nullptr)
        {
            ++s;
            p = p->next;
        }
        return s;
    }
    bool empty() const { return _first == nullptr; }
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
        return n->value;
    }
    void push_front(T const& v) { emplace_front(v); }
    void push_front(T&& v) { emplace_front(cc::move(v)); }

    T pop_front()
    {
        CC_CONTRACT(!empty());
        T v = cc::move(_first->value);
        auto n = _first;
        _first = _first->next;
        cc::free(n);
        return v;
    }

    iterator erase_after(const_iterator it)
    {
        CC_CONTRACT(it.n->next); // no element after exists

        // n = current node
        // next = next node (that is to be erased)
        node* n = const_cast<node*>(it.n); // this is OK because erase_after is not const
        auto to_erase = n->next;
        n->next = to_erase->next;

        cc::free(to_erase);

        return n->next;
    }

    void clear()
    {
        auto p = _first;
        while (p)
        {
            auto n = p->next;
            cc::free(p);
            p = n;
        }
        _first = nullptr;
    }

    // iteration
public:
    struct iterator
    {
        void operator++() { n = n->next; }
        T& operator*() { return n->value; }
        bool operator!=(sentinel) { return n; }

        iterator() = default;
    private:
        iterator(node* n) : n(n) {}
        node* n = nullptr;

        friend forward_list;
    };
    struct const_iterator
    {
        void operator++() { n = n->next; }
        T const& operator*() { return n->value; }
        bool operator!=(sentinel) { return n; }

        const_iterator(iterator it) : n(it.n) {}

        const_iterator() = default;
    private:
        const_iterator(node const* n) : n(n) {}
        node const* n = nullptr;

        friend forward_list;
    };

    iterator begin() { return {_first}; }
    const_iterator begin() const { return {_first}; }
    sentinel end() const { return {}; }

    // ctors
public:
    forward_list() = default;
    forward_list(forward_list const& rhs)
    {
        auto rn = rhs._first;
        node* pn = nullptr;
        while (rn != nullptr)
        {
            auto n = cc::alloc<node>(rn->value);
            (pn ? pn->next : _first) = n;
            pn = n;
            rn = rn->next;
        }
    }
    forward_list(forward_list&& rhs) noexcept
    {
        _first = rhs._first;
        rhs._first = nullptr;
    }
    forward_list& operator=(forward_list const& rhs)
    {
        clear();

        auto rn = rhs._first;
        node* pn = nullptr;
        while (rn != nullptr)
        {
            auto n = cc::alloc<node>(rn->value);
            (pn ? pn->next : _first) = n;
            pn = n;
            rn = rn->next;
        }
    }
    forward_list& operator=(forward_list&& rhs) noexcept
    {
        clear();

        _first = rhs._first;
        rhs._first = nullptr;
    }

    ~forward_list()
    {
        clear();
    }

    // internal types
private:
    // NOTE: node is NON-OWNING!
    //       cc::free(node) does NOT free next
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
    };

private:
    node* _first = nullptr;
};
}
