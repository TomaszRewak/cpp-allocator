#include "src/memory_slab.h"
#include <gtest/gtest.h>

namespace allocator {

TEST(MemorySlabTest, EmptySlab) {
    memory_slab<1024> slab{
        .header = {
            .neighbors {},
            .free_list {},
            .metadata {
                .element_size = 64,
                .mask = 0b0,
                .full_mask = 0b111111111111111
            }
        }
    };

    ASSERT_EQ(slab.max_elements(), (1024 - 64) / 64);
    ASSERT_TRUE(slab.is_empty());
    ASSERT_FALSE(slab.is_full());
    ASSERT_EQ(slab.get_first_free_element(), 0);
}

TEST(MemorySlabTest, SetsFirstElements) {
    memory_slab<1024> slab{
        .header {
            .neighbors {},
            .free_list {},
            .metadata {
                .element_size = 64,
                .mask = 0b0,
                .full_mask = 0b111111111111111
            }
        }
    };

    slab.set_element(0);

    ASSERT_TRUE(slab.has_element(0));
    ASSERT_FALSE(slab.has_element(2));
    ASSERT_FALSE(slab.is_empty());
    ASSERT_FALSE(slab.is_full());
    ASSERT_EQ(slab.get_first_free_element(), 1);
}

TEST(MemorySlabTest, SetsThirdElement) {
    memory_slab<1024> slab{
        .header {
            .neighbors {},
            .free_list {},
            .metadata {
                .element_size = 64,
                .mask = 0b0,
                .full_mask = 0b111111111111111
            }
        }
    };

    slab.set_element(2);

    ASSERT_FALSE(slab.has_element(0));
    ASSERT_TRUE(slab.has_element(2));
    ASSERT_FALSE(slab.is_empty());
    ASSERT_FALSE(slab.is_full());
    ASSERT_EQ(slab.get_first_free_element(), 0);
}

TEST(MemorySlabTest, SetsAllElements) {
    memory_slab<1024> slab{
        .header {
            .neighbors {},
            .free_list {},
            .metadata {
                .element_size = 64,
                .mask = 0b0,
                .full_mask = 0b111111111111111
            }
        }
    };

    for (std::size_t i = 0; i < slab.max_elements(); ++i) {
        slab.set_element(i);
    }

    ASSERT_FALSE(slab.is_empty());
    ASSERT_TRUE(slab.is_full());
}

TEST(MemorySlabTest, ClearsFirstElement) {
    memory_slab<1024> slab{
        .header {
            .neighbors {},
            .free_list {},
            .metadata {
                .element_size = 64,
                .mask = 0b0,
                .full_mask = 0b111111111111111
            }
        }
    };

    slab.set_element(0);
    slab.clear_element(0);

    ASSERT_FALSE(slab.has_element(0));
    ASSERT_TRUE(slab.is_empty());
    ASSERT_FALSE(slab.is_full());
    ASSERT_EQ(slab.get_first_free_element(), 0);
}

TEST(MemorySlabTest, ClearsOneElementOfFullSlab) {
    memory_slab<1024> slab{
        .header {
            .neighbors {},
            .free_list {},
            .metadata {
                .element_size = 64,
                .mask = 0b0,
                .full_mask = 0b111111111111111
            }
        }
    };

    for (std::size_t i = 0; i < slab.max_elements(); ++i) {
        slab.set_element(i);
    }

    ASSERT_TRUE(slab.is_full());

    slab.clear_element(5);

    ASSERT_FALSE(slab.is_empty());
    ASSERT_FALSE(slab.is_full());
    ASSERT_EQ(slab.get_first_free_element(), 5);
}

TEST(MemorySlabTest, GettingEmptyElementDoesNotChangeMask) {
    memory_slab<1024> slab{
        .header {
            .neighbors {},
            .free_list {},
            .metadata {
                .element_size = 64,
                .mask = 0b0,
                .full_mask = 0b111111111111111
            }
        }
    };

    slab.get_element(0);

    ASSERT_EQ(slab.get_first_free_element(), 0);
}

TEST(MemorySlabTest, ReturnsNextFreeElement) {
    memory_slab<1024> slab{
        .header {
            .neighbors {},
            .free_list {},
            .metadata {
                .element_size = 64,
                .mask = 0b0,
                .full_mask = 0b111111111111111
            }
        }
    };

    slab.set_element(0);
    slab.set_element(1);
    slab.set_element(3);

    ASSERT_EQ(slab.get_first_free_element(), 2);
}

}