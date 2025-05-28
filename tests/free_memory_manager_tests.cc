#include "src/free_memory_manager.h"
#include "src/memory_slab.h"
#include <gtest/gtest.h>
#include <cstdint>
#include <vector>
#include <set>

namespace allocator {

class FreeMemoryManagerTest : public ::testing::Test {
protected:
    template <std::size_t _slab_size>
    void launder_slab(memory_slab<_slab_size>* slab, const std::size_t span) {
        auto* aligned_slab = std::launder(slab);
        aligned_slab->header.metadata.free_memory_manager = nullptr;
        aligned_slab->header.metadata.mask = 0;
        aligned_slab->header.metadata.element_size = span * _slab_size - memory_slab<_slab_size>::data_block_offset;
        aligned_slab->header.neighbors.previous = nullptr;
        aligned_slab->header.neighbors.next = nullptr;
        aligned_slab->header.free_list.previous = nullptr;
        aligned_slab->header.free_list.next = nullptr;

        free_memory_manager<256> manager;
        manager._free_segments_mask = 2;
    }

    template <std::size_t _slab_size, typename... _sizes>
    void ASSERT_MASK_EQ(const free_memory_manager<_slab_size>& manager, _sizes... sizes) {
        ASSERT_EQ(manager._free_segments_mask, ((1ull << manager.block_size_to_bucket_index(sizes)) | ... | 0));
    }

    template <std::size_t _slab_size>
    void ASSERT_BUCKET_EQ(const free_memory_manager<_slab_size>& manager, std::size_t size, const memory_slab<_slab_size>* slab) {
        ASSERT_EQ(manager._free_segments[manager.block_size_to_bucket_index(size)], slab);
    }

    template <std::size_t _slab_size>
    void ASSERT_BUCKET_EQ(const free_memory_manager<_slab_size>& manager, std::size_t size, nullptr_t) {
        ASSERT_EQ(manager._free_segments[manager.block_size_to_bucket_index(size)], nullptr);
    }

