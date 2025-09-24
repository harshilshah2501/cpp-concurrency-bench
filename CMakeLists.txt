cmake_minimum_required(VERSION 3.20)
project(cpp-concurrency-bench)

# =============================================================================
# C++ Concurrency Benchmarking Suite
# =============================================================================
# This project benchmarks various C++ concurrency primitives to provide
# data-driven insights for choosing synchronization mechanisms in different
# scenarios. The benchmarks measure throughput, latency, fairness, and
# scalability across varying thread counts and contention levels.
# 
# Key goals:
# - Provide interview-ready performance comparisons
# - Demonstrate real-world performance characteristics
# - Identify optimal use cases for each primitive
# - Measure fairness and variance across threads
# =============================================================================

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Optimization flags for accurate performance measurement
# -O3: Maximum optimization for release builds
# -march=native: Use CPU-specific optimizations for the build machine
# -DNDEBUG: Disable assertions that could skew timing results
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -DNDEBUG")

# Debug build with profiling symbols for analysis
# -g: Include debug symbols for profiling tools (perf, Instruments)
# -O0: No optimization to see actual code structure
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")

# Default to Release for consistent benchmarking
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Required for std::thread and pthread operations
find_package(Threads REQUIRED)

# =============================================================================
# Google Benchmark Dependency
# =============================================================================
# We use FetchContent to ensure consistent benchmark framework across
# different systems. This avoids issues with system-installed versions
# and ensures reproducible results.
include(FetchContent)
FetchContent_Declare(
  googlebenchmark
  GIT_REPOSITORY https://github.com/google/benchmark.git
  GIT_TAG v1.8.3
)

# Disable benchmark's own tests to speed up build
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googlebenchmark)

# Include our utility headers
include_directories(include common)

# =============================================================================
# Common Utilities Library
# =============================================================================
# Shared utilities for thread affinity, timing, and statistical analysis
# These are used across all benchmarks for consistent measurement
add_library(bench_common STATIC
    common/affinity.cpp      # CPU core pinning for consistent results
    common/timing.cpp        # High-resolution timing utilities
    common/variance_tracker.cpp  # Fairness and variance measurement
)
target_link_libraries(bench_common Threads::Threads)

# =============================================================================
# Individual Benchmark Executables
# =============================================================================
# Each benchmark focuses on a specific concurrency primitive or pattern
# This allows running individual tests and comparing results

# Hot counter benchmark - measures peak contention performance
add_executable(bench_counter_mutex src/bench_counter_mutex.cpp)
target_link_libraries(bench_counter_mutex 
    bench_common 
    benchmark::benchmark 
    Threads::Threads
)

# Mutex benchmark - measures mutex performance under contention
# Add to CMakeLists.txt after the mutex benchmark
add_executable(bench_counter_atomic src/bench_counter_atomic.cpp)
target_link_libraries(bench_counter_atomic 
    bench_common 
    benchmark::benchmark 
    Threads::Threads
)

# Add this to CMakeLists.txt after the atomic benchmark
add_executable(bench_counter_spin src/bench_counter_spin.cpp)
target_link_libraries(bench_counter_spin 
    bench_common 
    benchmark::benchmark 
    Threads::Threads
)

# Producer-Consumer with Condition Variables
add_executable(bench_pc_condvar src/bench_pc_condvar.cpp)
target_link_libraries(bench_pc_condvar 
    bench_common 
    benchmark::benchmark 
    Threads::Threads
)

# Producer-Consumer with Semaphores
add_executable(bench_pc_semaphore src/bench_pc_semaphore.cpp)
target_link_libraries(bench_pc_semaphore 
    bench_common 
    benchmark::benchmark 
    Threads::Threads
)

# Reader-Writer with Shared Mutex
add_executable(bench_rw_shared_mutex src/bench_rw_shared_mutex.cpp)
target_link_libraries(bench_rw_shared_mutex 
    bench_common 
    benchmark::benchmark 
    Threads::Threads
)

# Barrier Synchronization
add_executable(bench_barrier src/bench_barrier.cpp)
target_link_libraries(bench_barrier 
    bench_common 
    benchmark::benchmark 
    Threads::Threads
)

# Thread Pool vs std::async
add_executable(bench_thread_pool src/bench_thread_pool.cpp)
target_link_libraries(bench_thread_pool 
    bench_common 
    benchmark::benchmark 
    Threads::Threads
)

# C++20 Coroutines
add_executable(bench_coroutines src/bench_coroutines.cpp)
target_link_libraries(bench_coroutines 
    bench_common 
    benchmark::benchmark 
    Threads::Threads
)

# Async Future Patterns
add_executable(bench_async_future src/bench_async_future.cpp)
target_link_libraries(bench_async_future 
    bench_common 
    benchmark::benchmark 
    Threads::Threads
)

