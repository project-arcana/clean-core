#include "memory.hh"

#include <clean-core/assert.hh>
#include <clean-core/macros.hh>
#include <clean-core/utility.hh>

#ifdef CC_OS_WINDOWS
#include <clean-core/native/win32_sanitized.hh>
#endif

#if defined(CC_OS_LINUX) || defined(CC_OS_MACOS)
#include <fcntl.h>
#include <unistd.h>

#include <sys/mman.h>
#include <sys/stat.h>

#ifndef MAP_UNINITIALIZED
#define MAP_UNINITIALIZED 0x4000000
#endif

#endif

std::byte* cc::reserve_virtual_memory(size_t size)
{
#ifdef CC_OS_WINDOWS

    void* res = VirtualAlloc(nullptr, size, MEM_RESERVE, PAGE_NOACCESS);
    CC_ASSERT(res != nullptr && "virtual reserve failed");
    return static_cast<std::byte*>(res);

#elif defined(CC_OS_LINUX) || defined(CC_OS_MACOS)

    // no target address, no file descriptor
    // flags: MAP_PRIVATE since not sharing with processes,
    // MAP_ANONYMOUS since no file descriptor
    // MAP_UNINITIALIZED likely no effect but can avoid commits
    // MAP_NORESERVE to avoid overcommit checks for huge allocations
    // TODO: does MAP_NORESERVE require an additional mmap call during commit?
    // ref https://stackoverflow.com/questions/15261527/how-can-i-reserve-virtual-memory-in-linux
    void* res = ::mmap(nullptr, size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_UNINITIALIZED | MAP_NORESERVE, -1, 4096);
    CC_ASSERT(res != MAP_FAILED && "virtual reserve failed");
    return static_cast<std::byte*>(res);

#else
    static_assert(false, "unsupported platform");
    return nullptr;
#endif
}

void cc::free_virtual_memory(std::byte* ptr, size_t size_bytes)
{
#ifdef CC_OS_WINDOWS

    (void)size_bytes;
    auto const res = VirtualFree(ptr, 0, MEM_RELEASE);
    CC_ASSERT(!!res && "virtual release failed");

#elif defined(CC_OS_LINUX) || defined(CC_OS_MACOS)

    auto const res = ::munmap(ptr, size_bytes);
    CC_ASSERT(res == 0 && "virtual release failed");

#else
    static_assert(false, "unsupported platform");
#endif
}

static volatile uint8_t s_byte_sink;

void cc::prefault_memory(std::byte *ptr, size_t size_bytes)
{
    if (size_bytes == 0)
        return;

    // generates nice SIMD code: https://godbolt.org/z/5YEPz4zP7

    // TODO: separate counters?
    uint8_t s = uint8_t(*ptr);
    auto end = ptr + size_bytes;
    ptr = cc::align_up(ptr, 4096);
    while (ptr < end) {
        s ^= uint8_t(*ptr);
        ptr += 4096;
    }
    s_byte_sink = s;
}

void cc::commit_physical_memory(std::byte* ptr, size_t size)
{
#ifdef CC_OS_WINDOWS

    void* res = VirtualAlloc(ptr, size, MEM_COMMIT, PAGE_READWRITE);
    CC_ASSERT(res != nullptr && "virtual commit failed");

#elif defined(CC_OS_LINUX)|| defined(CC_OS_MACOS)

    // ensure ptr and size are page-aligned
    auto new_ptr = cc::align_down(ptr, 4096);
    auto new_size = cc::align_up(size + (ptr - new_ptr), 4096);

    int const res = ::mprotect(new_ptr, new_size, PROT_READ | PROT_WRITE);
    CC_ASSERT(res == 0 && "virtual commit failed");

#else
    static_assert(false, "unsupported platform");
#endif
}

void cc::decommit_physical_memory(std::byte* ptr, size_t size)
{
#ifdef CC_OS_WINDOWS

    auto const res = VirtualFree(ptr, size, MEM_DECOMMIT);
    CC_ASSERT(!!res && "virtual decommit failed");

#elif defined(CC_OS_LINUX)|| defined(CC_OS_MACOS)

    // ensure ptr and size are page-aligned
    auto new_ptr = cc::align_down(ptr, 4096);
    auto new_size = cc::align_up(size + (ptr - new_ptr), 4096);

    // TODO: not sure if this actually decommits memory
    int const res = ::mprotect(new_ptr, new_size, PROT_NONE);
    CC_ASSERT(res == 0 && "virtual decommit failed");

#else
    static_assert(false, "unsupported platform");
#endif
}

std::byte* cc::grow_physical_memory(std::byte* physical_current, std::byte* physical_end, std::byte* virtual_end, size_t chunk_size, size_t grow_num_bytes)
{
    if (physical_current + grow_num_bytes <= physical_end)
        return physical_end; // no growth required

    // round up new size to multiple of chunk size
    size_t const new_commit_size = cc::align_up(grow_num_bytes, chunk_size);
    CC_ASSERT(physical_end + new_commit_size <= virtual_end && "grow_physical_memory: virtual memory overcommitted");

    // allocate new pages at the end of current physical commit range
    cc::commit_physical_memory(physical_end, new_commit_size);
    physical_end += new_commit_size;
    return physical_end;
}
