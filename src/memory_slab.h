#pragma once

#include <cstddef>
#include <type_traits>

namespace allocator {

template <std::size_t _size = 1024>
struct alignas(_size) memory_slab final {
    struct header final {
        struct neighbors final {
            memory_slab* previous;
            memory_slab* next;
        } neighbors;

        struct free_list final {
            memory_slab* previous;
            memory_slab* next;
        } free_list;

        struct metadata {
            std::size_t total_size;
            std::size_t element_size;
            std::size_t data_offset;
            std::size_t mask;
        } metadata;
    } header;

    std::byte data[0];

    std::size_t max_elements() const {
        const auto max_data_size = _size - offsetof(memory_slab, data) - header.metadata.data_offset;
        return std::max(1, max_data_size / header.metadata.element_size);
    }

    bool is_empty() const {
        return header.metadata.mask == 0;
    }

    bool is_full() const {
        return header.metadata.mask == (1 << max_elements()) - 1;
    }
};

static_assert(std::alignment_of_v<memory_slab<64>> == 64);
static_assert(std::alignment_of_v<memory_slab<1024>> == 1024);

}