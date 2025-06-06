## About

This repository contains a C++ library that implements a custom memory allocator based on memory slabs for small object allocation optimization.

Please note that this is just a for-fun side project and I would not recommend using it in production code.

## Memory layout and management

The full pre-allocated memory segment (used as a memory pool) is divided into fixed-size slabs. Each slab starts with a header (containing the slab metadata) and follows strict alignment rules.

A single slab can contain multiple small objects or can be merged with neighboring slabs to host a larger block of data.

Dividing memory into slabs allows for efficient allocations and deallocations (memory operations are performed in O(1) time) and reduces the memory footprint when allocating many small objects (as individual objects do not store any additional metadata). This comes at the expense of an additional memory overhead when allocating larger objects that do not align with the slab boundaries (as the remaining space in the last slab will be wasted).

Each slab header contains the following fields:
- `element_size` - size of the object allocated in this slab (it can be smaller than the slab size if the slab is used to store multiple small objects or larger than the slab size if the slab spans across multiple neighboring slabs)
- `mask` - bitmask of allocated objects in the slab (each bit corresponds to an object in the slab at the position of the bit)
- `previous/next_slab` - pointers to the neighboring slabs (used to merge slabs when deallocating objects)
- `previous/next_free_slab` - pointers forming a linked list of free slabs of similar sizes (used for free slab management/lookup)

The `free_memory_manager` class acts as a memory pool manager and is responsible for organizing, finding, and releasing slabs. It maintains a list of free slabs categorized by their sizes, allowing for quick lookup. Each bucket contains a linked list of free slabs of sizes representing consecutive powers of two (1, 2, 4, 8, 16, ...). Additionally, the `free_memory_manager` maintains a bitmask of occupied buckets to quickly find the next available slab of a suitable size (to find a bucket it uses the `std::countr_zero` which, depending on the CPU architecture, can be optimized into a single instruction, making the lookup very fast).

The process of allocating objects is as follows:

```
 │   1. Calculate the element size based on the requested allocation size.
 │   2. Calculate the matching bucket index based on the element size.
 ├┐  3. Check if there is a free slab in the matching bucket. If not, go to 7.
 ││  4. Allocate the object in the slab and update its mask.
 ││  5. If the slab is now full, remove it from the free slab list.
 │└  6. Return a pointer to the allocated object.
 │   7. Find the next empty slab of a suitable size (only fully empty slabs are considered).
 │   8. Remove the slab from the free list.
 ├┐  9. Divide the slab into two parts (if needed) so that the first part matches the required size.
 ││ 10. Update the headers of the second slab (both the element_size and the previous/next_slab fields)
 ││ 11. Add the second slab into the free list.
 ├┘ 12. Update the slab header.
 │  13. Allocate the object in the first slab and update its mask.
 │  14. If the slab is not full, add it to the free slab list.
 └─ 15. Return a pointer to the allocated object.
```

As you can see the fast path of the allocation process is when allocating small objects of similar sizes, as the allocator can quickly reuse the existing slabs without needing to split them. Nevertheless, even the slow path is performed in O(1) time - though it may require more CPU cycles to complete.

The deallocation process heavily relies on proper slab alignment. The slab alignment is always equal to the slab size. This means that any pointer pointing into the memory region inside the slab can be easily converted into a pointer to the slab header by using a simple modulo operation. Thanks to this, individual objects do not require any additional metadata to be stored alongside them, which reduces the memory footprint when allocating many small objects. After obtaining the slab header, it is possible to determine the element size and its index within the slab - making it possible to update the slab mask freeing the memory.

The process of deallocating objects is as follows:

```
 │   1. Calculate the slab header address based on the pointer to the object.
 │   2. Calculate the object index within the slab based on the header metadata.
 │   3. Update the slab mask to mark the object as free.
 ├─  4. If the slab is not empty yet, return.
 │   5. Update the slab element size in case it was previously storing multiple small objects.
 ├┐  6. If the slab has fully empty neighboring slabs, merge them into a single slab, otherwise go to 10.
 ││  7. Remove empty neighboring slabs from respective free slab lists.
 ││  8. Merge the neighboring slabs into a single slab.
 ││  9. Update the slab header to reflect the new size.
 ├┘ 10. Add the slab to the free slab list of the appropriate size.
 └─ 11. Return.
```

