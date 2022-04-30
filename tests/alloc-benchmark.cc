#include <nexus/test.hh>

#include <iostream>
#include <string>
#include <vector>

#include <ctracer/benchmark.hh>

#include <clean-core/allocate.hh>
#include <clean-core/array.hh>

#define DO_BENCHMARK 0

namespace
{
struct linear_alloc
{
    linear_alloc(size_t max_size)
    {
        _data = new std::byte[max_size];
        _curr = _data;
    }
    ~linear_alloc() { delete[] _data; }

    template <class T>
    T* alloc()
    {
        auto d = new (_curr) T();
        _curr += sizeof(T);
        return d;
    }
    template <class T>
    T* alloc_array(size_t size)
    {
        auto d = new (_curr) T[size]();
        _curr += size * sizeof(T);
        return d;
    }

    void reset() { _curr = _data; }

private:
    std::byte* _data;
    std::byte* _curr;
};

template <class F, class C>
void measure(std::string name, size_t samples, F&& f, C&& cleanup)
{
    constexpr auto cnt = 3;
    uint64_t cycles[cnt];
    for (auto i = 0; i < cnt; ++i)
    {
        auto c = ct::current_cycles();
        f();
        cycles[i] = (ct::current_cycles() - c) / samples;
        cleanup();
    }
    // std::cout << name << ": " << cycles[0] << ", " << cycles[1] << ", " << cycles[2] << " cycles / sample" << std::endl;
    std::cout << name << ": " << cycles[2] << " cycles / sample" << std::endl;
}
template <class F>
void measure(std::string name, size_t samples, F&& f)
{
    return measure(name, samples, f, [] {});
}

}
TEST("cc::alloc benchmark")
{
#if !DO_BENCHMARK
    CHECK(true);
    return;
#endif

    std::vector<int*> ptrs;
    std::vector<cc::array<int, 100>*> ptrs2;
    ptrs.resize(10000);
    ptrs2.resize(10000);

    linear_alloc la(10000 * 1000 * sizeof(int));

    measure("(cold) new/delete int", ptrs.size(), [&] {
        for (auto& p : ptrs)
            p = new int();
        for (auto& p : ptrs)
            ct::sink << p;
        for (auto p : ptrs)
            delete p;
    });
    measure("(cold) linear alloc int", ptrs.size(), [&] {
        for (auto& p : ptrs)
            p = la.alloc<int>();
        for (auto& p : ptrs)
            ct::sink << p;
        la.reset();
    });
    measure("(cold) cc::alloc int", ptrs.size(), [&] {
        for (auto& p : ptrs)
            p = cc::alloc<int>();
        for (auto& p : ptrs)
            ct::sink << p;
        for (auto p : ptrs)
            cc::free(p);
    });

    measure("(hot) new/delete int", ptrs.size(), [&] {
        for (auto& p : ptrs)
        {
            p = new int();
            ct::sink << p;
            delete p;
        }
    });
    measure("(hot) linear alloc int", ptrs.size(), [&] {
        for (auto& p : ptrs)
        {
            p = la.alloc<int>();
            ct::sink << p;
            la.reset();
        }
    });
    measure("(hot) cc::alloc int", ptrs.size(), [&] {
        for (auto& p : ptrs)
        {
            p = cc::alloc<int>();
            ct::sink << p;
            cc::free(p);
        }
    });

    measure("(cold) new/delete array<int, 100>", ptrs.size(), [&] {
        for (auto& p : ptrs2)
            p = new cc::array<int, 100>();
        for (auto& p : ptrs)
            ct::sink << p;
        for (auto p : ptrs2)
            delete p;
    });
    measure("(cold) linear alloc array<int, 100>", ptrs.size(), [&] {
        for (auto& p : ptrs2)
            p = la.alloc<cc::array<int, 100>>();
        for (auto& p : ptrs)
            ct::sink << p;
        la.reset();
    });
    measure("(cold) cc::alloc array<int, 100>", ptrs.size(), [&] {
        for (auto& p : ptrs2)
            p = cc::alloc<cc::array<int, 100>>();
        for (auto& p : ptrs)
            ct::sink << p;
        for (auto p : ptrs2)
            cc::free(p);
    });

    measure("(hot) new/delete array<int, 100>", ptrs.size(), [&] {
        for (auto& p : ptrs2)
        {
            p = new cc::array<int, 100>();
            ct::sink << p;
            delete p;
        }
    });
    measure("(hot) linear alloc array<int, 100>", ptrs.size(), [&] {
        for (auto& p : ptrs2)
        {
            p = la.alloc<cc::array<int, 100>>();
            ct::sink << p;
            la.reset();
        }
    });
    measure("(hot) cc::alloc array<int, 100>", ptrs.size(), [&] {
        for (auto& p : ptrs2)
        {
            p = cc::alloc<cc::array<int, 100>>();
            ct::sink << p;
            cc::free(p);
        }
    });
}
