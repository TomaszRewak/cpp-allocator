#include "src/free_memory_manager.h"
#include "src/utils.h"
#include <benchmark/benchmark.h>

const std::size_t iterations = 1000;
using mid_size_object = std::array<std::byte, 64>;
using big_size_object = std::array<std::byte, 1024>;

void same_size_small_allocations_with_new(benchmark::State& state) {
    for (auto _ : state) {
        std::array<int*, iterations> int_pointers;

        for (int i = 0; i < iterations; ++i) {
            int* p = new int(i);
            benchmark::DoNotOptimize(p);
            benchmark::DoNotOptimize(*p);
            int_pointers[i] = p;
        }

        for (int i = 0; i < iterations; ++i) {
            benchmark::DoNotOptimize(int_pointers[i]);
            delete int_pointers[i];
        }
    }
}
BENCHMARK(same_size_small_allocations_with_new);

void same_size_small_allocations_with_free_memory_manager(benchmark::State& state) {
    allocator::free_memory_manager<256> manager;
    allocator::memory_slab<256> slabs[100];
    allocator::launder_slab(slabs, 100);
    manager.add_new_memory_segment(slabs);

    for (auto _ : state) {
        std::array<int*, iterations> int_pointers;

        for (int i = 0; i < iterations; ++i) {
            void* raw_ptr = manager.allocate(sizeof(int));
            benchmark::DoNotOptimize(raw_ptr);
            int* ip = std::launder(reinterpret_cast<int*>(raw_ptr));
            ip = new (ip) int(i);
            benchmark::DoNotOptimize(ip);
            benchmark::DoNotOptimize(*ip);
            int_pointers[i] = ip;
        }

        for (int i = 0; i < iterations; ++i) {
            benchmark::DoNotOptimize(int_pointers[i]);
            manager.deallocate(int_pointers[i]);
        }
    }
}
BENCHMARK(same_size_small_allocations_with_free_memory_manager);

void different_size_small_allocations_with_new(benchmark::State& state) {
    for (auto _ : state) {
        std::array<bool*, iterations> bool_pointers;
        std::array<int*, iterations> int_pointers;
        std::array<long long*, iterations> long_pointers;

        for (int i = 0; i < iterations; ++i) {
            bool* bp = new bool(i % 2 == 0);
            benchmark::DoNotOptimize(bp);
            benchmark::DoNotOptimize(*bp);
            bool_pointers[i] = bp;

            int* p = new int(i);
            benchmark::DoNotOptimize(p);
            benchmark::DoNotOptimize(*p);
            int_pointers[i] = p;

            long long* lp = new long long(i);
            benchmark::DoNotOptimize(lp);
            benchmark::DoNotOptimize(*lp);
            long_pointers[i] = lp;
        }

        for (int i = 0; i < iterations; ++i) {
            benchmark::DoNotOptimize(bool_pointers[i]);
            benchmark::DoNotOptimize(int_pointers[i]);
            benchmark::DoNotOptimize(long_pointers[i]);
            delete bool_pointers[i];
            delete int_pointers[i];
            delete long_pointers[i];
        }
    }
}
BENCHMARK(different_size_small_allocations_with_new);

void different_size_small_allocations_with_free_memory_manager(benchmark::State& state) {
    allocator::free_memory_manager<256> manager;
    allocator::memory_slab<256> slabs[200];
    allocator::launder_slab(slabs, 200);
    manager.add_new_memory_segment(slabs);

    for (auto _ : state) {
        std::array<bool*, iterations> bool_pointers;
        std::array<int*, iterations> int_pointers;
        std::array<long long*, iterations> long_pointers;

        for (int i = 0; i < iterations; ++i) {
            void* raw_bp = manager.allocate(sizeof(bool));
            benchmark::DoNotOptimize(raw_bp);
            bool* bp = std::launder(reinterpret_cast<bool*>(raw_bp));
            bp = new (bp) bool(i % 2 == 0);
            benchmark::DoNotOptimize(bp);
            benchmark::DoNotOptimize(*bp);
            bool_pointers[i] = bp;

            void* raw_ip = manager.allocate(sizeof(int));
            benchmark::DoNotOptimize(raw_ip);
            int* ip = std::launder(reinterpret_cast<int*>(raw_ip));
            ip = new (ip) int(i);
            benchmark::DoNotOptimize(ip);
            benchmark::DoNotOptimize(*ip);
            int_pointers[i] = ip;

            void* raw_lp = manager.allocate(sizeof(long long));
            benchmark::DoNotOptimize(raw_lp);
            long long* lp = std::launder(reinterpret_cast<long long*>(raw_lp));
            lp = new (lp) long long(i);
            benchmark::DoNotOptimize(lp);
            benchmark::DoNotOptimize(*lp);
            long_pointers[i] = lp;
        }

        for (int i = 0; i < iterations; ++i) {
            benchmark::DoNotOptimize(bool_pointers[i]);
            benchmark::DoNotOptimize(int_pointers[i]);
            benchmark::DoNotOptimize(long_pointers[i]);
            manager.deallocate(bool_pointers[i]);
            manager.deallocate(int_pointers[i]);
            manager.deallocate(long_pointers[i]);
        }
    }
}
BENCHMARK(different_size_small_allocations_with_free_memory_manager);

