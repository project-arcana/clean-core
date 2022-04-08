#include <nexus/test.hh>

#include <clean-core/utility.hh>
#include <clean-core/vector.hh>

#include <vector>

namespace foo
{
bool used_custom_swap = false;

struct bar
{
};
void swap(bar&, bar&) { used_custom_swap = true; }
}

namespace fuz
{
struct baz
{
};
}

TEST("cc::swap")
{
    foo::bar a, b;

    // found via ADL
    foo::used_custom_swap = false;
    swap(a, b);
    CHECK(foo::used_custom_swap);

    // std::swap does NOT find via ADL
    foo::used_custom_swap = false;
    std::swap(a, b);
    CHECK(!foo::used_custom_swap);

    // cc::swap finds via ADL
    foo::used_custom_swap = false;
    cc::swap(a, b);
    CHECK(foo::used_custom_swap);

    cc::vector<int> u, v;
    // swap(u, v); - ERROR: no adl swap
    std::swap(u, v); // OK via move
    cc::swap(u, v);  // OK via move
}

TEST("cc::swap - using std::swap")
{
    using std::swap;
    foo::bar a, b;

    // found via ADL
    foo::used_custom_swap = false;
    swap(a, b);
    CHECK(foo::used_custom_swap);

    // std::swap does NOT find via ADL
    foo::used_custom_swap = false;
    std::swap(a, b);
    CHECK(!foo::used_custom_swap);

    // cc::swap finds via ADL
    foo::used_custom_swap = false;
    cc::swap(a, b);
    CHECK(foo::used_custom_swap);

    cc::vector<int> u, v;
    swap(u, v);      // OK, uses std::move
    std::swap(u, v); // OK via move
    cc::swap(u, v);  // OK via move
}
