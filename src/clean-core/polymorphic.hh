#pragma once

namespace cc
{
/// can be used as base class for polymorphic classes
/// (helper that creates virtual dtor and deletes copy/move)
struct polymorphic
{
    polymorphic() = default;

    polymorphic(polymorphic const&) = delete;
    polymorphic(polymorphic&&) = delete;
    polymorphic& operator=(polymorphic const&) = delete;
    polymorphic& operator=(polymorphic&&) = delete;

    virtual ~polymorphic() = default;
};
}
