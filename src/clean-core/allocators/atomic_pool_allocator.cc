#include "atomic_pool_allocator.hh"

cc::atomic_pool_allocator::atomic_pool_allocator(span<std::byte> buffer, size_t block_size) { initialize(buffer, block_size); }

void cc::atomic_pool_allocator::initialize(span<std::byte> buffer, size_t block_size)
{
    CC_ASSERT(_buffer_begin == nullptr && "double initialize");
    _buffer_begin = buffer.data();
    _buffer_size = buffer.size();
    _block_size = block_size;

    CC_ASSERT(_block_size >= sizeof(std::byte*) && "blocks must be large enough to accomodate a pointer");
    CC_ASSERT(_block_size <= _buffer_size && "not enough memory to allocate a single block");

    size_t const num_blocks = _buffer_size / _block_size;

    // initialize linked list
    for (auto i = 0u; i < num_blocks - 1; ++i)
    {
        std::byte* node_ptr = &_buffer_begin[i * block_size];
        new (cc::placement_new, node_ptr) std::byte*(&_buffer_begin[(i + 1) * block_size]);
    }

    // initialize linked list tail
    {
        std::byte* tail_ptr = &_buffer_begin[(num_blocks - 1) * block_size];
        new (cc::placement_new, tail_ptr) std::byte*(nullptr);
    }

    _first_free_node = &_buffer_begin[0];
}
