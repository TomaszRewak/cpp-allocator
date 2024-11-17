#pragma once

#include <cstdint>
#include <cstddef>

namespace allocator {

struct block_header final {
    std::byte* next_block;
};

struct segment_header final {
    segment_header* previous_neighbor_segment;
    segment_header* next_neighbor_segment;
    segment_header* previous_free_segment;
    segment_header* next_free_segment;
    std::size_t data_size;
};

struct allocation_result {
    std::byte* ptr;
    std::size_t count;
};

}