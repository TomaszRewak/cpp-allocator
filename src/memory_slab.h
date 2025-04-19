#pragma once

#include <cstddef>
#include <type_traits>
#include <algorithm>

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
            std::size_t element_size;
            std::size_t data_offset;
            std::size_t mask;
        } metadata;
    } header;

    std::byte padding[64 - sizeof(header)];
    std::byte data[_size - sizeof(header) - sizeof(padding)];

    std::size_t max_elements() const {
        const auto max_data_size = sizeof(data) - header.metadata.data_offset;
        return std::max(1ul, max_data_size / header.metadata.element_size);
    }

    bool is_empty() const {
        return header.metadata.mask == 0;
    }

    bool is_full() const {
        return header.metadata.mask == (1 << max_elements()) - 1;
    }

    bool has_element(std::size_t index) const {
        return header.metadata.mask & (1 << index);
    }

    std::byte* get_element(std::size_t index) {
        return data + header.metadata.data_offset + index * header.metadata.element_size;
    }
};

static_assert(sizeof(memory_slab<64>::header) == 56);
static_assert(offsetof(memory_slab<64>, data) == 64);
static_assert(std::alignment_of_v<memory_slab<64>> == 64);
static_assert(std::alignment_of_v<memory_slab<1024>> == 1024);

}