#pragma once

#include "memory_slab.h"

namespace allocator {

template <std::size_t _slab_size>
void launder_slab(memory_slab<_slab_size>* slab, const std::size_t slab_count) {
    auto* aligned_slab = std::launder(slab);
    aligned_slab->header.metadata.mask = 0;
    aligned_slab->header.metadata.full_mask = 1;
    aligned_slab->header.metadata.element_size = slab_count * _slab_size - memory_slab<_slab_size>::data_block_offset;
    aligned_slab->header.neighbors.previous = nullptr;
    aligned_slab->header.neighbors.next = nullptr;
    aligned_slab->header.free_list.previous = nullptr;
    aligned_slab->header.free_list.next = nullptr;
}

}