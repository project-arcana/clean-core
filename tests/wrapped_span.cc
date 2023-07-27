#include <nexus/test.hh>

#include <clean-core/experimental/wrapped_span.hh>
#include <clean-core/vector.hh>

TEST("cc::wrapped_span")
{
    auto vec = cc::vector{1, 2, 3, 4, 5};
    auto ws = cc::wrapped_span(vec);
    CHECK(ws[0] == 1);
    CHECK(ws[-1] == 5);
    CHECK(ws[-2] == 4);
    CHECK(ws[5] == 1);
    CHECK(ws[-6] == 5);
}
