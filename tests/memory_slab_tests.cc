#include "src/memory_slab.h"
#include <gtest/gtest.h>

namespace allocator {

TEST(MemorySlabTest, EmptySlab) {
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
                .element_size = 64,
                .mask = 0b0,
                .are_elements_trivially_destructible = true
            }
        }
    };

    ASSERT_EQ(slab.max_elements(), (1024 - 64) / 64);
    ASSERT_TRUE(slab.is_empty());
    ASSERT_FALSE(slab.is_full());
}

}