The entire process is visualized in the following diagram (click to enlarge):

<img src="https://github.com/TomaszRewak/cpp-allocator/blob/master/resources/diagram.png?raw=true" alt="Slab Allocator Diagram" width="1200">

## Usage

The recommended way to use the allocator is to interact with the `free_memory_manager` class directly. It provides a simple interface for allocating and deallocating memory, as well as managing slabs.

When using the `free_memory_manager`, you remain in control of when and how the slabs are allocated. It's even possible to place them on the stack.

```cpp

#include "src/free_memory_manager.h"
#include "src/utils.h"

int main() {
    // create a memory manager capable of managing slabs of size 1024 bytes
    allocator::free_memory_manager<1024> manager;

    // create 10 slabs of size 1024 bytes each
    allocator::memory_slab<1024> slabs[10];

    // initialize the slabs (set the slab header metadata)
    launder_slab(slabs, 10);

    // add slabs to the memory manager (this can be done multiple times)
    manager.add_new_memory_segment(slabs);

    // allocate 8 bytes of memory
    void* ptr = manager.allocate(8);

    // use the allocated memory
    int* my_int = std::launder(reinterpret_cast<int*>(ptr));
    *my_int = 10;

    // Deallocate the integer
    manager.deallocate(my_int);

    return 0;
}
```

The `free_memory_manager<slab_size>` provides following methods:
- `void free_memory_manager<slab_size>::add_new_memory_segment(memory_slab<slab_size>* slabs)` - adds a new memory segment to the manager. The slabs must be initialized using the `launder_slab` function before being added.
- `void* free_memory_manager<slab_size>::allocate(size_t size)` - allocates memory of the requested size.
- `void free_memory_manager<slab_size>::deallocate(void* ptr)` - deallocates the memory previously acquired using the `allocate` method.

Alternatively one can use the `allocator::memory` class, which is a thin templated wrapper around the `free_memory_manager` class which allows for automating the process of slab allocation and object initialization.

```cpp

#include "src/memory.h"

int main() {
    // create a memory manager capable of managing slabs of size 1024 bytes
    allocator::memory<allocator::in_place_block_allocator<10 * 1024, 1024>, 1024> memory;

    // allocate and initialize a custom object
    auto* my_object = memory.allocate<MyObject>(42, "Hello World");

    // use the allocated object
    my_object->doSomething();

    // deallocate the object
    memory.deallocate(my_object);
}

```

## Configuration

When using the `allocator`, the most important configuration parameter is the slab size. You should choose it based on the expected size of the objects you will be allocating.

There are only two limitations:
1. The slab size must be a power of two. This is a requirement imposed by the c++ memory alignment rules.
2. The slab size must be at least 64 bytes as the header alone takes 48 bytes of memory.

Anything else is up for grabs.

Choosing a very small value for the slab size will potentially diminish the benefits of this memory model. Each bigger object will require a header of its own and will not be able to reuse the existing slabs (making the slab management process more expensive).

Choosing a very large value for the slab size will potentially waste a lot of memory when allocating small objects. A single slab can store no more than 64 elements. If you choose a slab size of 1024 bytes but mainly allocate 1-byte values, you will waste 912 bytes of memory per slab.

So the choice depends on the actual usage scenario. The rule of thumb should be to choose a slab size that is equal to the average size of the objects you will be allocating multiplied by 64 (as the slab can store up to 64 elements of the same size). This way you will be able to reuse the existing slabs and minimize the memory overhead.

## Final notes

As mentioned before, this is a for-fun side project and I would not recommend using it in production code. It is not battle-tested and may contain bugs or performance issues. Use it at your own risk or for educational purposes only.
