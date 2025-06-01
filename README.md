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

1. `│ `Calculate the element size based on the requested allocation size.
2. `│ `Calculate the matching bucket index based on the element size.
3. `├┐`Check if there is a free slab in the matching bucket. If not, go to `7`.
4. `││`Allocate the object in the slab and update its mask.
5. `││`If the slab is now full, remove it from the free slab list.
6. `│└`Return a pointer to the allocated object.
7. `│ `Find the next empty slab of a suitable size (only fully empty slabs are considered - not to mix small objects of different sizes).
8. `│ `Remove the slab from the free list.
9. `│ `Divide the slab into two parts (in case the slab spans across multiple neighboring slabs) so that the first part matches the required size.
10. `│ `Update the headers of both slabs (both the `element_size` and the `previous/next_slab` fields)
11. `│ `Add the second slab into the free list.
12. `│ `Allocate the object in the first slab and update its mask.
13. `│ `If the slab is not full, add it to the free slab list.
14. `└─`Return a pointer to the allocated object.

As you can see the fast path of the allocation process is when allocating small objects of similar sizes, as the allocator can quickly reuse the existing slabs without needing to split them. Nevertheless, even the slow path is performed in O(1) time - though it may require more CPU cycles to complete.

The deallocation process heavily relies on proper slab alignment. The slab alignment is always equal to the slab size. This means that any pointer pointing into the memory region inside the slab can be easily converted into a pointer to the slab header by using a simple modulo operation. Thanks to this, individual objects do not require any additional metadata to be stored alongside them, which reduces the memory footprint when allocating many small objects. After obtaining the slab header, it is possible to determine the element size and its index within the slab - making it possible to update the slab mask freeing the memory.

The process of deallocating objects is as follows:
1. `│ `Calculate the slab header address based on the pointer to the object.
2. `│ `Calculate the slab index within the slab header.
3. `│ `Update the slab mask to mark the object as free.
4. `├─`If the slab is not empty yet, return.
5. `├┐`If the slab has fully empty neighboring slabs, merge them into a single slab, otherwise go to `8`.
6. `││`Remove empty neighboring slabs from the free slab list.
7. `││`Merge the neighboring slabs into a single slab, updated the slab header to reflect the new size.
8. `├┘`Add the slab to the free slab list.
9. `└─`Return.







