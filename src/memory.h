#pragma once

#include <span>
#include <cstdint>
#include <stdexcept>
#include <cassert>

#include "block_allocator.h"
#include "free_segments_manager.h"

namespace allocator {

template <typename _allocator_t = in_place_block_allocator<1024>>
class memory final {
public:
    std::byte* allocate(size_t size) {
        if (!_free_segments) {
            allocate_new_block(size);
        }

        auto* const current_segment_header = _free_segments;
        auto* const current_segment_data = reinterpret_cast<std::byte*>(current_segment_header + 1);

        assert(current_segment_header->previous_free_segment == nullptr);
        assert(current_segment_header->data_size >= size);

        if (current_segment_header->data_size > size + sizeof(segment_header)) {
            auto* const remaining_segment_header = std::launder(reinterpret_cast<segment_header*>(current_segment_data + size));

            remaining_segment_header->previous_neighbor_segment = current_segment_header;
            remaining_segment_header->next_neighbor_segment = current_segment_header->next_neighbor_segment;
            remaining_segment_header->previous_free_segment = current_segment_header->previous_free_segment;
            remaining_segment_header->next_free_segment = current_segment_header->next_free_segment;
            remaining_segment_header->data_size = current_segment_header->data_size - size - sizeof(segment_header);

            if (current_segment_header->next_neighbor_segment)
                current_segment_header->next_neighbor_segment->previous_neighbor_segment = remaining_segment_header;
            if (current_segment_header->next_free_segment)
                current_segment_header->next_free_segment->previous_free_segment = remaining_segment_header;
            if (current_segment_header->previous_free_segment)
                current_segment_header->previous_free_segment->next_free_segment = remaining_segment_header;

            current_segment_header->data_size = size;
            current_segment_header->next_neighbor_segment = remaining_segment_header;
            current_segment_header->next_free_segment = nullptr;
            current_segment_header->previous_free_segment = nullptr;

            _free_segments = remaining_segment_header;
        }
        else {
            if (current_segment_header->next_free_segment)
                current_segment_header->next_free_segment->previous_free_segment = current_segment_header->previous_free_segment;
            if (current_segment_header->previous_free_segment)
                current_segment_header->previous_free_segment->next_free_segment = current_segment_header->next_free_segment;

            current_segment_header->previous_free_segment = nullptr;
            current_segment_header->next_free_segment = nullptr;

            _free_segments = current_segment_header->next_free_segment;
        }

        return current_segment_data;
    }

    template <typename T, typename... Args>
    T* allocate(Args&&... args) {
        auto* const allocated = allocate(sizeof(T));
        return new (allocated) T(std::forward<Args>(args)...);
    }

    template <typename T>
    void deallocate(const T* const data) {
        if (!data)
            return;

        data->~T();

        auto* const freed_segment_header = std::launder(const_cast<segment_header*>(reinterpret_cast<const segment_header*>(data) - 1));

        freed_segment_header->next_free_segment = _free_segments;
        freed_segment_header->previous_free_segment = nullptr;

        if (freed_segment_header->next_free_segment)
            freed_segment_header->next_free_segment->previous_free_segment = freed_segment_header;

        _free_segments = freed_segment_header;
    }

private:
    void allocate_new_block(size_t size) {
        assert(_free_segments == nullptr);

        // TODO: see how much was actually allocated
        const auto allocation_result = _allocator.allocate_at_least(size + sizeof(block_header) + sizeof(segment_header));
        const auto new_block_size = allocation_result.count;

        auto* const new_block = allocation_result.ptr;
        auto* const new_block_header = std::launder(reinterpret_cast<block_header*>(new_block));
        auto* const new_segment = new_block + sizeof(block_header);
        auto* const new_segment_header = std::launder(reinterpret_cast<segment_header*>(new_segment));

        new_block_header->next_block = _block;

        new_segment_header->previous_neighbor_segment = nullptr;
        new_segment_header->next_neighbor_segment = nullptr;
        new_segment_header->previous_free_segment = nullptr;
        new_segment_header->next_free_segment = nullptr;
        new_segment_header->data_size = new_block_size - sizeof(block_header) - sizeof(segment_header);

        _block = new_block;
        _free_segments = new_segment_header;
    }

    _allocator_t _allocator{};
    std::byte* _block{ nullptr };
    segment_header* _free_segments{ nullptr };
    //free_segments_manager _free_segments_manager;

    friend struct AllocatorTest;
};

template <std::size_t _size = 1024>
using in_place_memory = memory<in_place_block_allocator<_size>>;

}