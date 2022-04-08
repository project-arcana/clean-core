#include <nexus/fuzz_test.hh>
#include <nexus/monte_carlo_test.hh>

#include <vector>

#include <clean-core/alloc_vector.hh>
#include <clean-core/allocators/linear_allocator.hh>
#include <clean-core/allocators/stack_allocator.hh>
#include <clean-core/capped_vector.hh>
#include <clean-core/functors.hh>
#include <clean-core/string.hh>
#include <clean-core/vector.hh>

#include <typed-geometry/feature/random.hh>
#include <typed-geometry/tg.hh>

#include "special_types.hh"

namespace
{
template <class T, class = void>
struct has_reserve_t : std::false_type
{
};
template <class T>
struct has_reserve_t<T, std::void_t<decltype(T::reserve)>> : std::true_type
{
};

template <class vector_t>
struct vector_tester
{
    using T = std::decay_t<decltype(std::declval<vector_t>()[0])>;

    static constexpr bool has_reserve = has_reserve_t<T>::value;
    static constexpr bool has_default_ctor = std::is_default_constructible_v<T>;
    static constexpr bool is_copyable = std::is_copy_assignable_v<T>;

    tg::rng rng;
    vector_t v;

    auto make_obj()
    {
        if constexpr (std::is_same_v<T, int>)
            return uniform(rng, -10, 10);
        else if constexpr (std::is_same_v<T, no_default_type>)
            return no_default_type(uniform(rng, -10, 10));
        else if constexpr (std::is_same_v<T, move_only_type>)
            return move_only_type(uniform(rng, -10, 10));
        else
            static_assert(cc::always_false<T>, "not implemented");
    }
    vector_t make_vec()
    {
        vector_t v;
        auto s = uniform(rng, 0, 4);
        for (auto i = 0; i < s; ++i)
            v.push_back(make_obj());
        return v;
    }

    void step()
    {
        switch (uniform(rng, 0, 13))
        {
        case 0:
            v.clear();
            break;
        case 1:
            if (!v.empty())
                v.pop_back();
            break;
        case 2:
            if (v.size() < 20)
                v.push_back(make_obj());
            break;
        case 3:
            if (v.size() < 20)
                v.emplace_back(make_obj());
            break;
        case 4:
            v = make_vec(); // move
            break;
        case 5:
            if constexpr (is_copyable)
            {
                auto v2 = make_vec();
                v = v2; // copy
            }
            break;
        case 6:
            if constexpr (has_default_ctor && is_copyable) // not defined for default ctor
                v.resize(size_t(uniform(rng, 0, 5)));
            break;
        case 7:
            if constexpr (is_copyable)
                v.resize(size_t(uniform(rng, 0, 5)), make_obj());
            break;
        case 8:
            if constexpr (has_reserve)
                v.reserve(size_t(uniform(rng, 0, 10)));
            break;
        case 9:
            if constexpr (has_reserve)
                v.shrink_to_fit();
            break;
        case 10:
            if constexpr (has_default_ctor) // not defined for default ctor
                if (v.size() < 20)
                    v.emplace_back() = make_obj();
            break;
        case 11:
            if constexpr (is_copyable)
                v = vector_t(make_vec()); // move ctor
            break;
        case 12:
            if constexpr (is_copyable)
            {
                auto v2 = make_vec();
                v = vector_t(v2); // copy ctor
            }
            break;
        case 13:
            if (!v.empty())
                v[uniform(rng, size_t(0), v.size() - 1)] = make_obj();
            break;
        default:
            break;
        }
    }

    template <class other_vector_t>
    void check_equal(other_vector_t const& rhs) const
    {
        auto const& v0 = v;
        auto const& v1 = rhs.v;

        CHECK(v0.size() == v1.size());
        CHECK(v0.empty() == v1.empty());
        for (auto i = 0u; i < v0.size(); ++i)
            CHECK(v0[i] == v1[i]);
        CHECK(v0 == v0);
        CHECK(v1 == v1);
    }
};
}

TEST("cc::vector basics")
{
    tg::rng rng;

    auto const test = [&](auto&& v0, auto&& v1)
    {
        //
        auto s = rng();
        v0.rng.seed(s);
        v1.rng.seed(s);

        for (auto i = 0; i < 100; ++i)
        {
            v1.check_equal(v0);

            v0.step();
            v1.step();

            v0.check_equal(v1);
        }
    };

    auto const type_test = [&](auto t)
    {
        using T = decltype(t);
        // test new vector and vector-like types
        for (auto i = 0; i < 10; ++i)
        {
            test(vector_tester<std::vector<T>>(), vector_tester<cc::vector<T>>());
            test(vector_tester<std::vector<T>>(), vector_tester<cc::capped_vector<T, 20>>());
        }
    };

    type_test(int());
    type_test(no_default_type(0));
    type_test(move_only_type(0));

    // TODO: count constructions!
}

