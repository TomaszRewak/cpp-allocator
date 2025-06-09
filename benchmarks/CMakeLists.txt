make_executable(
    benchmarks
    main.cc
)

target_link_libraries(
    benchmarks
    allocator
    benchmark
)