#include <nexus/fuzz_test.hh>
#include <nexus/test.hh>

#include <cstdint>
#include <cstdio>

#include <clean-core/allocator.hh>
#include <clean-core/utility.hh>

namespace
{
void write_memory_pattern(void* memory, size_t size)
{
    uint8_t* const mem_as_u8 = static_cast<uint8_t*>(memory);

    for (auto i = 0u; i < size; ++i)
    {
        mem_as_u8[i] = reinterpret_cast<size_t>(mem_as_u8 + i) & 0xFF;
    }
}

void write_memory_zero(void* memory, size_t size) { std::memset(memory, 0, size); }

bool verify_memory_pattern(void const* memory, size_t size)
{
    uint8_t const* const mem_as_u8 = static_cast<uint8_t const*>(memory);
    for (auto i = 0u; i < size; ++i)
    {
        if (mem_as_u8[i] != (reinterpret_cast<size_t>(mem_as_u8 + i) & 0xFF))
            return false;
    }

    return true;
}

bool verify_memory_zero(void const* memory, size_t size)
{
    uint8_t const* const mem_as_u8 = static_cast<uint8_t const*>(memory);
    for (auto i = 0u; i < size; ++i)
    {
        if (mem_as_u8[i] != 0)
            return false;
    }

    return true;
}

template <class T>
constexpr bool is_aligned(T value, size_t alignment)
{
    return 0 == ((size_t)value & (alignment - 1));
}

// returns true if alignment requests are properly honored
bool test_alignment_requirements(cc::allocator* alloc)
{
    auto f_test_alignment = [&](unsigned align) -> bool {
        auto* const buf = alloc->alloc(1, align);
        bool const res = is_aligned(buf, align);
        alloc->free(buf);
        return res;
    };

    if (!f_test_alignment(4))
        return false;
    if (!f_test_alignment(8))
        return false;
    if (!f_test_alignment(16))
        return false;
    if (!f_test_alignment(32))
        return false;
    if (!f_test_alignment(64))
        return false;
    if (!f_test_alignment(128))
        return false;
    return true;
}

void test_basic_integrity(cc::allocator* alloc, bool free_all = false)
{
    auto const buf1_size = 200u;
    std::byte* const buf1 = alloc->alloc(buf1_size);

    write_memory_pattern(buf1, buf1_size);
    CHECK(verify_memory_pattern(buf1, buf1_size));

    auto const buf2_size = 300u;
    std::byte* const buf2 = alloc->alloc(buf2_size);
    CHECK(buf2 >= buf1 + buf1_size);

    CHECK(verify_memory_pattern(buf1, buf1_size));

    write_memory_zero(buf2, buf2_size);

    CHECK(verify_memory_pattern(buf1, buf1_size));
    CHECK(verify_memory_zero(buf2, buf2_size));

    alloc->free(buf2);
    CHECK(verify_memory_pattern(buf1, buf1_size));

    if (free_all)
        alloc->free(buf1);
}

void test_persistent_integrity(cc::allocator* alloc, bool free_all)
{
    std::byte* persistent_allocs[10] = {0};

    for (auto i = 0u; i < 10; ++i)
    {
        persistent_allocs[i] = alloc->alloc(200);
    }

    for (auto i = 0u; i < 10; ++i)
    {
        if (i % 2 == 0)
            write_memory_pattern(persistent_allocs[i], 200);
        else
            write_memory_zero(persistent_allocs[i], 200);
    }

    for (auto i = 0u; i < 10; ++i)
    {
        if (i % 2 == 0)
            CHECK(verify_memory_pattern(persistent_allocs[i], 200));
        else
            CHECK(verify_memory_zero(persistent_allocs[i], 200));
    }

    for (auto i = 0u; i < 10; ++i)
    {
        if (i % 2 == 1)
            write_memory_pattern(persistent_allocs[i], 200);
        else
            write_memory_zero(persistent_allocs[i], 200);
    }

    for (auto i = 0u; i < 10; ++i)
    {
        if (i % 2 == 1)
            CHECK(verify_memory_pattern(persistent_allocs[i], 200));
        else
            CHECK(verify_memory_zero(persistent_allocs[i], 200));
    }

    if (free_all)
    {
        for (auto i = 0u; i < 10; ++i)
        {
            auto const index = 9 - i;
            alloc->free(persistent_allocs[index]);
        }
    }
}

void test_fuzz_allocations(cc::allocator* alloc, tg::rng& rng, unsigned buffer_size)
{
    // allocates and deallocates chunks of memory of random sizes
    unsigned const safe_capacity_bytes = buffer_size / 4;
    unsigned num_allocated_bytes = 0;
    auto f_get_num_available_bytes = [&]() -> int { return safe_capacity_bytes - num_allocated_bytes; };

    struct allocation_t
    {
        std::byte* ptr;
        unsigned size;
    };

    unsigned const max_num_persistent_allocs = 50;
    allocation_t persistent_allocs[max_num_persistent_allocs] = {};
    unsigned num_persistent_allocs = 0;

    auto f_allocate = [&](unsigned size) -> unsigned {
        num_allocated_bytes += size;

        persistent_allocs[num_persistent_allocs++] = {alloc->alloc(size), size};
        return num_persistent_allocs - 1;
    };

    auto f_deallocate_pop = [&] {
        allocation_t const allocation = persistent_allocs[--num_persistent_allocs];

        num_allocated_bytes -= allocation.size;
        alloc->free(allocation.ptr);
    };

    unsigned const min_alloc_size = 4u;

    for (auto run_iteration = 0; run_iteration < 10; ++run_iteration)
    {
        REQUIRE(num_persistent_allocs == 0);
        REQUIRE(num_allocated_bytes == 0);

        while (f_get_num_available_bytes() > int(min_alloc_size) && num_persistent_allocs < max_num_persistent_allocs)
        {
            // allocate a random amount
            unsigned const alloc_size = tg::uniform(rng, min_alloc_size, cc::max(f_get_num_available_bytes() / 3u, min_alloc_size));
            unsigned const new_i = f_allocate(alloc_size);

            // write a pattern or zero depending on index
            auto const& alloc = persistent_allocs[new_i];
            if (new_i % 2 == 0)
                write_memory_pattern(alloc.ptr, alloc.size);
            else
                write_memory_zero(alloc.ptr, alloc.size);

            if (tg::uniform(rng, 0, 4) == 4)
            {
                // chance of immediately freeing
                f_deallocate_pop();
            }
        }

        for (auto i = 0u; i < num_persistent_allocs; ++i)
        {
            // test if all persisted allocations have the correct pattern
            allocation_t const& alloc = persistent_allocs[i];

            if (i % 2 == 0)
                CHECK(verify_memory_pattern(alloc.ptr, alloc.size));
            else
                CHECK(verify_memory_zero(alloc.ptr, alloc.size));
        }

        // free all
        while (num_persistent_allocs > 0)
        {
            f_deallocate_pop();
        }
    }
}
}

