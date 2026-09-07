cmake_minimum_required(VERSION 3.20)
project(cpp-concurrency-bench)

# =============================================================================
# C++ Concurrency Benchmarking Suite
# =============================================================================

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

option(BENCH_NATIVE_ARCH
    "Enable -march=native for local microbenchmarks (off by default for reproducible CI builds)"
    OFF)

find_package(Threads REQUIRED)

include(FetchContent)
# Pin to the annotated commit for tag v1.8.3 for supply-chain reproducibility.
FetchContent_Declare(
  googlebenchmark
  GIT_REPOSITORY https://github.com/google/benchmark.git
  GIT_TAG 59abfeefc1c0a8a672879b7068db63d94d109c7b
  GIT_SHALLOW TRUE
)

set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googlebenchmark)

add_library(bench_common STATIC
    common/affinity.cpp
    common/timing.cpp
    common/variance_tracker.cpp
)
target_include_directories(bench_common PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/common
)
target_link_libraries(bench_common PUBLIC Threads::Threads)
target_compile_options(bench_common PRIVATE
    $<$<CONFIG:Release>:-O3 -DNDEBUG>
    $<$<CONFIG:Debug>:-g -O0>
    $<$<AND:$<BOOL:${BENCH_NATIVE_ARCH}>,$<CONFIG:Release>>:-march=native>
)

function(add_bench target source)
    add_executable(${target} ${source})
    target_link_libraries(${target} PRIVATE
        bench_common
        benchmark::benchmark
        Threads::Threads
    )
    target_compile_options(${target} PRIVATE
        $<$<CONFIG:Release>:-O3 -DNDEBUG>
        $<$<CONFIG:Debug>:-g -O0>
        $<$<AND:$<BOOL:${BENCH_NATIVE_ARCH}>,$<CONFIG:Release>>:-march=native>
    )
endfunction()

add_bench(bench_counter_mutex src/bench_counter_mutex.cpp)
add_bench(bench_counter_atomic src/bench_counter_atomic.cpp)
add_bench(bench_counter_spin src/bench_counter_spin.cpp)
add_bench(bench_pc_condvar src/bench_pc_condvar.cpp)
add_bench(bench_pc_semaphore src/bench_pc_semaphore.cpp)
add_bench(bench_rw_shared_mutex src/bench_rw_shared_mutex.cpp)
add_bench(bench_barrier src/bench_barrier.cpp)
add_bench(bench_thread_pool src/bench_thread_pool.cpp)
add_bench(bench_coroutines src/bench_coroutines.cpp)
add_bench(bench_async_future src/bench_async_future.cpp)

# Smoke unit tests for core utilities (no external test framework).
enable_testing()
add_executable(test_smoke tests/test_smoke.cpp)
target_link_libraries(test_smoke PRIVATE bench_common Threads::Threads)
add_test(NAME smoke COMMAND test_smoke)
