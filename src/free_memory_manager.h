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
    void add_memory_segment(memory_slab<_slab_size>* slab) {
        assert(slab->is_empty() && "slab must be empty when added to the manager");

        slab->header.metadata.free_memory_manager = this;
        slab = merge_neighbors_into_slab(slab);

        const auto element_size = slab->header.metadata.element_size;
        const auto bucket_index = block_size_to_bucket_index(element_size);

        assert(bucket_index < _max_buckets && "element size is too large");

        add_to_bucket(slab);
    }

    void* get_memory_block(std::size_t size) {
        const auto min_bucket_index = required_size_to_sufficient_bucket_index(size);

        if ((_free_segments_mask & (1 << min_bucket_index)) != 0) {
            return allocate_from_bucket(min_bucket_index, size);
        }

        if (size <= memory_slab<_slab_size>::data_block_size / 2) {
            const auto min_full_slab_size = std::bit_width(memory_slab<_slab_size>::data_block_size);

            if ((_free_segments_mask >> min_full_slab_size) == 0) {
                return nullptr;
            }

            const auto bucket_index = std::countr_zero(_free_segments_mask >> min_full_slab_size) + min_full_slab_size;
            assert(bucket_index < _max_buckets && "bucket index out of range");

            auto* slab = _free_segments[bucket_index];
            assert(slab != nullptr && "slab should not be null when bucket is occupied");

            remove_from_free_list(slab);
            return create_partitioned_slab(slab, size);
        }
        else {
            if ((_free_segments_mask >> min_bucket_index) == 0) {
                return nullptr;
            }

            const auto bucket_index = std::countr_zero(_free_segments_mask >> min_bucket_index) + min_bucket_index;

            if (bucket_index >= _max_buckets) {
                return nullptr;
            }

            auto* slab = _free_segments[bucket_index];
            if (slab == nullptr) {
                return nullptr;
            }

            remove_from_free_list(slab);

            auto remaining_size = slab->header.metadata.element_size - size;
            if (remaining_size >= (_slab_size + memory_slab<_slab_size>::data_block_offset)) {
                return split_and_allocate(slab, size);
            }

            slab->set_element(0);
            return slab->get_element(0);
        }
    }

    void* allocate_from_bucket(std::size_t bucket_index, std::size_t size) {
        assert(bucket_index < _max_buckets && "bucket index out of range");

        auto* slab = _free_segments[bucket_index];
        assert(slab != nullptr && "slab should not be null when bucket is occupied");

        if (slab->max_elements() > 1) {
            return allocate_from_slab(slab, bucket_index);
        }

        remove_from_free_list(slab);
        slab->set_element(0);
        return slab->get_element(0);
    }

    void* allocate_from_slab(memory_slab<_slab_size>* slab, std::size_t bucket_index) {
        const auto element_index = slab->get_first_free_element();

        if (element_index >= slab->max_elements()) {
            return nullptr;
        }

        slab->set_element(element_index);

        if (slab->is_full()) {
            remove_from_free_list(slab);
        }

        return slab->get_element(element_index);
    }

    void* create_partitioned_slab(memory_slab<_slab_size>* slab, std::size_t request_size) {
        const auto original_slab_size = slab->header.metadata.element_size;

        if (original_slab_size > memory_slab<_slab_size>::data_block_size) {

            split_slab_at_offset(slab, _slab_size);
        }

        const auto bucket_index = required_size_to_sufficient_bucket_index(request_size);
        const std::size_t bucket_element_size = (1ULL << bucket_index);

        slab->header.metadata.element_size = bucket_element_size;
        slab->header.metadata.mask = 0;

        slab->set_element(0);
        assert(!slab->is_full() && "slab should not be full after setting first element");

        if (!slab->is_full()) {
            add_to_bucket(slab);
        }

        return slab->get_element(0);
    }

    void release_memory_block(void* const data) {
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
            if (slab->header.metadata.element_size <= memory_slab<_slab_size>::data_block_size) {
                slab->header.metadata.element_size = memory_slab<_slab_size>::data_block_size;
            }
            add_memory_segment(slab);
        }
        else if (was_full) {
            add_to_bucket(slab);
        }
    }