FUZZ_TEST("cc::vector fuzz")(tg::rng& rng)
{
    auto cnt = uniform(rng, 1, 10);

    std::vector<int> v0;
    cc::vector<int> v1;

    for (auto i = 0; i < cnt; ++i)
    {
        auto v = uniform(rng, -10, 10);
        v0.push_back(v);
        v1.push_back(v);
    }

    CHECK(tg::sum(v0) == tg::sum(v1));
}

namespace
{
template <class T>
struct is_capped_vector : std::false_type
{
};
template <class T, size_t S>
struct is_capped_vector<cc::capped_vector<T, S>> : std::true_type
{
};

template <class T>
struct is_std_vector : std::false_type
{
};
template <class T>
struct is_std_vector<std::vector<T>> : std::true_type
{
};
}

MONTE_CARLO_TEST("cc::vector mct")
{
    auto const make_int = [](tg::rng& rng) { return uniform(rng, -10, 10); };
    auto const make_str = [](tg::rng& rng)
    {
        cc::string s;
        switch (uniform(rng, 0, 2))
        {
        case 0:
            // empty str
            break;

        case 1:
            // small string
            s.resize(uniform(rng, 0, 20));
            for (auto& c : s)
                c = uniform(rng, 'A', 'Z');
            break;

        case 2:
            // larger string
            s.resize(uniform(rng, 0, 100));
            for (auto& c : s)
                c = uniform(rng, 'A', 'Z');
            break;
        }

        return s;
    };

    auto const is_even_int = [](int i) { return i % 2 == 0; };
    auto const is_even_str = [](cc::string const& s) { return s.size() % 2 == 0; };

    addOp("gen int", make_int);
    addOp("gen str", make_str);

    auto constexpr max_size = 40;
    auto const addType = [&](auto obj, auto make_element, auto elem_str, auto elem_pred)
    {
        using vector_t = decltype(obj);
        using T = std::decay_t<decltype(obj[0])>;

        auto constexpr is_std_vec = is_std_vector<vector_t>::value;
        auto constexpr is_capped_vec = is_capped_vector<vector_t>::value;

        auto is_empty = [](vector_t const& s) { return s.empty(); };

        addOp("default ctor", [] { return vector_t(); });
        addOp("move ctor", [](vector_t const& s) { return cc::move(vector_t(s)); });
        addOp("move assignment", [](vector_t& a, vector_t const& b) { a = vector_t(b); });

        addOp("size ctor",
              [](tg::rng& rng)
              {
                  auto cnt = size_t(uniform(rng, 0, 30));
                  return vector_t(cnt);
              });

        if constexpr (is_std_vec)
        {
            addOp("ctor defaulted",
                  [](tg::rng& rng)
                  {
                      auto cnt = size_t(uniform(rng, 0, 30));
                      return vector_t(cnt);
                  });
            addOp("ctor uninitialized",
                  [](tg::rng& rng)
                  {
                      auto cnt = size_t(uniform(rng, 0, 30));
                      return vector_t(cnt);
                  });
            addOp("ctor filled",
                  [&](tg::rng& rng)
                  {
                      auto e = make_element(rng);
                      auto cnt = size_t(uniform(rng, 0, 30));
                      auto v = vector_t(cnt);
                      for (auto& ee : v)
                          ee = e;
                      return v;
                  });
        }
        else
        {
            addOp("ctor defaulted", [](tg::rng& rng) { return vector_t::defaulted(uniform(rng, 0, 30)); });
            addOp("ctor uninitialized",
                  [](tg::rng& rng)
                  {
                      auto v = vector_t::uninitialized(uniform(rng, 0, 30));
                      for (auto& e : v)
                          new (cc::placement_new, &e) T();
                      return v;
                  });
            addOp("ctor filled",
                  [&](tg::rng& rng)
                  {
                      auto e = make_element(rng);
                      return vector_t::filled(uniform(rng, 0, 30), e);
                  });
        }

        if constexpr (std::is_copy_constructible_v<T>)
        {
            addOp("copy ctor", [](vector_t const& s) { return vector_t(s); });
            addOp("copy assignment", [](vector_t& a, vector_t const& b) { a = b; });
        }

        addOp("randomize",
              [&](tg::rng& rng, vector_t& s)
              {
                  auto cnt = uniform(rng, 0, 30);
                  s.resize(cnt);
                  for (auto i = 0; i < cnt; ++i)
                      s[i] = make_element(rng);
                  return s;
              });

        if constexpr (is_capped_vec)
            addOp("reserve", [](tg::rng& rng, vector_t&) { (void)uniform(rng, 0, 30); });
        else
            addOp("reserve", [](tg::rng& rng, vector_t& s) { s.reserve(uniform(rng, 0, 30)); });

        addOp("resize", [](tg::rng& rng, vector_t& s) { s.resize(uniform(rng, 0, 30)); });
        addOp("resize + int", [](tg::rng& rng, vector_t& s, T const& c) { s.resize(uniform(rng, 0, 30), c); });

        addOp("random replace", [&](tg::rng& rng, vector_t& s) { random_choice(rng, s) = make_element(rng); })
            .when([](tg::rng&, vector_t const& s) { return s.size() > 0; });

        addOp("push_back",
              [](vector_t& s, T const& c)
              {
                  if constexpr (is_std_vec)
                  {
                      s.push_back(c);
                      return s.back();
                  }
                  else
                      return s.push_back(c);
              })
            .when([](vector_t& s) { return s.size() < max_size; });
        addOp("push_back move",
              [](vector_t& s, T c)
              {
                  if constexpr (is_std_vec)
                  {
                      s.push_back(cc::move(c));
                      return s.back();
                  }
                  else
                      return s.push_back(cc::move(c));
              })
            .when([](vector_t& s) { return s.size() < max_size; });
        addOp("emplace_back", [](vector_t& s, T const& c) { return s.emplace_back(c); }).when([](vector_t& s) { return s.size() < max_size; });
        addOp("emplace_back move", [](vector_t& s, T c) { return s.emplace_back(cc::move(c)); }).when([](vector_t& s) { return s.size() < max_size; });
        addOp("emplace_back_stable",
              [](vector_t& s, T const& c)
              {
                  if constexpr (is_std_vec || is_capped_vec)
                      s.push_back(c);
                  else if (s.at_capacity())
                      s.push_back(c);
                  else
                      s.emplace_back_stable(cc::move(c));
              })
            .when([](vector_t& s) { return s.size() < max_size; });
        addOp("push_back_range",
              [](vector_t& s, vector_t r)
              {
                  if constexpr (is_std_vec || is_capped_vec)
                  {
                      for (auto const& v : r)
                          s.push_back(v);
                  }
                  else
                      s.push_back_range(r);
              })
            .when([](vector_t& s, vector_t r) { return s.size() + r.size() < max_size; });

        if constexpr (!is_capped_vec)
        {
            addOp("insert_at",
                  [](tg::rng& rng, vector_t& s, T const& c)
                  {
                      auto i = uniform(rng, 0, int(s.size()));
                      if constexpr (is_std_vec)
                          s.insert(s.begin() + i, c);
                      else
                          s.insert_at(i, c);
                  })
                .when([](tg::rng&, vector_t& s) { return s.size() < max_size; })
                .make_optional();
            addOp("insert_range_at",
                  [](tg::rng& rng, vector_t& s, vector_t r)
                  {
                      auto i = uniform(rng, 0, int(s.size()));
                      if constexpr (is_std_vec)
                          s.insert(s.begin() + i, r.begin(), r.end());
                      else
                          s.insert_range_at(i, r);
                  })
                .when([](tg::rng&, vector_t& s, vector_t r) { return s.size() + r.size() < max_size; })
                .make_optional();
        }

        addOp("pop_back", [](vector_t& s) { s.pop_back(); }).when_not(is_empty);

        addOp("remove_all",
              [&](vector_t& s)
              {
                  if constexpr (is_std_vec || is_capped_vec)
                  {
                      vector_t n;
                      for (auto const& e : s)
                          if (!elem_pred(e))
                              n.push_back(e);
                      s = cc::move(n);
                  }
                  else
                      s.remove_all(elem_pred);
              });
        addOp("remove_first",
              [&](vector_t& s)
              {
                  if constexpr (is_std_vec || is_capped_vec)
                  {
                      vector_t n;
                      auto removed = false;
                      for (auto const& e : s)
                          if (removed || !elem_pred(e))
                              n.push_back(e);
                          else
                              removed = true;
                      s = cc::move(n);
                  }
                  else
                      s.remove_first(elem_pred);
              });
        addOp("remove_first_unordered",
              [&](vector_t& s)
              {
                  if constexpr (is_std_vec || is_capped_vec)
                  {
                      for (auto& e : s)
                          if (elem_pred(e))
                          {
                              cc::swap(e, s.back());
                              s.pop_back();
                              break;
                          }
                  }
                  else
                      s.remove_first_unordered(elem_pred);
              });
        addOp("remove_value",
              [&](vector_t& s, T const& e)
              {
                  if constexpr (is_std_vec || is_capped_vec)
                  {
                      vector_t n;
                      for (auto const& ee : s)
                          if (e != ee)
                              n.push_back(ee);
                      s = cc::move(n);
                  }
                  else
                      s.remove_value(e);
              });
        addOp("remove_at",
              [&](vector_t& s, int idx)
              {
                  if constexpr (is_std_vec || is_capped_vec)
                  {
                      vector_t n;
                      for (auto i = 0; i < int(s.size()); ++i)
                          if (i != idx)
                              n.push_back(s[i]);
                      s = cc::move(n);
                  }
                  else
                      s.remove_at(idx);
              })
            .when([](vector_t& s, int idx) { return 0 <= idx && idx < int(s.size()); });
        addOp("remove_at_unordered",
              [&](vector_t& s, int idx)
              {
                  if constexpr (is_std_vec || is_capped_vec)
                  {
                      cc::swap(s[idx], s.back());
                      s.pop_back();
                  }
                  else
                      s.remove_at_unordered(idx);
              })
            .when([](vector_t& s, int idx) { return 0 <= idx && idx < int(s.size()); });
        addOp("remove_range",
              [&](vector_t& s, int start, int count)
              {
                  if constexpr (is_std_vec || is_capped_vec)
                  {
                      vector_t n;
                      for (auto i = 0; i < int(s.size()); ++i)
                          if (i < start || i >= start + count)
                              n.push_back(s[i]);
                      s = cc::move(n);
                  }
                  else
                      s.remove_range(start, count);
              })
            .when([](vector_t& s, int start, int count) { return 0 <= start && 0 <= count && start + count <= int(s.size()); });

        addOp("contains",
              [&](vector_t const& s, T const& e)
              {
                  if constexpr (is_std_vec || is_capped_vec)
                  {
                      for (auto const& ee : s)
                          if (e == ee)
                              return true;
                      return false;
                  }
                  else
                      return s.contains(e);
              });

        addOp("op[]", [](tg::rng& rng, vector_t const& s) { return random_choice(rng, s); })
            .when([](tg::rng&, vector_t const& s) { return s.size() > 0; });
        addOp("data[]", [](tg::rng& rng, vector_t const& s) { return s.data()[uniform(rng, 0, int(s.size()) - 1)]; })
            .when([](tg::rng&, vector_t const& s) { return s.size() > 0; });

        addOp("fill",
              [](vector_t& s, T const& v)
              {
                  for (auto& c : s)
                      c = v;
              });


        if constexpr (is_capped_vec)
            addOp("shrink_to_fit", [](vector_t&) { /* no-op*/ });
        else
            addOp("shrink_to_fit", [](vector_t& s) { s.shrink_to_fit(); });
        addOp("clear", [](vector_t& s) { s.clear(); });

        addOp("size", [](vector_t const& s) { return s.size(); });
        addOp("size_bytes",
              [](vector_t const& s)
              {
                  if constexpr (is_std_vec)
                      return s.size() * sizeof(T);
                  else
                      return s.size_bytes();
              });
        addOp("front", [](vector_t const& s) { return s.front(); }).when_not(is_empty);
        addOp("back", [](vector_t const& s) { return s.back(); }).when_not(is_empty);

        setPrinter<vector_t>(
            [&](vector_t const& v)
            {
                cc::string s = "[";
                for (auto i = 0; i < int(v.size()); ++i)
                {
                    if (i > 0)
                        s += ", ";
                    s += elem_str(v[i]);
                }
                s += "]";
                return s;
            });
    };

    auto testType = [&](auto obj, auto make_element, auto elem_str, auto elem_pred)
    {
        using T = decltype(obj);

        addType(std::vector<T>(), make_element, elem_str, elem_pred);
        addType(cc::vector<T>(), make_element, elem_str, elem_pred);
        addType(cc::capped_vector<T, max_size>(), make_element, elem_str, elem_pred);

        testEquivalence(
            [](std::vector<T> const& a, cc::vector<T> const& b)
            {
                REQUIRE(a.size() == b.size());
                for (auto i = 0; i < int(a.size()); ++i)
                    REQUIRE(a[i] == b[i]);
            });
        testEquivalence(
            [](cc::vector<T> const& a, cc::capped_vector<T, max_size> const& b)
            {
                REQUIRE(a.size() == b.size());
                for (auto i = 0; i < int(a.size()); ++i)
                    REQUIRE(a[i] == b[i]);
            });
    };

    testType(
        int{}, make_int, [](int i) { return cc::to_string(i); }, is_even_int);
    testType(cc::string{}, make_str, cc::identity_function{}, is_even_str);
}

