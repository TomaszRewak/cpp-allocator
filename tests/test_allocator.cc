#include "src/allocator.h"
#include <gtest/gtest.h>

namespace allocator {

TEST(AllocatorTest, SimpleAllocation) {
    memory<1024> memory;
    const auto* value = memory.allocate<int>(42);

    ASSERT_EQ(*value, 42);
}

TEST(AllocatorTest, TwoSimpleAllocations) {
    memory<1024> memory;
    const auto* value1 = memory.allocate<int>(42);
    const auto* value2 = memory.allocate<int>(43);

    ASSERT_EQ(*value1, 42);
    ASSERT_EQ(*value2, 43);
}

}