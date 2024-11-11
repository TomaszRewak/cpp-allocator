#include "src/allocator.h"
#include <gtest/gtest.h>

namespace allocator {

TEST(AllocatorTest, Add) {
    EXPECT_EQ(3, add(1, 2));
}

}