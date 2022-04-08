#include <nexus/test.hh>

#include <rich-log/log.hh>

#include <clean-core/alloc_array.hh>
#include <clean-core/alloc_vector.hh>
#include <clean-core/array.hh>
#include <clean-core/capped_vector.hh>
#include <clean-core/flags.hh>
#include <clean-core/forward_list.hh>
#include <clean-core/map.hh>
#include <clean-core/optional.hh>
#include <clean-core/pair.hh>
#include <clean-core/poly_unique_ptr.hh>
#include <clean-core/set.hh>
#include <clean-core/span.hh>
#include <clean-core/tuple.hh>
#include <clean-core/unique_ptr.hh>
#include <clean-core/vector.hh>

TEST("cc pretty printer")
{
    //
    // container
    //
    auto empty_vector = cc::vector<int>{};
    auto vector = cc::vector<int>{1, 2, 3, 4};
    auto array = cc::array<int>{1, 2, 3, 4};
    auto array4 = cc::array<int, 4>{1, 2, 3, 4};
    auto span = cc::span(vector);
    auto null_span = cc::span<int>(nullptr, size_t(0));
    auto alloc_vector = cc::alloc_vector<int>{1, 2, 3, 4};
    auto alloc_array = cc::alloc_array<int>{1, 2, 3, 4};
    auto capped_vector = cc::capped_vector<int, 5>{1, 2, 3, 4};
    auto empty_capped_vector = cc::capped_vector<int, 0>{};

    auto fwd_list = cc::forward_list<int>();
    fwd_list.push_front(1);
    fwd_list.push_front(2);
    fwd_list.push_front(3);
    auto fwd_list_vec = cc::forward_list<cc::vector<int>>();
    fwd_list_vec.push_front({1, 2, 3});
    fwd_list_vec.push_front({4, 5});
    fwd_list_vec.push_front({6, 7, 8, 9});
    auto empty_fwd_list = cc::forward_list<int>();

    auto pair = cc::pair<int, float>(1, 2.5f);
    auto tuple = cc::tuple{1, 2.5f, 'c', cc::vector<int>{1, 2, 3}};
    auto empty_tuple = cc::tuple{};
    auto pair_encoding = cc::pair<cc::string, char>("hello \" world", '"');

    auto empty_optional = cc::optional<int>();
    auto optional_int = cc::optional<int>(10);
    auto optional_vec = cc::optional<cc::vector<int>>(cc::vector<int>{1, 2, 3});

    auto set = cc::set<int>{1, 3, 5, 7, 9};
    auto set_vec = cc::set<cc::vector<int>>{{1, 2}, {}, {3, 4}, {5}};
    auto empty_set = cc::set<int>{};

    auto map = cc::map<int, float>{{3, 3.33f}, {5, 5.55f}, {9, 9.99f}};
    auto map_compl = cc::map<cc::string, cc::vector<int>>();
    map_compl["hello"] = {1, 2, 3, 4, 5};
    map_compl["nope"] = {};
    map_compl["world"] = {6, 7, 8};
    auto map_compl2 = cc::map<cc::vector<int>, cc::vector<int>>();
    map_compl2[{1, 2}] = {3, 4};
    map_compl2[{3}] = {5, 6, 7};
    map_compl2[{4, 5, 6}] = {};
    auto empty_map = cc::map<int, int>{};

    //
    // strings
    //
    auto string = cc::string("hello world");
    auto string_non_sbo = cc::string("hello world and longer and longer and longer and longer and longer and longer");
    auto string_view = cc::string_view(string).subview(1, string.size() - 2);
    auto empty_string = cc::string("");
    auto empty_string_view = cc::string_view("");
    auto null_string_view = cc::string_view(nullptr, size_t(0));

    //
    // smart ptr
    //
    auto empty_unique_ptr = cc::unique_ptr<int>();
    auto unique_ptr_int = cc::make_unique<int>(17);
    auto unique_ptr_vec = cc::make_unique<cc::vector<int>>(cc::vector<int>{1, 2, 3});
    auto empty_poly_unique_ptr = cc::poly_unique_ptr<int>();
    auto poly_unique_ptr_int = cc::make_poly_unique<int>(17);
    auto poly_unique_ptr_vec = cc::make_poly_unique<cc::vector<int>>(cc::vector<int>{1, 2, 3});

    //
    // flags
    //
    enum class some_enum
    {
        val_a,
        val_b,
        val_c,
    };
    auto enu = some_enum::val_b;
    auto enu_flags = cc::make_flags(some_enum::val_a, some_enum::val_c);
    auto enu_flags_none = enu_flags & cc::make_flags(some_enum::val_b);

    CHECK(true); // not a real test
}
