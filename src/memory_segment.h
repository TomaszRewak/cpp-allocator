#pragma once

#include "memory_slab.h"

namespace allocator {

template <std::size_t _slab_count = 1024, std::size_t _slab_size = 1024>
struct memory_segment final {
    std::array<bool, _slab_count> has_header;
    std::array<memory_slab<_slab_size>, _slab_count> slabs;

    memory_segment() {
        std::fill(has_header.begin(), has_header.end(), false);
    }
};

}