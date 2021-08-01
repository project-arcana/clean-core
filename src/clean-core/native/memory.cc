#include "memory.hh"

#include <clean-core/assert.hh>
#include <clean-core/macros.hh>

#ifdef CC_OS_WINDOWS
#include <clean-core/native/win32_sanitized.hh>
#endif

#ifdef CC_OS_LINUX
#include <fcntl.h>
#include <unistd.h>

#include <sys/mman.h>
#include <sys/stat.h>
#endif

std::byte* cc::reserve_virtual_memory(size_t size)
{
#ifdef CC_OS_WINDOWS

    void* res = VirtualAlloc(nullptr, size, MEM_RESERVE, PAGE_NOACCESS);
    CC_ASSERT(res != nullptr && "virtual reserve failed");
    return static_cast<std::byte*>(res);

#elif defined(CC_OS_LINUX)

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

#elif defined(CC_OS_LINUX)

    auto const res = ::munmap(ptr, size_bytes);
    CC_ASSERT(res == 0 && "virtual release failed");

#else
    static_assert(false, "unsupported platform");
#endif
}

void cc::commit_physical_memory(std::byte* ptr, size_t size)
{
#ifdef CC_OS_WINDOWS

    void* res = VirtualAlloc(ptr, size, MEM_COMMIT, PAGE_READWRITE);
    CC_ASSERT(res != nullptr && "virtual commit failed");

#elif defined(CC_OS_LINUX)

    int const res = ::mprotect(ptr, size, PROT_READ | PROT_WRITE);
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

#elif defined(CC_OS_LINUX)

    // TODO: not sure if this actually decommits memory
    int const res = ::mprotect(ptr, size, PROT_NONE);
    CC_ASSERT(res == 0 && "virtual decommit failed");

#else
    static_assert(false, "unsupported platform");
#endif
}