MONTE_CARLO_TEST("cc::alloc_vector mct")
{
    // almost the same as the MCT above, but missing copy ctors/assign ops

    auto const make_int = [](tg::rng& rng) { return uniform(rng, -10, 10); };

    addOp("gen int", make_int);

    auto const addType = [&](auto obj)
    {
        using vector_t = decltype(obj);
        using T = std::decay_t<decltype(obj[0])>;

        auto is_empty = [](vector_t const& s) { return s.empty(); };

        addOp("default ctor", [] { return vector_t(); });

        addOp("move ctor",
              [](vector_t const& s)
              {
                  if constexpr (std::is_copy_assignable_v<vector_t>)
                      return cc::move(vector_t(s));
                  else
                      return cc::move(vector_t(cc::span<T const>{s}));
              });
        addOp("move assignment",
              [](vector_t& a, vector_t const& b)
              {
                  if constexpr (std::is_copy_assignable_v<vector_t>)
                      a = vector_t(b);
                  else
                      a = vector_t(cc::span<T const>{b});
              });

        addOp("randomize",
              [&](tg::rng& rng, vector_t& s)
              {
                  auto cnt = uniform(rng, 0, 30);
                  s.resize(cnt);
                  for (auto i = 0; i < cnt; ++i)
                      s[i] = T(make_int(rng));
              });

        if constexpr (!is_capped_vector<vector_t>::value)
            addOp("reserve", [](tg::rng& rng, vector_t& s) { s.reserve(uniform(rng, 0, 30)); }).make_optional();
        addOp("resize", [](tg::rng& rng, vector_t& s) { s.resize(uniform(rng, 0, 30)); });
        addOp("resize + int", [](tg::rng& rng, vector_t& s, int c) { s.resize(uniform(rng, 0, 30), c); });

        addOp("random replace", [&](tg::rng& rng, vector_t& s) { random_choice(rng, s) = make_int(rng); })
            .when([](tg::rng&, vector_t const& s) { return s.size() > 0; });

        addOp("push_back", [](vector_t& s, int c) { s.push_back(c); });
        addOp("emplace_back", [](vector_t& s, int c) { return s.emplace_back(c); });

        addOp("pop_back", [](vector_t& s) { s.pop_back(); }).when_not(is_empty);

        addOp("op[]", [](tg::rng& rng, vector_t const& s) { return random_choice(rng, s); })
            .when([](tg::rng&, vector_t const& s) { return s.size() > 0; });
        addOp("data[]", [](tg::rng& rng, vector_t const& s) { return s.data()[uniform(rng, 0, int(s.size()) - 1)]; })
            .when([](tg::rng&, vector_t const& s) { return s.size() > 0; });

        addOp("fill",
              [](vector_t& s, int v)
              {
                  for (auto& c : s)
                      c = v;
              });


        if constexpr (!is_capped_vector<vector_t>::value)
            addOp("shrink_to_fit", [](vector_t& s) { s.shrink_to_fit(); }).make_optional();
        addOp("clear", [](vector_t& s) { s.clear(); });

        addOp("size", [](vector_t const& s) { return s.size(); });
        addOp("front", [](vector_t const& s) { return s.front(); }).when_not(is_empty);
        addOp("back", [](vector_t const& s) { return s.back(); }).when_not(is_empty);
    };

    auto testType = [&](auto obj)
    {
        using T = decltype(obj);

        addType(std::vector<T>());
        addType(cc::alloc_vector<T>());
        addType(cc::vector<T>());

        testEquivalence(
            [](std::vector<T> const& a, cc::alloc_vector<T> const& b)
            {
                REQUIRE(a.size() == b.size());
                for (auto i = 0; i < int(a.size()); ++i)
                    REQUIRE(a[i] == b[i]);
            });

        testEquivalence(
            [](cc::vector<T> const& a, cc::alloc_vector<T> const& b)
            {
                REQUIRE(a.size() == b.size());
                for (auto i = 0; i < int(a.size()); ++i)
                    REQUIRE(a[i] == b[i]);
            });
    };

    testType(int{});
}

