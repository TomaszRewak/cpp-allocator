#pragma once

namespace allocator {

template <std::size_t _slab_size = 1024>
struct alignas(_slab_size) memory_slab final {
}

}