#include <nexus/test.hh>

#include <clean-core/variant.hh>

TEST("cc::variant basics")
{
    cc::variant<int, cc::string, char> v;
    CC_ASSERT(v == 0);
    // CC_ASSERT(v != '\0'); -- TODO: what behavior is desired here?
    CC_ASSERT(v.is<int>());
    CC_ASSERT(!v.is<char>());
    CC_ASSERT(v.get<int>() == 0);

    v = 'c';
    CC_ASSERT(v == 'c');
    // CC_ASSERT(v != int('c')); -- TODO: what behavior is desired here?
    CC_ASSERT(v.is<char>());
    CC_ASSERT(!v.is<int>());
    CC_ASSERT(v.get<char>() == 'c');

    v = cc::string("hello");
    CHECK(v == "hello");
}
