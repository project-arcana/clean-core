#pragma once

#include <cstdint>

#include <clean-core/fwd.hh>

namespace cc
{
struct type_id_t
{
    bool operator==(type_id_t const& rhs) const { return id == rhs.id; }
    bool operator!=(type_id_t const& rhs) const { return id != rhs.id; }
    bool operator<(type_id_t const& rhs) const { return id < rhs.id; }
    bool operator<=(type_id_t const& rhs) const { return id <= rhs.id; }
    bool operator>(type_id_t const& rhs) const { return id > rhs.id; }
    bool operator>=(type_id_t const& rhs) const { return id >= rhs.id; }

    type_id_t() = default; // no id is NOT the same as cc::type_id<void>();

    explicit operator bool() const { return id != nullptr; }

private:
    void const* id = nullptr;
    type_id_t(void const* id) : id(id) {}

    template <typename T>
    friend type_id_t type_id();
};

namespace detail
{
template <class T>
struct type_id_impl
{
    static constexpr int id = 0;
};
}

// provides a unique ID for the type
// WARNING: Not consistent across DLLs!
template <class T>
type_id_t type_id()
{
    return &detail::type_id_impl<T>::id;
}

template <>
struct hash<type_id_t>
{
    [[nodiscard]] uint64_t operator()(type_id_t const& v) const noexcept
    {
        static_assert(sizeof(v) == sizeof(uint64_t));
        return reinterpret_cast<uint64_t const&>(v);
    }
};
}
