#pragma once

#include <array>
#include <optional>
#include <limits>

#include "memory_slab.h"

namespace allocator {

template <std::size_t _slab_size = 1024>
class free_memory_manager final {
private:
    static constexpr std::size_t _max_buckets = std::numeric_limits<std::size_t>::digits;

public:
    void add_memory_segment(memory_slab<_slab_size>* const slab) {
        if (slab->is_empty()) {
            // TODO: merge with neighbour
            // slab->header.metadata.free_memory_manager = this;
            throw std::runtime_error("not supported yet");
        }

        const auto element_size = slab->header.metadata.element_size;
        const auto bucket_index = std::bit_width(element_size);

        if (bucket_index >= _max_buckets) {
            throw std::runtime_error("element size is too large");
        }

        auto*& bucket = _free_segments[bucket_index];

        if (bucket != nullptr) {
            bucket->header.free_list.previous = slab;
        }

        slab->header.free_list.next = bucket;
        slab->header.free_list.previous = nullptr;
        bucket = slab;
        _free_segnets_mask |= (1 << bucket_index);
    }

    void* get_memory_block(std::size_t size) {
        const auto min_bucket_index = std::bit_width(size);
        const auto bucket_index = std::countr_one(_free_segnets_mask >> min_bucket_index) + min_bucket_index;
    }

    void release_memory_block(void* const data) {
        auto* const slab_aligned_ptr = data % memory_slab<_slab_size>::memory_slab_alignment;
        auto* const slab = std::launder(reinterpret_cast<memory_slab<_slab_size>*>(slab_aligned_ptr));
        const auto element_size = slab->header.metadata.element_size;
        const auto element_offset = static_cast<std::size_t>(data - slab_aligned_ptr) - memory_slab<_slab_size>::data_block_offset;
        const auto element_index = element_offset / element_size;
        const auto was_full = slab->is_full();

        if (!slab->has_element(element_index)) {
            throw std::runtime_error("trying to release memory that wasn't acquired");
        }

        slab->clear_element(element_index);

        if (was_full) {
            add_memory_segment(slab);
        }
    }

private:
    std::array<memory_slab<_slab_size>*, _max_buckets> _free_segments{};
    std::uint64_t _free_segnets_mask{ 0 };

    static_assert(_max_buckets <= sizeof(_free_segnets_mask) * 8, "Too many buckets for free segments manager");
};

}