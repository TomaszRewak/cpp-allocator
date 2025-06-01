#include "src/memory.h"
#include <gtest/gtest.h>

namespace allocator {

TEST(MemoryTests, Allocates) {
    in_place_memory memory;
    auto* const value = memory.allocate<int32_t>(42);

    ASSERT_EQ(*value, 42);
}

TEST(MemoryTests, AllocatesMultipleObjects) {
    in_place_memory memory;
    auto* const value1 = memory.allocate<int32_t>(42);
    auto* const value2 = memory.allocate<int32_t>(43);

    ASSERT_EQ(*value1, 42);
    ASSERT_EQ(*value2, 43);
}

TEST(MemoryTests, ReusesFreedMemory) {
    in_place_memory memory;
    auto* const value1 = memory.allocate<int32_t>(42);
    memory.deallocate(value1);
    auto* const value2 = memory.allocate<int32_t>(43);

    ASSERT_EQ(value1, value2);
    ASSERT_EQ(*value2, 43);
}

TEST(MemoryTests, ReusesFreedMemoryGaps) {
    in_place_memory memory;
    auto* const value1 = memory.allocate<int32_t>(42);
    auto* const value2 = memory.allocate<int32_t>(43);
    auto* const value3 = memory.allocate<int32_t>(44);
    memory.deallocate(value2);
    auto* const value4 = memory.allocate<int32_t>(45);

    ASSERT_EQ(value2, value4);
    ASSERT_EQ(*value4, 45);
}

TEST(MemoryTests, AllocatesBiggerObjectAfterSmaller) {
    in_place_memory memory;
    auto* const value1 = memory.allocate<int32_t>(42);
    auto* const value2 = memory.allocate<int64_t>(43);

    ASSERT_EQ(*value1, 42);
    ASSERT_EQ(*value2, 43);
}

TEST(MemoryTests, AllocatesBiggerObjectAfterFreeingSmaller) {
    in_place_memory memory;
    auto* const value1 = memory.allocate<int32_t>(42);
    memory.deallocate(value1);
    auto* const value2 = memory.allocate<int64_t>(43);

    ASSERT_EQ(*value2, 43);
}

}