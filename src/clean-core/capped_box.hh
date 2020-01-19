#pragma once

#include <cstring>
#include <type_traits>

#include <clean-core/enable_if.hh>
#include <clean-core/forward.hh>
#include <clean-core/function_ptr.hh>
#include <clean-core/move.hh>
#include <clean-core/new.hh>
#include <clean-core/typedefs.hh>

namespace cc
{
namespace detail
{
template <size_t Size>
struct capped_box_storage
{
    std::byte _data[Size];
};

template <bool HasDtor>
struct capped_box_dtor
{
    static constexpr bool has_dtor = false;
};
template <>
struct capped_box_dtor<true>
{
    static constexpr bool has_dtor = true;
    cc::function_ptr<void(void*)> _dtor = nullptr;
};

template <bool HasDtor>
struct capped_box_move
{
    static constexpr bool has_move = false;
};
template <>
struct capped_box_move<true>
{
    static constexpr bool has_move = true;
    cc::function_ptr<void(void*, void*)> _move = nullptr;
};
}

/// a polymorphic move-only value type (allocated on the stack with a maximum size)
/// (basically a non-nullable poly_unique_ptr but on the stack)
/// TODO: are there alignment issues?
template <class T, size_t MaxSize = sizeof(T)>
struct capped_box : private detail::capped_box_storage<MaxSize>, //
                    private detail::capped_box_dtor<!std::is_trivially_destructible_v<T>>,
                    private detail::capped_box_move<!std::is_trivially_move_constructible_v<T>>
{
    template <class U, cc::enable_if<std::is_base_of_v<T, U>> = true>
    capped_box(U&& v)
    {
        static_assert(sizeof(U) <= MaxSize, "type too big to store");

        new (placement_new, this->_data) U(cc::forward<U>(v));
        _init_type<U>();
    }

    template <class U, cc::enable_if<std::is_base_of_v<T, U>> = true>
    capped_box& operator=(U&& v)
    {
        if constexpr (capped_box::has_dtor)
            if (this->_dtor)
                this->_dtor(this->_data);

        static_assert(sizeof(U) <= MaxSize, "type too big to store");

        new (placement_new, this->_data) U(cc::forward<U>(v));
        _init_type<U>();

        return *this;
    }

    capped_box(capped_box&& b) noexcept
    {
        if constexpr (capped_box::has_move)
        {
            if (b._move)
                b._move(this->_data, b._data);

            this->_move = b._move;
            b._move = nullptr;
        }
        else
            std::memcpy(this->_data, b._data, MaxSize);

        if constexpr (capped_box::has_dtor)
        {
            this->_dtor = b._dtor;
            b._dtor = nullptr;
        }
    }
    capped_box& operator=(capped_box&& b) noexcept
    {
        if constexpr (capped_box::has_dtor)
        {
            if (this->_dtor)
                this->_dtor(this->_data);
            this->_dtor = b._dtor;
        }

        if constexpr (capped_box::has_move)
        {
            if (b._move)
                b._move(this->_data, b._data);

            this->_move = b._move;
            b._move = nullptr;
        }
        else
            std::memcpy(this->_data, b._data, MaxSize);

        return *this;
    }

    template <class U, cc::enable_if<std::is_base_of_v<T, U>> = true>
    capped_box(capped_box<U>&& b) noexcept
    {
        if constexpr (capped_box::has_move)
        {
            if (b._move)
                b._move(this->_data, b._data);

            this->_move = b._move;
            b._move = nullptr;
        }
        else
            std::memcpy(this->_data, b._data, MaxSize);

        if constexpr (capped_box::has_dtor)
        {
            this->_dtor = b._dtor;
            b._dtor = nullptr;
        }
    }
    template <class U, cc::enable_if<std::is_base_of_v<T, U>> = true>
    capped_box& operator=(capped_box<U>&& b) noexcept
    {
        if constexpr (capped_box::has_dtor)
        {
            if (this->_dtor)
                this->_dtor(this->_data);
            this->_dtor = b._dtor;
        }

        if constexpr (capped_box::has_move)
        {
            if (b._move)
                b._move(this->_data, b._data);

            this->_move = b._move;
            b._move = nullptr;
        }
        else
            std::memcpy(this->_data, b._data, MaxSize);

        return *this;
    }

    template <class U, class... Args>
    U& emplace(Args&&... args)
    {
        static_assert(std::is_base_of_v<T, U>, "classes not compatible");
        static_assert(sizeof(U) <= MaxSize, "type too big to store");

        if (capped_box::has_dtor)
            if (this->_dtor)
                this->_dtor(this->_data);

        auto p = new (placement_new, this->_data) U(cc::forward<Args>(args)...);
        _init_type<U>();

        return *p;
    }

    capped_box(capped_box const&) = delete;
    capped_box& operator=(capped_box const&) = delete;

    ~capped_box()
    {
        if constexpr (capped_box::has_dtor)
            if (this->_dtor)
                this->_dtor(this->_data);
    }

    template <class U, size_t MaxSize2, class, class... Args>
    friend capped_box<U, MaxSize2> make_capped_box(Args&&... args);

    template <class, size_t>
    friend struct capped_box;

    [[nodiscard]] T* get() { return &reinterpret_cast<T const&>(this->_data); }
    [[nodiscard]] T const* get() const { return &reinterpret_cast<T const&>(this->_data); }

    T* operator->() { return &reinterpret_cast<T&>(this->_data); }
    T const* operator->() const { return &reinterpret_cast<T const&>(this->_data); }
    T& operator*() { return reinterpret_cast<T&>(this->_data); }
    T const& operator*() const { return reinterpret_cast<T const&>(this->_data); }

    operator T const&() const { return reinterpret_cast<T const&>(this->_data); }
    operator T&() { return reinterpret_cast<T&>(this->_data); }

private:
    capped_box() = default;

    template <class U>
    void _init_type()
    {
        if constexpr (capped_box::has_dtor)
            this->_dtor = [](void* data) { static_cast<U*>(data)->~U(); };
        else
            static_assert(std::is_trivially_destructible_v<U>, "if base is trivially destructible, derived also has to be");

        if constexpr (capped_box::has_move)
            this->_move = [](void* data, void* src) { new (placement_new, data) U(cc::move(*static_cast<U*>(src))); };
        else
            static_assert(std::is_trivially_move_constructible_v<U>, "if base is trivially move-constructible, derived also has to be");
    }
};

template <class T, size_t MaxSize = sizeof(T), class U = T, class... Args>
capped_box<T, MaxSize> make_capped_box(Args&&... args)
{
    capped_box<T, MaxSize> b;
    b.template emplace<U>(cc::forward<Args>(args)...);
    return b;
}
}
