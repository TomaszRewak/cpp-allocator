#pragma once

#include <array>
#include <optional>
#include <limits>
#include <stdexcept>
#include <cstdint>
#include <cassert>

#include "memory_slab.h"

namespace allocator {

template <std::size_t _slab_size = 1024>
class free_memory_manager final {
private:
    static constexpr std::size_t _max_buckets = std::numeric_limits<std::size_t>::digits;

public:
    void add_new_memory_segment(memory_slab<_slab_size>* const slab) {
        assert(slab != nullptr && "slab must not be null");
        assert(slab->header.neighbors.previous == nullptr && "slab must not have a previous neighbor");
        assert(slab->header.neighbors.next == nullptr && "slab must not have a next neighbor");

        add_memory_segment(slab);
    }

    void* allocate(std::size_t size, const void* = nullptr) {
        const auto matching_bucket_index = required_size_to_sufficient_bucket_index(size);

        if (has_bucket_at_index(matching_bucket_index)) {
            return allocate_from_bucket(matching_bucket_index);
        }

        const auto element_size = required_size_to_element_size(size);
        const auto min_full_slab_index = block_size_to_bucket_index(memory_slab<_slab_size>::data_block_size);
        const auto min_bucket_index = std::max(matching_bucket_index, min_full_slab_index);
        assert(min_bucket_index < _max_buckets && "minimum bucket index out of range");

        if ((_free_segments_mask >> min_bucket_index) == 0) {
            return nullptr;
        }

        const auto bucket_index = std::countr_zero(_free_segments_mask >> min_bucket_index) + min_bucket_index;
        const auto data_block_size = std::max(element_size, 0 + memory_slab<_slab_size>::data_block_size);
        assert(bucket_index < _max_buckets && "bucket index out of range");
        assert(has_bucket_at_index(bucket_index) && "bucket must exist for the given index");

        auto* slab = _free_segments[bucket_index];
        assert(slab != nullptr && "slab should not be null when bucket is occupied");
        assert(slab->is_empty() && "slab must be empty when allocating from it");

        remove_from_free_list(slab);
        split_slab_at_offset(slab, data_block_size + memory_slab<_slab_size>::data_block_offset);

        slab->header.metadata.element_size = element_size;
        slab->set_element(0);

        if (!slab->is_full()) {
            add_to_bucket(slab);
        }

        return slab->get_element(0);
    }

    void deallocate(void* const data, std::size_t = 0) {
        auto* const slab_aligned_ptr = reinterpret_cast<void*>(
            reinterpret_cast<std::uintptr_t>(data) & ~(memory_slab<_slab_size>::memory_slab_alignment - 1));
        auto* const slab = std::launder(reinterpret_cast<memory_slab<_slab_size>*>(slab_aligned_ptr));
        const auto element_size = slab->header.metadata.element_size;
        const auto element_offset = static_cast<std::size_t>(
            reinterpret_cast<std::byte*>(data) - reinterpret_cast<std::byte*>(slab_aligned_ptr)
            ) - memory_slab<_slab_size>::data_block_offset;
        const auto element_index = element_offset / element_size;
        const auto was_full = slab->is_full();
        const auto was_empty = slab->is_empty();

        assert(slab->has_element(element_index) && "element must exist in slab before release");

        slab->clear_element(element_index);

        if (slab->is_empty()) {
            if (!was_full) {
                remove_from_free_list(slab);
            }
            slab->header.metadata.element_size = std::max(
                slab->header.metadata.element_size,
                0 + memory_slab<_slab_size>::data_block_size
            );
            add_memory_segment(slab);
        }
        else if (was_full) {
            add_to_bucket(slab);
        }
    }

private:
    void add_memory_segment(memory_slab<_slab_size>* const slab) {
        assert(slab->is_empty() && "slab must be empty when added to the manager");
        assert(slab->header.free_list.previous == nullptr && "slab must not have a previous free list element");
        assert(slab->header.free_list.next == nullptr && "slab must not have a next free list element");

        const auto merged_slab = merge_neighbors_into_slab(slab);

        add_to_bucket(merged_slab);
    }

    void* allocate_from_bucket(std::size_t bucket_index) {
        auto* const slab = _free_segments[bucket_index];
        const auto element_index = slab->get_first_free_element();

        assert(!slab->has_element(element_index) && "element must not already exist in slab");
        assert(element_index < slab->max_elements() && "element index must be within slab bounds");

        slab->set_element(element_index);

        if (slab->is_full())
            remove_from_free_list(slab);

        return slab->get_element(element_index);
    }

