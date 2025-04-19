#pragma once

#include <cstddef>
#include <type_traits>
#include <algorithm>

namespace allocator {

const auto min_required_data_block_align = alignof(std::max_align_t);

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
            std::size_t mask;
            bool are_elements_trivially_destructible;
        } metadata;
    } header;

    std::byte padding[sizeof(header) % min_required_data_block_align == 0 ? 0 : min_required_data_block_align - sizeof(header) % min_required_data_block_align];
    std::byte data[_size - sizeof(header) - sizeof(padding)];

    std::size_t max_elements() const {
        return std::max(1ul, sizeof(data) / header.metadata.element_size);
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

    std::size_t get_first_free_element() const {
        return std::countr_one(header.metadata.mask);
    }

    std::byte* get_element(std::size_t index) {
        return data + index * header.metadata.element_size;
    }

    void set_element(std::size_t index) {
        header.metadata.mask |= (1 << index);
    }

    void clear_element(std::size_t index) {
        header.metadata.mask &= ~(1 << index);
    }
};

static_assert(std::alignment_of_v<memory_slab<64>> == 64);
static_assert(std::alignment_of_v<memory_slab<1024>> == 1024);
static_assert(std::is_trivial_v<memory_slab<64>>);

// Compiler-specific sanity checks
static_assert(sizeof(min_required_data_block_align) == 8);
static_assert(sizeof(memory_slab<64>::header) == 56);
static_assert(offsetof(memory_slab<64>, data) == 64);

}