    template <std::size_t _slab_size>
    void ASSERT_IS_IN_SLAB(void* ptr, memory_slab<_slab_size>* slab) {
        ASSERT_EQ(reinterpret_cast<std::uintptr_t>(ptr) / _slab_size * _slab_size, reinterpret_cast<std::uintptr_t>(slab));
    }
};

// Basic functionality tests
TEST_F(FreeMemoryManagerTest, AddEmptySlab) {
    memory_slab<256> slabs[10];
    launder_slab(slabs, 10);

    free_memory_manager<256> manager;
    manager.add_new_memory_segment(slabs);

    ASSERT_MASK_EQ(manager, 256 * 10 - memory_slab<256>::data_block_offset);
    ASSERT_BUCKET_EQ(manager, 256 * 10 - memory_slab<256>::data_block_offset, slabs);
    ASSERT_EQ(slabs[0].header.metadata.element_size, 256 * 10 - memory_slab<256>::data_block_offset);
    ASSERT_EQ(slabs[0].header.metadata.free_memory_manager, &manager);
    ASSERT_EQ(slabs[0].header.neighbors.previous, nullptr);
    ASSERT_EQ(slabs[0].header.neighbors.next, nullptr);
    ASSERT_EQ(slabs[0].header.free_list.previous, nullptr);
    ASSERT_EQ(slabs[0].header.free_list.next, nullptr);
}

TEST_F(FreeMemoryManagerTest, AllocateSmallElement) {
    memory_slab<256> slabs[10];
    launder_slab(slabs, 10);

    free_memory_manager<256> manager;
    manager.add_new_memory_segment(slabs);
    void* ptr = manager.get_memory_block(8);

    ASSERT_NE(ptr, nullptr);
    ASSERT_IS_IN_SLAB(ptr, &slabs[0]);
    ASSERT_MASK_EQ(manager, 8, 256 * 9 - memory_slab<256>::data_block_offset);
    ASSERT_BUCKET_EQ(manager, 8, &slabs[0]);
    ASSERT_BUCKET_EQ(manager, 256 * 9 - memory_slab<256>::data_block_offset, &slabs[1]);
    ASSERT_EQ(slabs[0].header.metadata.free_memory_manager, &manager);
    ASSERT_EQ(slabs[1].header.metadata.free_memory_manager, &manager);
    ASSERT_TRUE(slabs[0].has_element(0));
    ASSERT_FALSE(slabs[0].is_full());
    ASSERT_TRUE(slabs[1].is_empty());
    ASSERT_EQ(slabs[0].header.metadata.element_size, 8);
    ASSERT_EQ(slabs[1].header.metadata.element_size, 256 * 9 - memory_slab<256>::data_block_offset);
    ASSERT_EQ(slabs[0].header.metadata.mask, 1);
    ASSERT_EQ(slabs[0].header.neighbors.previous, nullptr);
    ASSERT_EQ(slabs[0].header.neighbors.next, &slabs[1]);
    ASSERT_EQ(slabs[1].header.neighbors.previous, &slabs[0]);
    ASSERT_EQ(slabs[1].header.neighbors.next, nullptr);
    ASSERT_EQ(slabs[0].header.free_list.previous, nullptr);
    ASSERT_EQ(slabs[0].header.free_list.next, nullptr);
    ASSERT_EQ(slabs[1].header.free_list.previous, nullptr);
    ASSERT_EQ(slabs[1].header.free_list.next, nullptr);
}

TEST_F(FreeMemoryManagerTest, AllocateMultipleSmallElementsInSameSlab) {
    memory_slab<256> slabs[10];
    launder_slab(slabs, 10);

    free_memory_manager<256> manager;
    manager.add_new_memory_segment(slabs);

    void* ptr1 = manager.get_memory_block(5);
    void* ptr2 = manager.get_memory_block(6);
    void* ptr3 = manager.get_memory_block(7);

    ASSERT_NE(ptr1, nullptr);
    ASSERT_NE(ptr2, nullptr);
    ASSERT_NE(ptr3, nullptr);
    ASSERT_IS_IN_SLAB(ptr1, &slabs[0]);
    ASSERT_IS_IN_SLAB(ptr2, &slabs[0]);
    ASSERT_IS_IN_SLAB(ptr3, &slabs[0]);
    ASSERT_MASK_EQ(manager, 8, 256 * 9 - memory_slab<256>::data_block_offset);
    ASSERT_BUCKET_EQ(manager, 8, &slabs[0]);
    ASSERT_BUCKET_EQ(manager, 256 * 9 - memory_slab<256>::data_block_offset, &slabs[1]);
    ASSERT_TRUE(slabs[0].has_element(0));
    ASSERT_TRUE(slabs[0].has_element(1));
    ASSERT_TRUE(slabs[0].has_element(2));
    ASSERT_FALSE(slabs[0].has_element(3));
    ASSERT_FALSE(slabs[0].is_full());
}

TEST_F(FreeMemoryManagerTest, AllocateSmallElementsInDifferentSlabs) {
    memory_slab<256> slabs[10];
    launder_slab(slabs, 10);

    free_memory_manager<256> manager;
    manager.add_new_memory_segment(slabs);

    void* ptr1 = manager.get_memory_block(4);
    void* ptr2 = manager.get_memory_block(5);

    ASSERT_NE(ptr1, nullptr);
    ASSERT_NE(ptr2, nullptr);
    ASSERT_IS_IN_SLAB(ptr1, &slabs[0]);
    ASSERT_IS_IN_SLAB(ptr2, &slabs[1]);
    ASSERT_MASK_EQ(manager, 4, 8, 256 * 8 - memory_slab<256>::data_block_offset);
    ASSERT_BUCKET_EQ(manager, 4, &slabs[0]);
    ASSERT_BUCKET_EQ(manager, 8, &slabs[1]);
    ASSERT_BUCKET_EQ(manager, 256 * 8 - memory_slab<256>::data_block_offset, &slabs[2]);
    ASSERT_TRUE(slabs[0].has_element(0));
    ASSERT_TRUE(slabs[1].has_element(0));
}

TEST_F(FreeMemoryManagerTest, FillSlabWithSmallElements) {
    memory_slab<256> slabs[10];
    launder_slab(slabs, 10);

    free_memory_manager<256> manager;
    manager.add_new_memory_segment(slabs);

    do {
        void* ptr = manager.get_memory_block(8);
        ASSERT_NE(ptr, nullptr);
        ASSERT_IS_IN_SLAB(ptr, &slabs[0]);
    } while (!slabs[0].is_full());

    ASSERT_MASK_EQ(manager, 256 * 9 - memory_slab<256>::data_block_offset);
    ASSERT_BUCKET_EQ(manager, 8, nullptr);
    ASSERT_BUCKET_EQ(manager, 256 * 9 - memory_slab<256>::data_block_offset, &slabs[1]);
    ASSERT_TRUE(slabs[0].is_full());
}

TEST_F(FreeMemoryManagerTest, FillSlabWithSmallElementsAndThenAddMore) {
    memory_slab<256> slabs[10];
    launder_slab(slabs, 10);

    free_memory_manager<256> manager;
    manager.add_new_memory_segment(slabs);

    do {
        void* ptr = manager.get_memory_block(8);
        ASSERT_NE(ptr, nullptr);
        ASSERT_IS_IN_SLAB(ptr, &slabs[0]);
    } while (!slabs[0].is_full());

    void* ptr2 = manager.get_memory_block(8);

    ASSERT_NE(ptr2, nullptr);
    ASSERT_IS_IN_SLAB(ptr2, &slabs[1]);
    ASSERT_MASK_EQ(manager, 8, 256 * 8 - memory_slab<256>::data_block_offset);
    ASSERT_BUCKET_EQ(manager, 8, &slabs[1]);
    ASSERT_BUCKET_EQ(manager, 256 * 8 - memory_slab<256>::data_block_offset, &slabs[2]);
    ASSERT_TRUE(slabs[0].is_full());
    ASSERT_FALSE(slabs[1].is_full());
    ASSERT_TRUE(slabs[1].has_element(0));
    ASSERT_FALSE(slabs[1].has_element(1));
    ASSERT_EQ(slabs[0].header.metadata.element_size, 8);
    ASSERT_EQ(slabs[1].header.metadata.element_size, 8);
    ASSERT_EQ(slabs[0].header.neighbors.previous, nullptr);
    ASSERT_EQ(slabs[0].header.neighbors.next, &slabs[1]);
    ASSERT_EQ(slabs[1].header.neighbors.previous, &slabs[0]);
    ASSERT_EQ(slabs[1].header.neighbors.next, &slabs[2]);
    ASSERT_EQ(slabs[2].header.neighbors.previous, &slabs[1]);
    ASSERT_EQ(slabs[2].header.neighbors.next, nullptr);
    ASSERT_EQ(slabs[0].header.free_list.previous, nullptr);
    ASSERT_EQ(slabs[0].header.free_list.next, nullptr);
    ASSERT_EQ(slabs[1].header.free_list.previous, nullptr);
    ASSERT_EQ(slabs[1].header.free_list.next, nullptr);
    ASSERT_EQ(slabs[2].header.free_list.previous, nullptr);
    ASSERT_EQ(slabs[2].header.free_list.next, nullptr);
}

TEST_F(FreeMemoryManagerTest, RemoveOneOfTheSmallElements) {
    memory_slab<256> slabs[10];
    launder_slab(slabs, 10);

    free_memory_manager<256> manager;
    manager.add_new_memory_segment(slabs);

    void* ptr1 = manager.get_memory_block(8);
    void* ptr2 = manager.get_memory_block(8);
    void* ptr3 = manager.get_memory_block(8);
    manager.release_memory_block(ptr2);

    ASSERT_MASK_EQ(manager, 8, 256 * 9 - memory_slab<256>::data_block_offset);
    ASSERT_BUCKET_EQ(manager, 8, &slabs[0]);
    ASSERT_TRUE(slabs[0].has_element(0));
    ASSERT_FALSE(slabs[0].has_element(1));
    ASSERT_TRUE(slabs[0].has_element(2));
}

TEST_F(FreeMemoryManagerTest, RemoveLastSmallElement) {
    memory_slab<256> slabs[10];
    launder_slab(slabs, 10);

    free_memory_manager<256> manager;
    manager.add_new_memory_segment(slabs);

    void* ptr = manager.get_memory_block(8);
    manager.release_memory_block(ptr);

    ASSERT_MASK_EQ(manager, 256 * 10 - memory_slab<256>::data_block_offset);
    ASSERT_BUCKET_EQ(manager, 8, nullptr);
    ASSERT_TRUE(slabs[0].is_empty());
    ASSERT_EQ(slabs[0].header.metadata.element_size, 256 * 10 - memory_slab<256>::data_block_offset);
    ASSERT_EQ(slabs[0].header.metadata.free_memory_manager, &manager);
    ASSERT_EQ(slabs[0].header.neighbors.previous, nullptr);
    ASSERT_EQ(slabs[0].header.neighbors.next, nullptr);
    ASSERT_EQ(slabs[0].header.free_list.previous, nullptr);
    ASSERT_EQ(slabs[0].header.free_list.next, nullptr);
}

TEST_F(FreeMemoryManagerTest, AllSlabsHoldTheSameNumberOfElements) {
    memory_slab<256> slabs[10];
    launder_slab(slabs, 10);

    free_memory_manager<256> manager;
    manager.add_new_memory_segment(slabs);

    std::array<std::size_t, 10> sizes;
    std::fill(sizes.begin(), sizes.end(), 0);

    for (std::size_t i = 0; i < 8; ++i) {
        do {
            void* ptr = manager.get_memory_block(8);
            ASSERT_NE(ptr, nullptr);
            ASSERT_IS_IN_SLAB(ptr, &slabs[i]);
            ++sizes[i];
        } while (!slabs[i].is_full());
    }

    for (std::size_t i = 0; i < 8; ++i) {
        ASSERT_EQ(slabs[i].header.metadata.element_size, 8);
        ASSERT_EQ(sizes[i], slabs[i].max_elements());
        ASSERT_EQ(sizes[i], sizes[0]);
        ASSERT_TRUE(slabs[i].is_full());
    }
}

TEST_F(FreeMemoryManagerTest, AllocateFewSlabsWithSmallElementsAndThenReleaseFewOfThem) {
    memory_slab<256> slabs[10];
    launder_slab(slabs, 10);

    free_memory_manager<256> manager;
    manager.add_new_memory_segment(slabs);

    std::vector<void*> slab_1_ptrs;
    do {
        slab_1_ptrs.push_back(manager.get_memory_block(8));
    } while (!slabs[0].is_full());

    std::vector<void*> slab_2_ptrs;
    do {
        slab_2_ptrs.push_back(manager.get_memory_block(8));
    } while (!slabs[1].is_full());

    std::vector<void*> slab_3_ptrs;
    do {
        slab_3_ptrs.push_back(manager.get_memory_block(8));
    } while (!slabs[2].is_full());

    void* const ptr4 = manager.get_memory_block(8);

    for (auto ptr : slab_1_ptrs) {
        ASSERT_IS_IN_SLAB(ptr, &slabs[0]);
        manager.release_memory_block(ptr);
    }
    for (auto ptr : slab_3_ptrs) {
        ASSERT_IS_IN_SLAB(ptr, &slabs[2]);
        manager.release_memory_block(ptr);
    }

    ASSERT_EQ(slabs[0].header.metadata.element_size, 256 - memory_slab<256>::data_block_offset);
    ASSERT_EQ(slabs[1].header.metadata.element_size, 8);
    ASSERT_EQ(slabs[2].header.metadata.element_size, 256 - memory_slab<256>::data_block_offset);
    ASSERT_EQ(slabs[3].header.metadata.element_size, 8);
    ASSERT_EQ(slabs[4].header.metadata.element_size, 256 * 6 - memory_slab<256>::data_block_offset);
    ASSERT_MASK_EQ(manager, 8, 256 - memory_slab<256>::data_block_offset, 256 * 6 - memory_slab<256>::data_block_offset);
    ASSERT_BUCKET_EQ(manager, 8, &slabs[3]);
    ASSERT_BUCKET_EQ(manager, 256 - memory_slab<256>::data_block_offset, &slabs[2]);
    ASSERT_BUCKET_EQ(manager, 256 * 6 - memory_slab<256>::data_block_offset, &slabs[4]);
    ASSERT_TRUE(slabs[0].is_empty());
    ASSERT_TRUE(slabs[1].is_full());
    ASSERT_TRUE(slabs[2].is_empty());
    ASSERT_FALSE(slabs[3].is_empty());
    ASSERT_FALSE(slabs[3].is_full());
    ASSERT_TRUE(slabs[4].is_empty());
    ASSERT_EQ(slabs[0].header.neighbors.previous, nullptr);
    ASSERT_EQ(slabs[0].header.neighbors.next, &slabs[1]);
    ASSERT_EQ(slabs[1].header.neighbors.previous, &slabs[0]);
    ASSERT_EQ(slabs[1].header.neighbors.next, &slabs[2]);
    ASSERT_EQ(slabs[2].header.neighbors.previous, &slabs[1]);
    ASSERT_EQ(slabs[2].header.neighbors.next, &slabs[3]);
    ASSERT_EQ(slabs[3].header.neighbors.previous, &slabs[2]);
    ASSERT_EQ(slabs[3].header.neighbors.next, &slabs[4]);
    ASSERT_EQ(slabs[4].header.neighbors.previous, &slabs[3]);
    ASSERT_EQ(slabs[4].header.neighbors.next, nullptr);
    ASSERT_EQ(slabs[2].header.free_list.previous, nullptr);
    ASSERT_EQ(slabs[2].header.free_list.next, &slabs[0]);
    ASSERT_EQ(slabs[0].header.free_list.previous, &slabs[2]);
    ASSERT_EQ(slabs[0].header.free_list.next, nullptr);

    for (auto ptr : slab_2_ptrs) {
        ASSERT_IS_IN_SLAB(ptr, &slabs[1]);
        manager.release_memory_block(ptr);
    }

    ASSERT_TRUE(slabs[1].is_empty());

    ASSERT_EQ(slabs[0].header.metadata.element_size, 256 * 3 - memory_slab<256>::data_block_offset);
    ASSERT_EQ(slabs[3].header.metadata.element_size, 8);
    ASSERT_EQ(slabs[4].header.metadata.element_size, 256 * 6 - memory_slab<256>::data_block_offset);
    ASSERT_EQ(slabs[0].header.neighbors.previous, nullptr);
    ASSERT_EQ(slabs[0].header.neighbors.next, &slabs[3]);
    ASSERT_EQ(slabs[3].header.neighbors.previous, &slabs[0]);
    ASSERT_EQ(slabs[3].header.neighbors.next, &slabs[4]);
    ASSERT_EQ(slabs[4].header.neighbors.previous, &slabs[3]);
    ASSERT_EQ(slabs[4].header.neighbors.next, nullptr);
    ASSERT_MASK_EQ(manager, 8, 256 * 3 - memory_slab<256>::data_block_offset, 256 * 6 - memory_slab<256>::data_block_offset);
    ASSERT_BUCKET_EQ(manager, 8, &slabs[3]);
    ASSERT_BUCKET_EQ(manager, 256 * 3 - memory_slab<256>::data_block_offset, &slabs[0]);
    ASSERT_BUCKET_EQ(manager, 256 * 6 - memory_slab<256>::data_block_offset, &slabs[4]);
}

}
