#include <nexus/test.hh>

#include <clean-core/hash.hh>

TEST("cc::hash")
{
    struct foo
    {
        int f = 6;
        bool b = true;
    };
    struct trivial_foo
    {
        int a = 2;
        int c = 4;
    };

    static_assert(cc::can_hash<float>);
    static_assert(cc::can_hash<int>);
    static_assert(cc::can_hash<bool>);
    static_assert(cc::can_hash<trivial_foo>);
    static_assert(!cc::can_hash<foo>);

    CHECK(true); // silence warning
}
