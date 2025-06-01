#pragma once

#include <span>
#include <cstdint>
#include <stdexcept>
#include <memory>

#include "types.h"

namespace allocator {

template <std::size_t _size, std::size_t _alignment = alignof(std::max_align_t)>
class alignas(_alignment) in_place_block_allocator final {
public:
    allocation_result allocate_at_least(std::size_t size) {
        if (_allocated)
            throw std::runtime_error("memory expansion is not supported");
        if (size > _size)
            throw std::runtime_error("requested size is too large");

        _allocated = true;
        return { _data.data(), _size };
    }

    void deallocate(std::byte* data) {
        if (data != _data.data())
            throw std::runtime_error("invalid deallocation");

        _allocated = false;
    }

private:
    std::array<std::byte, _size> _data;
    bool _allocated = false;
};

}