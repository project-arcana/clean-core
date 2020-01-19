#pragma once

/// Define a scoped enum that supports bitwise operations
/// Example:
///   CC_ENUM_FLAGS(my_flags, uint32_t)
///   {
///     none = 0,
///     foo = 1 << 0,
///     bar = 1 << 1,
///   };
#define CC_ENUM_FLAGS(_name_, _type_)                                                                                                                     \
enum class _name_ : _type_;                                                                                                                               \
inline constexpr _name_ operator ~ (_name_ t) noexcept { return static_cast<_name_>(~static_cast<_type_>(t)); }                                           \
inline constexpr _name_ operator | (_name_ lhs, _name_ rhs) noexcept { return static_cast<_name_>(static_cast<_type_>(lhs) | static_cast<_type_>(rhs)); } \
inline constexpr _name_ operator ^ (_name_ lhs, _name_ rhs) noexcept { return static_cast<_name_>(static_cast<_type_>(lhs) ^ static_cast<_type_>(rhs)); } \
inline constexpr _name_ operator & (_name_ lhs, _name_ rhs) noexcept { return static_cast<_name_>(static_cast<_type_>(lhs) & static_cast<_type_>(rhs)); } \
enum class _name_ : _type_
