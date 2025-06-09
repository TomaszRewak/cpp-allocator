cmake_minimum_required(VERSION 3.28)

project(exchange
    VERSION 0.0.1
    DESCRIPTION "A custom allocator for C++"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
enable_testing()

macro(make_library name)
    add_library(${name} STATIC ${ARGN})
    target_include_directories(${name} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
endmacro()

macro(make_test name)
    add_executable(${name} ${ARGN})
    target_link_libraries(${name} gtest_main)
    include(GoogleTest)
    gtest_discover_tests(${name})
endmacro()

macro(make_executable name)
    add_executable(${name} ${ARGN})
    target_include_directories(${name} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
endmacro()

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
FetchContent_MakeAvailable(googletest)
FetchContent_Declare(
  benchmark
  URL https://github.com/google/benchmark/archive/refs/tags/v1.9.4.zip
)
FetchContent_MakeAvailable(benchmark)

add_subdirectory(src)
add_subdirectory(tests)
add_subdirectory(benchmarks)
