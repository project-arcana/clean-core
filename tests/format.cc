#include <nexus/test.hh>

#include <clean-core/format.hh>

TEST("cc::format basics")
{
    CHECK(cc::format("{}", 17) == "17");
    CHECK(cc::format("{} + {} = {}", 1, 2, 3) == "1 + 2 = 3");
    CHECK(cc::format("{} + {} = {", 1, 2, 3) == "1 + 2 = 3");
}
