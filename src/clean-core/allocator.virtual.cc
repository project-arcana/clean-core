#include "allocator.hh"

#include <clean-core/bits.hh>
#include <clean-core/macros.hh>
#include <clean-core/utility.hh>

#ifdef CC_OS_WINDOWS
#include <clean-core/native/win32_sanitized.hh>
#endif

#ifdef CC_OS_LINUX
#include <fcntl.h>
#include <unistd.h>

#include <sys/mman.h>
#include <sys/stat.h>
#endif

namespace
{
struct vstack_alloc_header
{
    uint32_t padding;
    int32_t alloc_id;
};

// [... pad ...] [header] [data]
std::byte* valign_up_with_header(std::byte* head, size_t align, size_t header_size)
{
    std::byte* padded_res = cc::align_up(head, align);
    size_t const padding = size_t(padded_res - head);

    if (padding < header_size)
    {
        // header does not fit - align up
        header_size -= padding;

        if (header_size % align > 0)
        {
            padded_res += align * (1 + (header_size / align));
        }
        else
        {
            padded_res += align * (header_size / align);
        }
    }

    return padded_res;
}
}

cc::virtual_stack_allocator::virtual_stack_allocator(size_t max_size_bytes, size_t chunk_size_bytes)
  : _virtual_begin(vmem_reserve_virtual(max_size_bytes)),
    _virtual_end(_virtual_begin + max_size_bytes),
    _physical_current(_virtual_begin),
    _physical_end(_virtual_begin),
    _last_alloc_id(0),
    _is_lifo_intact(true),
    _chunk_size_bytes(chunk_size_bytes)
{
    CC_ASSERT(max_size_bytes > 0 && chunk_size_bytes > 0 && "invalid sizes");
    CC_ASSERT(is_pow2(chunk_size_bytes) && "Chunk size must be a power of 2");
    CC_ASSERT(_virtual_begin != nullptr && "virtual reserve failed");
}

cc::virtual_stack_allocator::~virtual_stack_allocator()
{
    if (_virtual_begin)
    {
        vmem_free_virtual(_virtual_begin, _virtual_end - _virtual_begin);
    }
}

std::byte* cc::virtual_stack_allocator::alloc(size_t size, size_t align)
{
    CC_ASSERT(_virtual_begin != nullptr && "virtual_stack_allocator uninitialized");

    std::byte* const padded_res = valign_up_with_header(_physical_current, align, sizeof(vstack_alloc_header));
    size_t const num_padding_bytes = size_t(padded_res - _physical_current);
    size_t const required_size = num_padding_bytes + size;
    grow_physical(required_size);

    ++_last_alloc_id;
    vstack_alloc_header header = {};
    header.padding = uint32_t(num_padding_bytes);
    header.alloc_id = _last_alloc_id;

    std::memcpy(padded_res - sizeof(header), &header, sizeof(header));

    _physical_current = padded_res + size;
    return padded_res;
}

void cc::virtual_stack_allocator::free(void* ptr)
{
    if (ptr == nullptr)
        return;

    if (!_is_lifo_intact)
        return;

    std::byte* const byte_ptr = static_cast<std::byte*>(ptr);
    vstack_alloc_header const* const alloc_header = (vstack_alloc_header*)(byte_ptr - sizeof(vstack_alloc_header));

    if (alloc_header->alloc_id != _last_alloc_id)
    {
        // this ptr is not the most recent allocation, LIFO is broken
        // continue as a linear allocator until the next reset
        _is_lifo_intact = false;
        return;
    }

    // LIFO still intact, free the allocation
    --_last_alloc_id;
    _physical_current = byte_ptr - alloc_header->padding;
}

std::byte* cc::virtual_stack_allocator::realloc(void* ptr, size_t old_size, size_t new_size, size_t align)
{
    if (ptr == nullptr || !_is_lifo_intact)
        return this->alloc(new_size, align);

    (void)align;

    // no need to memcpy, the memory remains the same
    std::byte* const byte_ptr = static_cast<std::byte*>(ptr);
    vstack_alloc_header const* const alloc_header = (vstack_alloc_header*)(byte_ptr - sizeof(vstack_alloc_header));

    if (alloc_header->alloc_id != _last_alloc_id)
    {
        // this ptr is not the most recent allocation, LIFO is broken
        // continue as a linear allocator until the next reset
        _is_lifo_intact = false;
        return this->alloc(new_size, align);
    }

    size_t const real_old_size = _physical_current - byte_ptr;
    CC_ASSERT(real_old_size == old_size && "incorrect old size");

    if (new_size > old_size)
    {
        // grow physically to meet demand
        size_t const num_new_bytes = new_size - old_size;
        grow_physical(num_new_bytes);
    }

    _physical_current = byte_ptr + new_size;
    return byte_ptr;
}

size_t cc::virtual_stack_allocator::reset()
{
    size_t const num_bytes_allocated = _physical_current - _virtual_begin;
    _physical_current = _virtual_begin;
    _last_alloc_id = 0;
    _is_lifo_intact = true;
    return num_bytes_allocated;
}

size_t cc::virtual_stack_allocator::decommit_idle_memory()
{
    // align up to the start of the first empty page
    std::byte* const ptr = cc::align_up(_physical_current, _chunk_size_bytes);
    // then free all memory between that and _physical_end
    size_t const size_to_free = _physical_end - ptr;

    if (size_to_free > 0)
    {
        vmem_decommit_physical(ptr, size_to_free);
        _physical_end = ptr;
    }

    return size_to_free;
}

bool cc::virtual_stack_allocator::is_latest_allocation(void* ptr) const
{
    CC_CONTRACT(ptr);

    std::byte* const byte_ptr = static_cast<std::byte*>(ptr);
    CC_ASSERT(byte_ptr > _virtual_begin && _physical_end && "pointer not inside physical region");

    vstack_alloc_header const* const alloc_header = (vstack_alloc_header*)(byte_ptr - sizeof(vstack_alloc_header));
    return alloc_header->alloc_id == _last_alloc_id;
}

void cc::virtual_stack_allocator::grow_physical(size_t num_bytes)
{
    if (_physical_current + num_bytes <= _physical_end)
        return; // no growth required

    // round up new size to multiple of chunk size
    size_t const new_commit_size = cc::align_up(num_bytes, _chunk_size_bytes);

    if (_physical_end + new_commit_size > _virtual_end)
    {
        // doesn't fit virtual address space
        CC_ASSERT(false && "virtual_stack_allocator overcommitted");
    }

    // allocate new pages at the end of current physical commit range
    vmem_commmit_physical(_physical_end, new_commit_size);
    _physical_end += new_commit_size;
}

std::byte* cc::virtual_stack_allocator::vmem_reserve_virtual(size_t size)
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

void cc::virtual_stack_allocator::vmem_free_virtual(std::byte* ptr, size_t size_bytes)
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

void cc::virtual_stack_allocator::vmem_commmit_physical(std::byte* ptr, size_t size)
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

void cc::virtual_stack_allocator::vmem_decommit_physical(std::byte* ptr, size_t size)
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
