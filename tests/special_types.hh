#pragma once

struct regular_type
{
};

struct move_only_type
{
    int value;
    move_only_type() = default;
    explicit move_only_type(int i) : value(i) {}

    move_only_type(move_only_type const&) = delete;
    move_only_type(move_only_type&&) = default;
    move_only_type& operator=(move_only_type const&) = delete;
    move_only_type& operator=(move_only_type&&) = default;

    bool operator==(move_only_type const& rhs) const { return value == rhs.value; }
    bool operator!=(move_only_type const& rhs) const { return value != rhs.value; }
};

struct no_default_type
{
    no_default_type() = delete;
    explicit no_default_type(int i) : value(i) {}

    int value;

    bool operator==(no_default_type const& rhs) const { return value == rhs.value; }
    bool operator!=(no_default_type const& rhs) const { return value != rhs.value; }
};
