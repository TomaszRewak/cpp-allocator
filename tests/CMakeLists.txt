make_test(
    test_allocator
    memory_destructor_tests.cc
    memory_tests.cc
    memory_slab_tests.cc
)

target_link_libraries(
    test_allocator
    allocator
)