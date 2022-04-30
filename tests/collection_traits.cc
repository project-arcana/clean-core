#include <nexus/test.hh>

#include <clean-core/array.hh>
#include <clean-core/capped_array.hh>
#include <clean-core/capped_vector.hh>
#include <clean-core/collection_traits.hh>
#include <clean-core/forward_list.hh>
#include <clean-core/map.hh>
#include <clean-core/set.hh>
#include <clean-core/span.hh>
#include <clean-core/strided_span.hh>
#include <clean-core/string.hh>
#include <clean-core/string_view.hh>
#include <clean-core/unique_ptr.hh>
#include <clean-core/vector.hh>

TEST("collection traits")
{
    {
        cc::vector<int> v;
        using traits = cc::collection_traits<decltype(v)>;

        static_assert(traits::is_range);
        static_assert(traits::is_contiguous);
        static_assert(!traits::is_fixed_size);
        static_assert(traits::can_add);
        static_assert(std::is_same_v<traits::element_t, int>);

        cc::collection_add(v, 7);
        CHECK(cc::collection_size(v) == 1);
    }

    {
        cc::capped_vector<int, 10> v;
        using traits = cc::collection_traits<decltype(v)>;

        static_assert(std::is_same_v<traits::element_t, int>);
        static_assert(traits::is_range);
        static_assert(traits::is_contiguous);
        static_assert(!traits::is_fixed_size);
        static_assert(traits::can_add);

        cc::collection_add(v, 7);
        CHECK(cc::collection_size(v) == 1);
    }

    {
        cc::span<int> v;
        using traits = cc::collection_traits<decltype(v)>;

        static_assert(std::is_same_v<traits::element_t, int>);
        static_assert(traits::is_range);
        static_assert(traits::is_contiguous);
        static_assert(!traits::is_fixed_size);
        static_assert(!traits::can_add);

        CHECK(cc::collection_size(v) == 0);
    }

    {
        cc::strided_span<int> v;
        using traits = cc::collection_traits<decltype(v)>;

        static_assert(std::is_same_v<traits::element_t, int>);
        static_assert(traits::is_range);
        static_assert(!traits::is_contiguous);
        static_assert(!traits::is_fixed_size);
        static_assert(!traits::can_add);

        CHECK(cc::collection_size(v) == 0);
    }

    {
        cc::array<int> v;
        using traits = cc::collection_traits<decltype(v)>;

        static_assert(std::is_same_v<traits::element_t, int>);
        static_assert(traits::is_range);
        static_assert(traits::is_contiguous);
        static_assert(!traits::is_fixed_size);
        static_assert(!traits::can_add);

        CHECK(cc::collection_size(v) == 0);
        v = {1, 2};
        CHECK(cc::collection_size(v) == 2);
    }

    {
        cc::array<int, 10> v;
        using traits = cc::collection_traits<decltype(v)>;

        static_assert(std::is_same_v<traits::element_t, int>);
        static_assert(traits::is_range);
        static_assert(traits::is_contiguous);
        static_assert(traits::is_fixed_size);
        static_assert(!traits::can_add);

        CHECK(cc::collection_size(v) == 10);
    }

    {
        cc::capped_array<int, 10> v;
        using traits = cc::collection_traits<decltype(v)>;

        static_assert(std::is_same_v<traits::element_t, int>);
        static_assert(traits::is_range);
        static_assert(traits::is_contiguous);
        static_assert(!traits::is_fixed_size);
        static_assert(!traits::can_add);

        CHECK(cc::collection_size(v) == 0);
        v = {1, 2};
        CHECK(cc::collection_size(v) == 2);
    }

    {
        int v[10];
        using traits = cc::collection_traits<decltype(v)>;

        static_assert(std::is_same_v<traits::element_t, int>);
        static_assert(traits::is_range);
        static_assert(traits::is_contiguous);
        static_assert(traits::is_fixed_size);
        static_assert(!traits::can_add);

        CHECK(cc::collection_size(v) == 10);
    }

    {
        cc::set<int> v;
        using traits = cc::collection_traits<decltype(v)>;

        static_assert(std::is_same_v<traits::element_t, int const>);
        static_assert(traits::is_range);
        static_assert(!traits::is_contiguous);
        static_assert(!traits::is_fixed_size);
        static_assert(traits::can_add);

        cc::collection_add(v, 7);
        CHECK(cc::collection_size(v) == 1);
    }

    {
        cc::map<int, float> v;
        using traits = cc::collection_traits<decltype(v)>;

        static_assert(traits::is_range);
        static_assert(!traits::is_contiguous);
        static_assert(!traits::is_fixed_size);
        static_assert(!traits::can_add);

        // TODO: map_traits

        v[1] = 3;
        CHECK(cc::collection_size(v) == 1);
    }

    {
        cc::forward_list<int> v;
        using traits = cc::collection_traits<decltype(v)>;

        static_assert(std::is_same_v<traits::element_t, int>);
        static_assert(traits::is_range);
        static_assert(!traits::is_contiguous);
        static_assert(!traits::is_fixed_size);
        static_assert(!traits::can_add); // cannot add at end

        CHECK(cc::collection_size(v) == 0);
    }

    {
        cc::string s;
        using traits = cc::collection_traits<decltype(s)>;

        static_assert(std::is_same_v<traits::element_t, char>);
        static_assert(traits::is_range);
        static_assert(traits::is_contiguous);
        static_assert(!traits::is_fixed_size);
        static_assert(traits::can_add);

        cc::collection_add(s, 'c');
        CHECK(cc::collection_size(s) == 1);
    }

    {
        cc::string_view s = "abc";
        using traits = cc::collection_traits<decltype(s)>;

        static_assert(std::is_same_v<traits::element_t, char const>);
        static_assert(traits::is_range);
        static_assert(traits::is_contiguous);
        static_assert(!traits::is_fixed_size);
        static_assert(!traits::can_add);

        CHECK(cc::collection_size(s) == 3);
    }

    {
        cc::vector<cc::unique_ptr<int>> v;
        using traits = cc::collection_traits<decltype(v)>;

        static_assert(traits::is_range);
        static_assert(traits::is_contiguous);
        static_assert(!traits::is_fixed_size);
        static_assert(traits::can_add);
        static_assert(std::is_same_v<traits::element_t, cc::unique_ptr<int>>);

        cc::collection_add(v, cc::make_unique<int>(7));
        CHECK(cc::collection_size(v) == 1);
        CHECK(*v[0] == 7);
    }
}
