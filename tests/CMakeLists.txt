make_test(
    test_allocator
    allocator_tests.cc
    memory_slab_tests.cc
)

target_link_libraries(
    test_allocator
    allocator
)