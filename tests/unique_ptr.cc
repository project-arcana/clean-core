#include <nexus/test.hh>

#include <clean-core/unique_ptr.hh>

TEST("cc::unique_ptr")
{
    auto a = cc::make_unique<int>(7);
    auto b = cc::make_unique<int>(7);
    auto pa = a.get();
    cc::unique_ptr<int> c;

    CHECK(a != b);
    CHECK(a != nullptr);
    CHECK(nullptr == c);
    CHECK(pa == a);
    CHECK(b != pa);
}
