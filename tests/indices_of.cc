#include <nexus/test.hh>

#include <clean-core/array.hh>
#include <clean-core/indices_of.hh>
#include <clean-core/vector.hh>
#include <clean-core/vector_ex.hh>

TEST("cc::indices_of")
{
    {
        cc::vector<int> v = {10, 7, 3};
        CHECK(cc::indices_of(v) == nx::range<size_t>{0, 1, 2});
    }
    {
        cc::array<int> v = {10, 7, 3};
        CHECK(cc::indices_of(v) == nx::range<size_t>{0, 1, 2});
    }
    {
        cc::array<int, 3> v = {10, 7, 3};
        CHECK(cc::indices_of(v) == nx::range<size_t>{0, 1, 2});
    }
    {
        int v[] = {10, 7, 3};
        CHECK(cc::indices_of(v) == nx::range<size_t>{0, 1, 2});
    }

    // NOTE: currently not supported because indices_of is based on collection size type (index type is harder to infer)
    // {
    //     enum class custom_idx : int;
    //     struct vec_traits
    //     {
    //         using element_t = bool;
    //         using index_t = custom_idx;
    //     };
    //     cc::vector_ex<vec_traits> v = {true, false, true};
    //     CHECK(cc::indices_of(v) == nx::range<custom_idx>{custom_idx(0), custom_idx(1), custom_idx(2)});
    // }
}

TEST("cc::indices_of reversed")
{
    cc::vector<int> v = {10, 7, 3};
    CHECK(cc::indices_of(v).reversed() == nx::range<size_t>{2, 1, 0});
    CHECK(cc::indices_of(v).reversed().reversed() == nx::range<size_t>{0, 1, 2});
}

TEST("cc::indices_of empty")
{
    cc::vector<int> v;
    CHECK(cc::indices_of(v) == nx::range<size_t>{});
    CHECK(cc::indices_of(v).reversed() == nx::range<size_t>{});
    CHECK(cc::indices_of(v).reversed().reversed() == nx::range<size_t>{});
}
