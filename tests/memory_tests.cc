#include "src/memory.h"
#include <gtest/gtest.h>

namespace allocator {

TEST(MemoryTests, Allocates) {
    memory memory;
    const auto* value = memory.allocate<int32_t>(42);

    ASSERT_EQ(*value, 42);
}

TEST(MemoryTests, AllocatesMultipleObjects) {
    memory memory;
    const auto* value1 = memory.allocate<int32_t>(42);
    const auto* value2 = memory.allocate<int32_t>(43);

    ASSERT_EQ(*value1, 42);
    ASSERT_EQ(*value2, 43);
}

TEST(MemoryTests, ReusesFreedMemory) {
    memory memory;
    const auto* value1 = memory.allocate<int32_t>(42);
    memory.deallocate(value1);
    const auto* value2 = memory.allocate<int32_t>(43);

    ASSERT_EQ(value1, value2);
    ASSERT_EQ(*value2, 43);
}

TEST(MemoryTests, ReusesFreedMemoryGaps) {
    memory memory;
    const auto* value1 = memory.allocate<int32_t>(42);
    const auto* value2 = memory.allocate<int32_t>(43);
    const auto* value3 = memory.allocate<int32_t>(44);
    memory.deallocate(value2);
    const auto* value4 = memory.allocate<int32_t>(45);

    ASSERT_EQ(value2, value4);
    ASSERT_EQ(*value4, 45);
}

TEST(MemoryTests, AllocatesBiggerObjectAfterSmaller) {
    memory memory;
    const auto* value1 = memory.allocate<int32_t>(42);
    const auto* value2 = memory.allocate<int64_t>(43);

    ASSERT_EQ(*value1, 42);
    ASSERT_EQ(*value2, 43);
}

TEST(MemoryTests, AllocatesBiggerObjectAfterFreeingSmaller) {
    memory memory;
    const auto* value1 = memory.allocate<int32_t>(42);
    memory.deallocate(value1);
    const auto* value2 = memory.allocate<int64_t>(43);

    ASSERT_EQ(*value2, 43);
}

}