TEST("cc::allocator")
{
    // make sure the memory pattern verification works correctly
    char stack_buf[4096];
    write_memory_pattern(stack_buf, sizeof(stack_buf));
    REQUIRE(verify_memory_pattern(stack_buf, sizeof(stack_buf)));
}

TEST("cc::linear_allocator")
{
    std::byte linalloc_buf[4096];
    cc::linear_allocator linalloc(linalloc_buf);

    CHECK(test_alignment_requirements(&linalloc));

    test_basic_integrity(&linalloc, true);

    linalloc.reset();
    linalloc.alloc(sizeof(linalloc_buf));
    linalloc.reset();
    linalloc.alloc(sizeof(linalloc_buf));
    linalloc.reset();
}

TEST("cc::stack_allocator")
{
    std::byte stackalloc_buf[4096];
    cc::stack_allocator stackalloc(stackalloc_buf);

    CHECK(test_alignment_requirements(&stackalloc));

    test_basic_integrity(&stackalloc, false);

    // alloc and re-free
    for (auto i = 0u; i < 20; ++i)
    {
        std::byte* const buf_n = stackalloc.alloc(500);
        stackalloc.free(buf_n);
    }

    // realloc
    std::byte* const buf_realloc = stackalloc.alloc(250);
    stackalloc.realloc(buf_realloc, 250, 500);
    stackalloc.realloc(buf_realloc, 500, 750);
    stackalloc.realloc(buf_realloc, 750, 1000);
    stackalloc.realloc(buf_realloc, 1000, 100);
    stackalloc.free(buf_realloc);
}

TEST("cc::scratch_allocator")
{
    {
        std::byte scratchalloc_buf[4096];
        cc::scratch_allocator scratchalloc(scratchalloc_buf, nullptr);

        CHECK(test_alignment_requirements(&scratchalloc));
        CHECK(scratchalloc.is_empty());

        test_basic_integrity(&scratchalloc, true);
        CHECK(scratchalloc.is_empty());

        // alloc and re-free
        for (auto i = 0u; i < 50; ++i)
        {
            std::byte* const buf_n = scratchalloc.alloc(500);
            CHECK(scratchalloc.in_use(buf_n));
            scratchalloc.free(buf_n);
        }
        CHECK(scratchalloc.is_empty());

        test_persistent_integrity(&scratchalloc, true);
        CHECK(scratchalloc.is_empty());
    }
    // test correct fallback to the backing allocator
    {
        std::byte small_buf[64];
        cc::scratch_allocator backed_scratchalloc(small_buf, cc::system_allocator);

        auto* const large_alloc = backed_scratchalloc.alloc(1024);

        write_memory_pattern(large_alloc, 1024);
        CHECK(verify_memory_pattern(large_alloc, 1024));

        backed_scratchalloc.free(large_alloc);
    }
}

FUZZ_TEST("cc::scratch_allocator fuzz")(tg::rng& rng)
{
    std::byte scratchalloc_buf[8192];
    cc::scratch_allocator scratchalloc(scratchalloc_buf, nullptr);

    test_fuzz_allocations(&scratchalloc, rng, sizeof(scratchalloc_buf));
}
