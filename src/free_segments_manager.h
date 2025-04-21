#pragma once

#include <array>
#include <optional>
#include <limits>

#include "memory_slab.h"

namespace allocator {

template <std::size_t _size = 1024>
class free_segments_manager final {
private:
    static constexpr std::size_t _max_buckets = std::numeric_limits<std::size_t>::digits;

public:

private:
    std::array<memory_slab<_size>*, _max_buckets> _free_segments{};
    std::uint64_t _free_segnets_mask{ 0 };

    static_assert(_max_buckets <= sizeof(_free_segnets_mask) * 8, "Too many buckets for free segments manager");
};

}