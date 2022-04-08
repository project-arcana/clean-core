#include <nexus/fuzz_test.hh>

#include <clean-core/array.hh>
#include <clean-core/base64.hh>
#include <clean-core/vector.hh>

FUZZ_TEST("cc::base64")(tg::rng& rng)
{
    auto l = uniform(rng, 0, 50);
    auto bytes = cc::vector<std::byte>::defaulted(l);
    for (auto& c : bytes)
        c = (std::byte)uniform(rng, 0, 255);

    auto s = cc::base64_encode(bytes);
    auto bytes2 = cc::base64_decode(s);

    CHECK(bytes == bytes2);
}
