#pragma once

#include <cstddef>
#include <type_traits>
#include <algorithm>

namespace allocator {

template <std::size_t _size = 1024>
struct alignas(_size) memory_slab final {
    static_assert((_size& (_size - 1)) == 0, "Memory slab size must be a power of two");

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
            void* free_memory_manager;
            std::size_t element_size;
            std::size_t mask;
        } metadata;
    } header;

    const static auto memory_slab_alignment = _size;
    const static auto min_required_data_block_align = alignof(std::max_align_t);
    const static auto data_block_padding = sizeof(header) % min_required_data_block_align == 0 ? 0 : min_required_data_block_align - sizeof(header) % min_required_data_block_align;
    const static auto data_block_offset = sizeof(header) + data_block_padding;
    const static auto data_block_size = _size - data_block_offset;

    std::byte padding[data_block_padding];
    std::byte data[data_block_size];

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
static_assert(sizeof(memory_slab<128>::min_required_data_block_align) == 8);
static_assert(sizeof(memory_slab<128>::header) == 56);
static_assert(offsetof(memory_slab<128>, data) == 64);

}