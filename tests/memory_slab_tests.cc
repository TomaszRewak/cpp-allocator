#include "src/memory_slab.h"
#include <gtest/gtest.h>

namespace allocator {

TEST(MemorySlabTest, Allocates) {
    memory_slab<1024> slab{
        .header = {
            .neighbors = {
                .previous = nullptr,
                .next = nullptr
            },
            .free_list = {
                .previous = nullptr,
                .next = nullptr
            },
            .metadata = {
                .total_size = 1024,
                .element_size = 64,
                .element_alignment = 64,
                .mask = 0b0
            }
        }
    };
}

}