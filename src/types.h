#pragma once

#include <cstdint>
#include <cstddef>

namespace allocator {

struct allocation_result {
    std::byte* ptr;
    std::size_t count;
};

}