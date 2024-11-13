#pragma once

#include <span>

namespace allocator {

template <std::size_t Size>
class memory final {
private:
    using pointer_type = uint64_t; // TODO: determine it based on the size of the memory
    using size_type = uint64_t; // TODO: determine it based on the size of the memory

    struct allocation_header final {
        pointer_type previous;
        pointer_type next;
        pointer_type previous_free;
        pointer_type next_free;
    };

public:
    std::span<std::byte> allocate(size_t size) {
        const auto data = std::span<std::byte>(_data);
        return data.subspan(0, size);
    }

    template <typename T, typename... Args>
    T* allocate(Args&&... args) {
        const auto allocated = allocate(sizeof(T));
        return new (allocated.data()) T(std::forward<Args>(args)...);
    }

private:
    std::array<allocation*, 2> _nodes;
    std::byte _data[Size];
};

}