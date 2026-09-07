# Contributing to C++ Concurrency Benchmarking Suite

Thank you for your interest in contributing! This project aims to provide comprehensive, educational benchmarks for C++ concurrency primitives.

## 🎯 Project Goals

- **Educational**: Help developers understand concurrency trade-offs with real data
- **Interview Preparation**: Provide concrete performance insights for technical discussions
- **Production Guidance**: Offer data-driven decision making for system design

## 🚀 Quick Start for Contributors

1. **Fork and Clone**
   ```bash
   git clone https://github.com/harshilshah2501/cpp-concurrency-bench.git
   cd cpp-concurrency-bench
   ```

2. **Build and Test**
   ```bash
   cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
   cmake --build build -j
   ctest --test-dir build --output-on-failure
   
   # Test your changes
   ./scripts/run_all_benchmarks.sh simple
   ```

## 🔧 Development Guidelines

### Adding New Benchmarks

1. **Create the benchmark file** in `src/bench_your_feature.cpp`
2. **Follow the existing pattern**:
   ```cpp
   #include <benchmark/benchmark.h>
   #include "common_includes.hpp"
   
   static void BenchmarkYourFeature(benchmark::State& state) {
       // Setup
       for (auto _ : state) {
           // Measured code
           benchmark::DoNotOptimize(result);
       }
       // Cleanup and metrics
       state.SetItemsProcessed(state.iterations());
   }
   BENCHMARK(BenchmarkYourFeature)->RangeMultiplier(2)->Range(1, std::thread::hardware_concurrency());
   ```

3. **Add to CMakeLists.txt** via `add_bench(bench_your_feature src/bench_your_feature.cpp)`

### Kitchen Analogy Guidelines

When explaining concurrency concepts, maintain our consistent **restaurant kitchen theme**:

- **Mutex**: Premium chef's knife (exclusive access)
- **Atomic**: Digital order counter (lock-free updates)
- **Spinlock**: Hovering by salt shaker (busy waiting)
- **Shared Mutex**: Recipe book (multiple readers, single writer)
- **Condition Variable**: Kitchen bell (wake sleeping threads)
- **Semaphore**: Oven tokens (resource counting)
- **Barrier**: Staff meeting (synchronization point)
- **Thread Pool**: Kitchen staff (worker threads)
- **Async/Future**: Order receipt (asynchronous tasks)
- **Coroutines**: Master chef (pause/resume capability)

### Code Style

- **C++20 Standard**: Use modern C++ features when available
- **Google Style**: Follow Google C++ Style Guide
- **Comments**: Explain the "why", not just the "what"
- **Safety**: Always prefer safe constructs over performance hacks

### Benchmarking Best Practices

1. **Reproducible Results**:
   ```cpp
   // Pin threads for consistent results
   affinity::pin_thread_to_core(core_id);
   
   // Use proper timing
   auto start = high_resolution_clock::now();
   ```

2. **Meaningful Metrics**:
   ```cpp
   state.SetItemsProcessed(operations_completed);
   state.counters["fairness_cv"] = variance_tracker.coefficient_of_variation();
   ```

3. **Avoid Compiler Optimization Issues**:
   ```cpp
   benchmark::DoNotOptimize(result);  // Prevent optimization
   benchmark::ClobberMemory();        // Prevent reordering
   ```

## 📊 Testing Your Changes

### Local Testing
```bash
# Build and run specific benchmark
make bench_your_feature
./bench_your_feature --benchmark_min_time=0.1s

# Test full suite
../scripts/run_all_benchmarks.sh simple
```

### Performance Validation
```bash
# Compare before/after results
../scripts/compare_results.sh benchmark_results/before benchmark_results/after

# Check for regressions (should be within 5%)
```

### Documentation Updates
- Update README.md with new concepts and analogies
- Add benchmark results to the "Proving Our Hypotheses" section
- Include performance insights and trade-offs

## 🐛 Reporting Issues

### Bug Reports
Include:
- **System Info**: OS, compiler version, hardware
- **Reproduction Steps**: Minimal example to reproduce
- **Expected vs Actual**: What should happen vs what happens
- **Benchmark Output**: Relevant logs or performance data

### Performance Issues
Include:
- **Benchmark Results**: Before/after performance data
- **System Load**: CPU usage, memory pressure during test
- **Environment**: Thread affinity, NUMA topology effects

### Hanging Issues
Follow our debugging checklist:
1. Identify hanging process: `ps aux | grep bench`
2. Kill safely: `kill -9 <PID>`
3. Test with timeout: `timeout 30 ./benchmark`
4. Report race conditions or deadlock patterns

## 🎯 Priority Areas for Contribution

### High Priority
- **Fix semaphore benchmark hanging** — addressed via `SemaphoreQueue::close()` lifecycle
- Expand decision-matrix corpus across CPU vendors
- Optional Windows CI + affinity implementation
- **Add NUMA-aware benchmarks** for multi-socket systems
- **Coroutine performance analysis** (C++20 adoption increasing)
- **Lock-free data structure benchmarks**

### Medium Priority
- **GPU compute integration** (CUDA/OpenCL coordination patterns)
- **Network I/O async patterns** (io_uring, ASIO benchmarks)
- **Memory allocation benchmarks** (allocator coordination)

### Documentation
- **Video explanations** of kitchen analogies
- **Interactive examples** with web-based visualization
- **More real-world case studies**

## 📝 Pull Request Process

1. **Create Feature Branch**
   ```bash
   git checkout -b feature/your-benchmark-name
   ```

2. **Make Changes**
   - Add benchmark implementation
   - Update documentation
   - Add tests if applicable

3. **Test Thoroughly**
   ```bash
   # Full benchmark suite should pass
   ./scripts/run_all_benchmarks.sh full
   
   # No hanging processes
   # Performance within expected ranges
   ```

4. **Create Pull Request**
   - **Clear title**: "Add benchmark for [feature] with [insight]"
   - **Description**: Explain the concurrency pattern being benchmarked
   - **Results**: Include sample performance data
   - **Kitchen Analogy**: How it fits our educational theme

5. **Code Review**
   - Address reviewer feedback promptly
   - Maintain educational focus
   - Ensure production-ready quality

## 🏆 Recognition

Contributors will be:
- **Credited** in README.md and documentation
- **Featured** in performance analysis sections
- **Invited** to co-author educational content

## 📚 Resources

- [Google Benchmark Library](https://github.com/google/benchmark)
- [C++20 Concurrency Features](https://en.cppreference.com/w/cpp/thread)
- [Intel Optimization Manual](https://software.intel.com/content/www/us/en/develop/articles/intel-sdm.html)
- [ARM Optimization Guide](https://developer.arm.com/documentation/den0024/a/)

---

**Questions?** Open an issue or start a discussion. We're here to help make C++ concurrency more approachable for everyone! 🚀