    void split_slab_at_offset(memory_slab<_slab_size>* slab, std::size_t split_offset) {
        assert(slab != nullptr && "slab must not be null");
        assert(slab->is_empty() && "slab must be empty when splitting");
        assert(split_offset % _slab_size == 0 && "split offset must be aligned to slab size");
        assert(slab->header.free_list.previous == nullptr && "slab must not have a previous free list element");
        assert(slab->header.free_list.next == nullptr && "slab must not have a next free list element");

        if (slab->header.metadata.element_size + memory_slab<_slab_size>::data_block_offset == split_offset) {
            return;
        }

        const auto original_element_size = slab->header.metadata.element_size;

        auto* slab_ptr = reinterpret_cast<std::byte*>(slab);
        auto* remaining_slab_ptr = slab_ptr + split_offset;
        auto* remaining_slab = std::launder(reinterpret_cast<memory_slab<_slab_size>*>(remaining_slab_ptr));

        slab->header.metadata.element_size = split_offset - memory_slab<_slab_size>::data_block_offset;

        remaining_slab->header.metadata.element_size = original_element_size - split_offset;
        remaining_slab->header.metadata.mask = 0;

        remaining_slab->header.neighbors.previous = slab;
        remaining_slab->header.neighbors.next = slab->header.neighbors.next;
        remaining_slab->header.free_list.previous = nullptr;
        remaining_slab->header.free_list.next = nullptr;

        if (slab->header.neighbors.next != nullptr) {
            slab->header.neighbors.next->header.neighbors.previous = remaining_slab;
        }

        slab->header.neighbors.next = remaining_slab;

        add_to_bucket(remaining_slab);
    }

    void add_to_bucket(memory_slab<_slab_size>* slab) {
        assert(slab != nullptr && "slab must not be null");
        assert(slab->header.free_list.previous == nullptr && "slab must not have a previous free list element");
        assert(slab->header.free_list.next == nullptr && "slab must not have a next free list element");

        const auto bucket_index = block_size_to_bucket_index(slab->header.metadata.element_size);
        auto*& bucket = _free_segments[bucket_index];

        if (bucket != nullptr) {
            bucket->header.free_list.previous = slab;
        }

        slab->header.free_list.next = bucket;
        bucket = slab;
        _free_segments_mask |= (1 << bucket_index);
    }

    void remove_from_free_list(memory_slab<_slab_size>* slab) {
        const auto bucket_index = block_size_to_bucket_index(slab->header.metadata.element_size);

        assert(bucket_index < _max_buckets && "bucket index out of range");
        assert(has_bucket_at_index(bucket_index) && "bucket must exist for the given index");

        auto* const prev = slab->header.free_list.previous;
        auto* const next = slab->header.free_list.next;

        if (prev != nullptr) {
            prev->header.free_list.next = next;
        }

        if (next != nullptr) {
            next->header.free_list.previous = prev;
        }

        if (_free_segments[bucket_index] == slab) {
            _free_segments[bucket_index] = next;
        }

        if (next == nullptr && prev == nullptr) {
            _free_segments_mask &= ~(1ull << bucket_index);
        }

        slab->header.free_list.previous = nullptr;
        slab->header.free_list.next = nullptr;
    }

    memory_slab<_slab_size>* merge_neighbors_into_slab(memory_slab<_slab_size>* slab) {
        assert(slab != nullptr && "slab must not be null");
        assert(slab->is_empty() && "slab must be empty when merging neighbors");
        assert(slab->header.free_list.previous == nullptr && "slab must not have a previous free list element");
        assert(slab->header.free_list.next == nullptr && "slab must not have a next free list element");

        auto* const prev = slab->header.neighbors.previous;
        if (prev != nullptr && prev->is_empty()) {
            remove_from_free_list(prev);

            prev->header.metadata.element_size += slab->header.metadata.element_size +
                memory_slab<_slab_size>::data_block_offset;

            prev->header.neighbors.next = slab->header.neighbors.next;
            if (prev->header.neighbors.next != nullptr) {
                prev->header.neighbors.next->header.neighbors.previous = prev;
            }

            slab = prev;
        }

        auto* const next = slab->header.neighbors.next;
        if (next != nullptr && next->is_empty()) {
            remove_from_free_list(next);

            slab->header.metadata.element_size += next->header.metadata.element_size +
                memory_slab<_slab_size>::data_block_offset;

            slab->header.neighbors.next = next->header.neighbors.next;
            if (slab->header.neighbors.next != nullptr) {
                slab->header.neighbors.next->header.neighbors.previous = slab;
            }
        }

        return slab;
    }

    constexpr std::size_t required_size_to_sufficient_bucket_index(const std::size_t size) const {
        return std::bit_width(size - 1);
    }

    constexpr std::size_t required_size_to_element_size(const std::size_t size) const {
        const auto element_size = (1ull << required_size_to_sufficient_bucket_index(size));
        return element_size < memory_slab<_slab_size>::data_block_size
            ? element_size
            : (size + memory_slab<_slab_size>::data_block_offset + _slab_size - 1) / _slab_size * _slab_size -
            memory_slab<_slab_size>::data_block_offset;
    }

    constexpr std::size_t block_size_to_bucket_index(const std::size_t size) const {
        return std::bit_width(size) - 1;
    }

    bool has_bucket_at_index(const std::size_t bucket_index) const {
        return _free_segments_mask & (1ull << bucket_index);
    }

    std::array<memory_slab<_slab_size>*, _max_buckets> _free_segments{};
    std::uint64_t _free_segments_mask{ 0 };

    static_assert(_max_buckets <= sizeof(_free_segments_mask) * 8, "Too many buckets for free segments manager");

    friend class FreeMemoryManagerTest;
};

}