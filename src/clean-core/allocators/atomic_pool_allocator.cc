#include "atomic_pool_allocator.hh"

#include <clean-core/utility.hh>

cc::atomic_pool_allocator::atomic_pool_allocator(cc::allocator* alloc, size_t block_size_bytes, size_t num_blocks, size_t buffer_align)
{
    initialize(alloc, block_size_bytes, num_blocks, buffer_align);
}

void cc::atomic_pool_allocator::initialize(cc::allocator* alloc, size_t block_size_bytes, size_t num_blocks, size_t buffer_align)
{
    CC_ASSERT(alloc);
    CC_ASSERT(block_size_bytes >= sizeof(void*) && "blocks must be large enough to accomodate a pointer");
    CC_ASSERT(num_blocks > 1 && "pool allocator too small");

    CC_ASSERT(_backing_alloc == nullptr && "double initialize");

    _block_size = block_size_bytes;
    _buffer_size = block_size_bytes * num_blocks;
    _backing_alloc = alloc;

    // allocate buffer
    _buffer_begin = _backing_alloc->alloc(_buffer_size, cc::max<size_t>(64, buffer_align));

    // allocate free list
    _free_list = reinterpret_cast<int32_t*>(_backing_alloc->alloc(sizeof(int32_t) * num_blocks, 64u));
    CC_ASSERT(_buffer_begin && _free_list && "backing allocator failed to provide memory");

    // initialize free list
    for (int32_t i = 0; i < num_blocks - 1; ++i)
    {
        _free_list[i] = i + 1;
    }
    // initialize free list tail
    _free_list[num_blocks - 1] = -1;

    // initialize first free node index
    VersionedIndex head;
    head.set_index(0);
    _first_free_node.store(head);

    //// initialize linked list
    // for (auto i = 0u; i < num_blocks - 1; ++i)
    //{
    //     std::byte* node_ptr = &_buffer_begin[i * _block_size];
    //     new (cc::placement_new, node_ptr) std::byte*(&_buffer_begin[(i + 1) * _block_size]);
    // }

    //// initialize linked list tail
    //{
    //    std::byte* tail_ptr = &_buffer_begin[(num_blocks - 1) * _block_size];
    //    new (cc::placement_new, tail_ptr) std::byte*(nullptr);
    //}

    //_first_free_node = &_buffer_begin[0];
}

void cc::atomic_pool_allocator::destroy()
{
    if (!_buffer_begin)
        return;

    _backing_alloc->free(_buffer_begin);
    _buffer_begin = nullptr;

    _backing_alloc->free(_free_list);
    _free_list = nullptr;
    _backing_alloc = nullptr;
}
