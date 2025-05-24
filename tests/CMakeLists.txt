make_test(
    test_allocator
    free_memory_manager_tests.cc
    memory_destructor_tests.cc
    memory_tests.cc
    memory_slab_tests.cc
)

target_link_libraries(
    test_allocator
    allocator
)