void same_size_mid_allocations_with_new(benchmark::State& state) {
    for (auto _ : state) {
        std::array<mid_size_object*, iterations> pointers;

        for (int i = 0; i < iterations; ++i) {
            mid_size_object* p = new mid_size_object();
            benchmark::DoNotOptimize(p);
            benchmark::DoNotOptimize(*p);
            pointers[i] = p;
        }

        for (int i = 0; i < iterations; ++i) {
            benchmark::DoNotOptimize(pointers[i]);
            delete pointers[i];
        }
    }
}
BENCHMARK(same_size_mid_allocations_with_new);

void same_size_mid_allocations_with_free_memory_manager(benchmark::State& state) {
    allocator::free_memory_manager<4096> manager;
    allocator::memory_slab<4096> slabs[100];
    allocator::launder_slab(slabs, 100);
    manager.add_new_memory_segment(slabs);

    for (auto _ : state) {
        std::array<mid_size_object*, iterations> pointers;

        for (int i = 0; i < iterations; ++i) {
            void* raw_ptr = manager.allocate(sizeof(mid_size_object));
            benchmark::DoNotOptimize(raw_ptr);
            mid_size_object* p = std::launder(reinterpret_cast<mid_size_object*>(raw_ptr));
            new (p) mid_size_object();
            benchmark::DoNotOptimize(p);
            benchmark::DoNotOptimize(*p);
            pointers[i] = p;
        }

        for (int i = 0; i < iterations; ++i) {
            benchmark::DoNotOptimize(pointers[i]);
            manager.deallocate(pointers[i]);
        }
    }
}
BENCHMARK(same_size_mid_allocations_with_free_memory_manager);

void same_size_mid_allocations_with_misconfigured_free_memory_manager(benchmark::State& state) {
    allocator::free_memory_manager<128> manager;
    allocator::memory_slab<128> slabs[1000];
    allocator::launder_slab(slabs, 1000);
    manager.add_new_memory_segment(slabs);

    for (auto _ : state) {
        std::array<mid_size_object*, iterations> pointers;

        for (int i = 0; i < iterations; ++i) {
            void* raw_ptr = manager.allocate(sizeof(mid_size_object));
            benchmark::DoNotOptimize(raw_ptr);
            mid_size_object* p = std::launder(reinterpret_cast<mid_size_object*>(raw_ptr));
            new (p) mid_size_object();
            benchmark::DoNotOptimize(p);
            benchmark::DoNotOptimize(*p);
            pointers[i] = p;
        }

        for (int i = 0; i < iterations; ++i) {
            benchmark::DoNotOptimize(pointers[i]);
            manager.deallocate(pointers[i]);
        }
    }
}
BENCHMARK(same_size_mid_allocations_with_misconfigured_free_memory_manager);

void big_allocations_with_new(benchmark::State& state) {
    for (auto _ : state) {
        std::array<big_size_object*, iterations> pointers;

        for (int i = 0; i < iterations; ++i) {
            big_size_object* p = new big_size_object();
            benchmark::DoNotOptimize(p);
            benchmark::DoNotOptimize(*p);
            pointers[i] = p;
        }

        for (int i = 0; i < iterations; ++i) {
            benchmark::DoNotOptimize(pointers[i]);
            delete pointers[i];
        }
    }
}
BENCHMARK(big_allocations_with_new);

void big_allocations_with_free_memory_manager(benchmark::State& state) {
    allocator::free_memory_manager<256> manager;
    allocator::memory_slab<256> slabs[10000];
    allocator::launder_slab(slabs, 10000);
    manager.add_new_memory_segment(slabs);

    for (auto _ : state) {
        std::array<big_size_object*, iterations> pointers;

        for (int i = 0; i < iterations; ++i) {
            void* raw_ptr = manager.allocate(sizeof(big_size_object));
            benchmark::DoNotOptimize(raw_ptr);
            big_size_object* p = std::launder(reinterpret_cast<big_size_object*>(raw_ptr));
            new (p) big_size_object();
            benchmark::DoNotOptimize(p);
            benchmark::DoNotOptimize(*p);
            pointers[i] = p;
        }

        for (int i = 0; i < iterations; ++i) {
            benchmark::DoNotOptimize(pointers[i]);
            manager.deallocate(pointers[i]);
        }
    }
}
BENCHMARK(big_allocations_with_free_memory_manager);

BENCHMARK_MAIN();