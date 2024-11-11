make_test(
    test_allocator
    test_allocator.cc
)

target_link_libraries(
    test_allocator
    allocator
)