#include <cstdint>
#include <type_traits>

// checking the "sanity" of the c++ type system

static_assert(!std::is_same_v<char, signed char>);
static_assert(!std::is_same_v<char, unsigned char>);

static_assert(std::is_same_v<int, signed int>);
static_assert(std::is_same_v<long, signed long>);

static_assert(!std::is_same_v<int, long>);
static_assert(!std::is_same_v<long, long long>);

static_assert(sizeof(int) == sizeof(long) || sizeof(long) == sizeof(long long));
