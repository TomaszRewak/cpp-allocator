#pragma once

#include <span>
#include <cstdint>
#include <stdexcept>
#include <cassert>

#include "block_allocator.h"
#include "free_memory_manager.h"
#include "utils.h"

namespace allocator {

template <typename _allocator_t>
concept allocator = requires(_allocator_t allocator, std::byte * data, std::size_t size) {
    { allocator.allocate_at_least(size) } -> std::same_as<allocation_result>;
    { allocator.deallocate(data) };
};

template <allocator _allocator_t, std::size_t _slab_size = 1024, std::size_t _min_allocation_size = 1>
class memory final {
public:
    void* allocate(size_t size) {
        auto* const data = _free_memory_manager.allocate(size);

        if (data)
            return data;

        allocate_new_block(size);

        return _free_memory_manager.allocate(size);
    }

    template <typename T, typename... Args>
    T* allocate(Args&&... args) {
        const auto size = alignof(T) > sizeof(T) || alignof(T) > alignof(std::max_align_t)
            ? sizeof(T) + alignof(T)
            : sizeof(T);

        auto* const allocated = allocate(sizeof(T));
        return new (allocated) T(std::forward<Args>(args)...);
    }

    template <typename T>
    void deallocate(const T* const data) {
        if (!data)
            return;

        auto* const non_const_data = const_cast<T*>(data);

        data->~T();

        _free_memory_manager.deallocate(reinterpret_cast<void*>(non_const_data));
    }

private:
    void allocate_new_block(size_t size) {
        const auto allocation_size = std::max(alignof(memory_slab<_slab_size>) + (std::max(size + sizeof(block), 0 + memory_slab<_slab_size>::data_block_size) + sizeof(memory_slab<_slab_size>::data_block_offset)) * 2, _min_allocation_size);
        const auto allocation_result = _allocator.allocate_at_least(allocation_size);
        auto* const aligned_data = reinterpret_cast<void*>(reinterpret_cast<std::uintptr_t>(allocation_result.ptr) / memory_slab<_slab_size>::memory_slab_alignment * memory_slab<_slab_size>::memory_slab_alignment);
        const auto slab_count = (allocation_size - (reinterpret_cast<std::uintptr_t>(aligned_data) - reinterpret_cast<std::uintptr_t>(allocation_result.ptr))) / sizeof(memory_slab<_slab_size>);

        assert(slab_count >= 1 && "aligned size must be at least the size of memory_slab");

        auto* const slab = std::launder(reinterpret_cast<memory_slab<_slab_size>*>(aligned_data));
        launder_slab(slab, slab_count);

        _free_memory_manager.add_new_memory_segment(slab);

        if (_last_block._ptr) {
            auto* const new_block = allocate<block>();
            *new_block = _last_block;
        }

        _last_block._ptr = allocation_result.ptr;
        _last_block._next = nullptr;
    }

    _allocator_t _allocator{};
    free_memory_manager<_slab_size> _free_memory_manager{};

    struct block {
        std::byte* _ptr;
        block* _next;
    } _last_block{ nullptr, nullptr };

    friend struct AllocatorTest;
};

template <std::size_t _size = 16 * 1024, std::size_t _slab_size = 1024>
using in_place_memory = memory<in_place_block_allocator<_size, _size>, _slab_size>;

}