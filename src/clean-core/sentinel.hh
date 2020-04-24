#pragma once

namespace cc
{
/// a generic end-of-range sentinel
/// use:
///
///   struct my_struct
///   {
///       ...
///       auto begin() { return ...; }
///       cc::sentinel end() const { return {}; }
///   };
///
///   struct my_iterator
///   {
///       bool operator!=(cc::sentinel) const { return is_still_valid(); }
///   };
///
struct sentinel
{
};
}
