#pragma once

#include <stddef.h>
#include <stdint.h>

#include <atomic>

#include <clean-core/alloc_array.hh>
#include <clean-core/assert.hh>
#include <clean-core/bits.hh>

namespace cc
{
// Multi-Producer/Multi-Consumer Queue
// FIFO
// ~75 cycles per enqueue and dequeue under contention
// Adapted from http://www.1024cores.net/home/lock-free-algorithms/queues/bounded-mpmc-queue
template <class T>
struct mpmc_queue
{
public:
    mpmc_queue() = default;
    explicit mpmc_queue(size_t num_elements, cc::allocator* allocator) { initialize(num_elements, allocator); }

    void initialize(size_t num_elements, cc::allocator* allocator)
    {
        CC_ASSERT(num_elements >= 2 && cc::is_pow2(num_elements) && "mpmc_queue size not a power of two");

        _buffer_mask = num_elements - 1;
        _buffer.reset(allocator, num_elements);
        for (size_t i = 0; i < num_elements; ++i)
        {
            _buffer[i].sequence_.store(i, std::memory_order_relaxed);
        }

        _enqueue_pos.store(0, std::memory_order_relaxed);
        _dequeue_pos.store(0, std::memory_order_relaxed);
    }

    bool enqueue(const T& data)
    {
        cell* cell;
        size_t pos = _enqueue_pos.load(std::memory_order_relaxed);
        for (;;)
        {
            cell = &_buffer[pos & _buffer_mask];
            size_t seq = cell->sequence_.load(std::memory_order_acquire);
            intptr_t diff = static_cast<intptr_t>(seq) - static_cast<intptr_t>(pos);
            if (diff == 0)
            {
                if (_enqueue_pos.compare_exchange_weak(pos, pos + 1, std::memory_order_relaxed))
                    break;
            }
            else if (diff < 0)
                return false;
            else
                pos = _enqueue_pos.load(std::memory_order_relaxed);
        }
        cell->data_ = data;
        cell->sequence_.store(pos + 1, std::memory_order_release);
        return true;
    }

    bool dequeue(T* out_data)
    {
        cell* cell;
        size_t pos = _dequeue_pos.load(std::memory_order_relaxed);
        for (;;)
        {
            cell = &_buffer[pos & _buffer_mask];
            size_t seq = cell->sequence_.load(std::memory_order_acquire);
            intptr_t diff = static_cast<intptr_t>(seq) - static_cast<intptr_t>(pos + 1);
            if (diff == 0)
            {
                if (_dequeue_pos.compare_exchange_weak(pos, pos + 1, std::memory_order_relaxed))
                    break;
            }
            else if (diff < 0)
                return false;
            else
                pos = _dequeue_pos.load(std::memory_order_relaxed);
        }
        *out_data = cell->data_;
        cell->sequence_.store(pos + _buffer_mask + 1, std::memory_order_release);
        return true;
    }

private:
    struct cell
    {
        std::atomic<size_t> sequence_;
        T data_;
    };

    using cacheline_pad_t = char[64];

    cacheline_pad_t _pad0;

    cc::alloc_array<cell> _buffer;
    size_t _buffer_mask = 0;

    cacheline_pad_t _pad1;

    std::atomic<size_t> _enqueue_pos = {0};

    cacheline_pad_t _pad2;

    std::atomic<size_t> _dequeue_pos = {0};

    cacheline_pad_t _pad3;

    mpmc_queue(mpmc_queue const& other) = delete;
    mpmc_queue(mpmc_queue&& other) noexcept = delete;
    mpmc_queue& operator=(mpmc_queue const& other) = delete;
    mpmc_queue& operator=(mpmc_queue&& other) noexcept = delete;
};
}