TEST("cc::vector remove")
{
    cc::vector<int> v = {3, 2, 1, 3};
    v.remove(3);
    CHECK(v.size() == 2);
    CHECK(v == cc::vector{2, 1});
    v.remove(2);
    CHECK(v == cc::vector{1});
    CHECK(v.size() == 1);
    v.push_back(4);
    v.push_back(4);
    CHECK(v == cc::vector{1, 4, 4});
    CHECK(v.size() == 3);
    v.remove(2);
    CHECK(v == cc::vector{1, 4, 4});
    CHECK(v.size() == 3);
    v.remove(1);
    CHECK(v == cc::vector{4, 4});
    CHECK(v.size() == 2);
    v.remove(4);
    CHECK(v.empty());
    CHECK(v.size() == 0);

    v = {3, 2, 4};
    v.remove_at(0);
    CHECK(v == cc::vector{2, 4});
    v.remove_at(1);
    CHECK(v == cc::vector{2});
    v = {3, 2, 4};
    v.remove_range(1, 2);
    CHECK(v == cc::vector{3});
    v = {3, 4, 4};
    v.remove_first([](int i) { return i == 4; });
    CHECK(v == cc::vector{3, 4});
    v.remove_first([](int i) { return i == 5; });
    CHECK(v == cc::vector{3, 4});
}

