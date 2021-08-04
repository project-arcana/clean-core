#include <clean-core/allocator.hh>

#include <clean-core/assert.hh>

#include <clean-core/detail/lib/tlsf.hh>

void cc::tlsf_allocator::initialize(cc::span<std::byte> buffer)
{
    CC_ASSERT(_tlsf == nullptr && "double init");
    CC_ASSERT(buffer.size() > tlsf_size() && "buffer not large enough");

    _tlsf = tlsf_create_with_pool(buffer.data(), buffer.size());
    CC_ASSERT(_tlsf != nullptr && "failed to create TLSF");
}

void cc::tlsf_allocator::destroy()
{
    if (_tlsf)
    {
        tlsf_destroy(_tlsf);
        _tlsf = nullptr;
    }
}

void cc::tlsf_allocator::add_pool(cc::span<std::byte> buffer)
{
    CC_ASSERT(_tlsf && "unitialized");
    void* pool = tlsf_add_pool(_tlsf, buffer.data(), buffer.size());
    CC_ASSERT(pool != nullptr && "failed to add TLSF pool");
}

bool cc::tlsf_allocator::check_consistency()
{
    CC_ASSERT(_tlsf && "unitialized");
    return tlsf_check(_tlsf) == 0;
}

std::byte* cc::tlsf_allocator::alloc(size_t size, size_t align)
{
    CC_ASSERT(size > 0 && "Attempted empty TLSF allocation");
    auto const res = static_cast<std::byte*>(tlsf_memalign(_tlsf, align, size));
    CC_ASSERT(res != nullptr && "TLSF full");
    return res;
}

void cc::tlsf_allocator::free(void* ptr) { tlsf_free(_tlsf, ptr); }

std::byte* cc::tlsf_allocator::realloc(void* ptr, size_t old_size, size_t new_size, size_t align)
{
    (void)old_size;
    (void)align;
    return static_cast<std::byte*>(tlsf_realloc(_tlsf, ptr, new_size));
}
