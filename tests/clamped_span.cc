#include <nexus/test.hh>

#include <clean-core/experimental/clamped_span.hh>
#include <clean-core/vector.hh>

TEST("cc::wrapped_span")
{
    auto vec = cc::vector{1, 2, 3, 4, 5};
    auto ds = cc::clamped_span(vec);
    CHECK(ds[0] == 1);
    CHECK(ds[-1] == 1);
    CHECK(ds[-2] == 1);
    CHECK(ds[5] == 5);
    CHECK(ds[6] == 5);
}