TEST("cc::vector/alloc_vector interior references")
{
    struct foo
    {
        bool is_moved_from = false;
        bool is_destroyed = false;

        foo() = default;

        foo(foo& f)
        {
            CHECK(!f.is_moved_from);
            CHECK(!f.is_destroyed);
            f.is_moved_from = true;
        }
        foo& operator=(foo& f)
        {
            CHECK(!f.is_moved_from);
            CHECK(!f.is_destroyed);
            f.is_moved_from = true;
            return *this;
        }
        foo(foo const& f)
        {
            CHECK(!f.is_moved_from);
            CHECK(!f.is_destroyed);
        }
        foo& operator=(foo const& f)
        {
            CHECK(!f.is_moved_from);
            CHECK(!f.is_destroyed);
            return *this;
        }
        ~foo() { is_destroyed = true; }
    };
    {
        cc::vector<foo> fs;
        fs.push_back({});
        for (auto i = 0; i < 100; ++i)
            fs.push_back(fs[0]);
    }

    auto const f_test_alloc_vector = [](cc::allocator* alloc)
    {
        cc::alloc_vector<foo> afs(alloc);
        afs.push_back({});
        for (auto i = 0; i < 100; ++i)
            afs.push_back(afs[0]);
    };

    f_test_alloc_vector(cc::system_allocator);

    std::byte buffer[sizeof(foo) * 500];
    cc::linear_allocator linalloc(buffer);

    f_test_alloc_vector(&linalloc);
}

TEST("cc::vector/alloc_vector interior references (value types)")
{
    {
        cc::vector<int> vals;
        vals.push_back(7);

        for (auto i = 0; i < 100; ++i)
            vals.emplace_back(vals[0]);

        for (auto& v : vals)
            CHECK(v == 7);
    }
    {
        std::byte buffer[sizeof(int) * 500];
        cc::linear_allocator linalloc(buffer);

        cc::alloc_vector<int> vals(&linalloc);
        vals.push_back(7);

        for (auto i = 0; i < 100; ++i)
            vals.emplace_back(vals[0]);

        for (auto& v : vals)
            CHECK(v == 7);
    }
}

TEST("cc::alloc_vector realloc")
{
    // this test checks if realloc is correctly used when growing an alloc_vector with a trivially copyable T
    // this way, it only ever requires as much space as the maximum size, plus some margin for alignment / headers

    std::byte buffer[sizeof(int) * 520]; // slightly more space for stack alloc headers
    cc::stack_allocator stackalloc(buffer);

    cc::alloc_vector<int> vec(&stackalloc);

    for (auto i = 0; i < 500; ++i)
        vec.push_back(i);

    CHECK(true); // this test has the asserts in stack_allocator / alloc_vector instead of checks
}