private:
    void split_slab_at_offset(memory_slab<_slab_size>* slab, std::size_t split_offset) {
        assert(split_offset % _slab_size == 0 && "split offset must be aligned to slab size");
        assert(slab->header.metadata.element_size + memory_slab<_slab_size>::data_block_offset > split_offset &&
            "cannot split slab at its full size");

        const auto original_element_size = slab->header.metadata.element_size;
        const auto remaining_element_size = original_element_size - split_offset;

        auto* slab_ptr = reinterpret_cast<std::byte*>(slab);
        auto* remaining_slab_ptr = slab_ptr + split_offset;
        auto* remaining_slab = std::launder(reinterpret_cast<memory_slab<_slab_size>*>(remaining_slab_ptr));

        slab->header.metadata.element_size = split_offset - memory_slab<_slab_size>::data_block_offset;

        remaining_slab->header.metadata.element_size = remaining_element_size;
        remaining_slab->header.metadata.mask = 0;
        remaining_slab->header.metadata.free_memory_manager = slab->header.metadata.free_memory_manager;

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
        const auto bucket_index = block_size_to_bucket_index(slab->header.metadata.element_size);
        auto*& bucket = _free_segments[bucket_index];

        if (bucket != nullptr) {
            bucket->header.free_list.previous = slab;
        }

        slab->header.free_list.next = bucket;
        slab->header.free_list.previous = nullptr;
        bucket = slab;
        _free_segments_mask |= (1 << bucket_index);
    }

    void remove_from_free_list(memory_slab<_slab_size>* slab) {
        const auto bucket_index = block_size_to_bucket_index(slab->header.metadata.element_size);

        assert(bucket_index < _max_buckets && "bucket index out of range");

        auto* prev = slab->header.free_list.previous;
        auto* next = slab->header.free_list.next;

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
        auto* prev = slab->header.neighbors.previous;
        if (prev && prev->is_empty()) {
            const auto prev_size = prev->header.metadata.element_size;
            const auto prev_bucket = block_size_to_bucket_index(prev_size);

            if (prev->header.free_list.previous != nullptr || prev->header.free_list.next != nullptr) {
                remove_from_free_list(prev);
            }

            prev->header.metadata.element_size += slab->header.metadata.element_size +
                memory_slab<_slab_size>::data_block_offset;

            prev->header.neighbors.next = slab->header.neighbors.next;
            if (prev->header.neighbors.next != nullptr) {
                prev->header.neighbors.next->header.neighbors.previous = prev;
            }

            slab = prev;
        }

        auto* next = slab->header.neighbors.next;
        if (next && next->is_empty()) {
            const auto next_size = next->header.metadata.element_size;

            if (next->header.free_list.previous != nullptr || next->header.free_list.next != nullptr) {
                remove_from_free_list(next);
            }

            slab->header.metadata.element_size += next->header.metadata.element_size +
                memory_slab<_slab_size>::data_block_offset;

            slab->header.neighbors.next = next->header.neighbors.next;
            if (slab->header.neighbors.next != nullptr) {
                slab->header.neighbors.next->header.neighbors.previous = slab;
            }
        }

        return slab;
    }

    void* split_and_allocate(memory_slab<_slab_size>* slab, std::size_t size) {
        const auto original_size = slab->header.metadata.element_size;

        const std::size_t required_size = size;

        const std::size_t allocation_slabs = (size + memory_slab<_slab_size>::data_block_offset + _slab_size - 1) / _slab_size;
        const std::size_t allocation_size = allocation_slabs * _slab_size;

        const std::size_t remaining_size = original_size - allocation_size;

        if (remaining_size < _slab_size) {

            slab->header.metadata.element_size = original_size;
            slab->set_element(0);
            return slab->get_element(0);
        }

        slab->header.metadata.element_size = required_size;

        split_slab_at_offset(slab, allocation_size);

        slab->set_element(0);

        return slab->get_element(0);
    }

    constexpr std::size_t required_size_to_sufficient_bucket_index(const std::size_t size) const {
        return std::bit_width(size - 1);
    }

    constexpr std::size_t block_size_to_bucket_index(const std::size_t size) const {
        return std::bit_width(size) - 1;
    }

    std::array<memory_slab<_slab_size>*, _max_buckets> _free_segments{};
    std::uint64_t _free_segments_mask{ 0 };

    static_assert(_max_buckets <= sizeof(_free_segments_mask) * 8, "Too many buckets for free segments manager");

    friend class FreeMemoryManagerTest;
};

}