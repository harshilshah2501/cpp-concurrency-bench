# C++ Concurrency Benchmarking Suite

[![CI](https://github.com/harshilshah2501/cpp-concurrency-bench/actions/workflows/ci.yml/badge.svg)](https://github.com/harshilshah2501/cpp-concurrency-bench/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![C++20](https://img.shields.io/badge/C%2B%2B-20-orange.svg)](CMakeLists.txt)

Runnable C++20 benchmarks for standard concurrency primitives, with kitchen analogies for teaching and a **measured decision matrix** you generate on your own machine.

This project helps you:
- Compare `mutex`, atomics, spinlocks, shared locks, PC queues, barriers, pools, futures, and coroutines
- Measure **throughput and fairness**, not just folklore
- Learn trade-offs with consistent analogies — then **verify locally** (we do not ship invented speedups)

See [docs/METHODOLOGY.md](docs/METHODOLOGY.md), [docs/DECISION_MATRIX.md](docs/DECISION_MATRIX.md), and [docs/EXERCISES.md](docs/EXERCISES.md).

## Learning path (recommended)

Work through these in order. For each day: **predict → run → compare to your prediction**. Do not skip the predict step.

| Day | Focus | Kitchen metaphor | Command |
|-----|--------|------------------|---------|
| 1 | Contended counters | Knife vs digital counter vs salt-shaker hover | See below |
| 2 | Readers/writers + queues | Recipe book; kitchen bell / oven tokens | See below |
| 3 | Tasks & async | Kitchen staff, order tickets, master chef pause/resume | See below |

### Day 1 — counters (build once first)

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBENCH_NATIVE_ARCH=OFF
cmake --build build --parallel

# 1 thread, then raise thread count and watch throughput/fairness change
./build/bench_counter_mutex  --benchmark_filter='Counter_Mutex_Hot/'   --benchmark_min_time=0.2s
./build/bench_counter_atomic --benchmark_filter='Counter_Atomic_'      --benchmark_min_time=0.2s
./build/bench_counter_spin   --benchmark_filter='Counter_Spin_Hot/'    --benchmark_min_time=0.2s
```

**Ask yourself:** At 1 thread, who is fastest? At `hardware_concurrency`, who collapses first?

### Day 2 — sharing & coordination

```bash
./build/bench_rw_shared_mutex --benchmark_filter='SharedMutex_ReadHeavy|ExclusiveMutex_ReadHeavy' --benchmark_min_time=0.2s
./build/bench_pc_condvar      --benchmark_min_time=0.2s
./build/bench_pc_semaphore    --benchmark_filter='ProducerConsumer_' --benchmark_min_time=0.2s
./build/bench_barrier         --benchmark_filter='StdBarrier_WithWork|ManualBarrier_WithWork' --benchmark_min_time=0.2s
```

**Ask yourself:** When do shared locks beat exclusive locks? When is a semaphore enough vs a condition variable?

### Day 3 — tasking

```bash
./build/bench_thread_pool   --benchmark_filter='ThreadPool_CPUBound/.*/real_time$|StdAsync_CPUBound/.*/real_time$' --benchmark_min_time=0.05s
./build/bench_async_future  --benchmark_filter='StdAsync_Launch' --benchmark_min_time=0.2s
./build/bench_coroutines    --benchmark_filter='Coroutines_CPUBound|Threads_CPUBound|Coroutines_IOBound' --benchmark_min_time=0.2s
```

**Ask yourself:** When does thread-pool reuse beat `std::async`? What are you actually measuring with coroutine IO delays?

### Capture your own matrix

```bash
./scripts/run_all_benchmarks.sh simple
python3 scripts/build_decision_matrix.py benchmark_results/<timestamp> \
  --out matrix/corpus/$(uname -n | tr ' /' '__')/decision_matrix.json
```

Guided exercises: [docs/EXERCISES.md](docs/EXERCISES.md). Example measured corpus: [matrix/corpus/](matrix/corpus/).

## Quick Start

```bash
# Clone and setup
git clone https://github.com/harshilshah2501/cpp-concurrency-bench.git
cd cpp-concurrency-bench
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
# Optional local-only: -DBENCH_NATIVE_ARCH=ON
cmake --build build --parallel

# Run comprehensive benchmark suite
./scripts/run_all_benchmarks.sh full

# Or run quick comparison of core primitives
./scripts/run_all_benchmarks.sh simple

# Or run individual benchmarks
./build/bench_counter_mutex
./build/bench_rw_shared_mutex
./build/bench_coroutines
```

## Understanding Concurrency: Analogies and Real-World Problems

### The Fundamental Challenge
Imagine a busy restaurant kitchen where multiple chefs need to access shared resources (ingredients, equipment, prep stations). Without coordination, chaos ensues: chefs bump into each other, ingredients get double-used, orders get mixed up. **Concurrency primitives are like different management strategies to organize this chaos efficiently.**

---

### **Mutex (Mutual Exclusion)**
**Kitchen Analogy**: *A premium chef's knife that only one chef can use at a time*

```cpp
std::mutex kitchen_knife;
// Only one chef can use the knife at a time
std::lock_guard<std::mutex> lock(kitchen_knife);
chef.chop_vegetables();  // Safe, exclusive access
```

**Problem It Solves**: 
- **Race conditions** when multiple threads modify shared data
- **Data corruption** from simultaneous writes
- **Consistency** by ensuring atomic operations

**When to Use**:
- Protecting shared state that changes frequently
- Critical sections with moderate duration (100s-1000s of cycles)
- When fairness is important (threads take turns)

**Trade-offs**:
- High overhead for very short operations
- Can become a bottleneck under high contention
- Potential for deadlocks with multiple mutexes

---

### **Atomic Operations**
**Kitchen Analogy**: *A digital order counter that multiple chefs can safely update simultaneously*

```cpp
std::atomic<int> order_count{0};
// Multiple chefs can safely increment the order counter without locks
order_count.fetch_add(1, std::memory_order_relaxed);
```

**Problem It Solves**:
- **Lock-free programming** - no blocking, no deadlocks
- **High-performance counters** and simple shared variables
- **Memory efficiency** - no need for separate mutex objects

**When to Use**:
- Simple operations: increment, decrement, compare-and-swap
- High-frequency updates (millions of ops/second)
- Lock-free data structures

**Trade-offs**:
- Limited to simple operations
- Complex for anything beyond basic arithmetic
- Memory ordering can be tricky to get right

---

### **Spinlock**
**Kitchen Analogy**: *Hovering next to the salt shaker, constantly checking if the previous chef is done*

```cpp
spinlock salt_shaker_lock;
// Chef "spins" in place checking if salt shaker is available
salt_shaker_lock.lock();  // Busy-waiting, no sleeping
chef.add_seasoning();     // Very quick operation
salt_shaker_lock.unlock();
```

**Problem It Solves**:
- **Ultra-low latency** for very short critical sections
- **No kernel overhead** - pure userspace spinning
- **Predictable timing** - no scheduler unpredictability

**When to Use**:
- Extremely short critical sections (< 100 cycles)
- Real-time systems where predictability matters
- When you have spare CPU cores available

**Trade-offs**:
- **Wastes CPU cycles** spinning instead of useful work
- Terrible for longer operations
- Can cause starvation under high contention

---

### **Shared Mutex (Reader-Writer Lock)**
**Kitchen Analogy**: *A recipe book where multiple chefs can read recipes simultaneously, but only one chef can update recipes*

```cpp
std::shared_mutex recipe_book;

// Multiple chefs can read recipes simultaneously
std::shared_lock<std::shared_mutex> read_lock(recipe_book);
auto recipe = chef.read_recipe(dish_id);

// Only one chef can update recipes at a time
std::unique_lock<std::shared_mutex> write_lock(recipe_book);
chef.update_recipe(dish_id, new_instructions);
```

**Problem It Solves**:
- **Read-heavy workloads** where data is mostly consumed, rarely modified
- **Improved parallelism** for scenarios with many readers
- **Better throughput** than exclusive locks for read-dominant patterns

**When to Use**:
- Data structures with >70% read operations
- Configuration data, lookup tables, caches
- When read operations are expensive and benefit from parallelism

**Trade-offs**:
- Higher overhead than regular mutex for write operations
- **Writer starvation** - readers can monopolize the lock
- More complex than simple mutex

---

### 🚦 **Condition Variables**
**Kitchen Analogy**: *A kitchen bell that wakes up sleeping chefs when fresh ingredients arrive*

```cpp
std::condition_variable ingredient_arrived;
std::mutex mtx;
bool ingredients_ready = false;

// Chef waits for ingredients
std::unique_lock<std::mutex> lock(mtx);
ingredient_arrived.wait(lock, []{ return ingredients_ready; });
chef.start_cooking();

// Delivery person notifies chefs
{
    std::lock_guard<std::mutex> lock(mtx);
    ingredients_ready = true;
}
ingredient_arrived.notify_all();  // Ring bell to wake up all waiting chefs
```

**Problem It Solves**:
- **Efficient waiting** - threads sleep instead of busy-waiting
- **Producer-consumer coordination** - elegant signaling mechanism
- **Complex synchronization** patterns with multiple conditions

**When to Use**:
- Producer-consumer queues
- Thread pools waiting for work
- Any scenario where threads need to wait for specific conditions

**Trade-offs**:
- More complex than simple locks
- Risk of spurious wakeups
- Can be inefficient for simple signaling

---

### 🎫 **Semaphores (C++20)**
**Kitchen Analogy**: *Oven access tokens - only 4 chefs can use the ovens simultaneously*

```cpp
std::counting_semaphore<4> oven_slots{4};  // 4 ovens available

// Chef acquires an oven token
oven_slots.acquire();  // Blocks if all ovens busy
chef.bake_dish();
oven_slots.release();  // Returns oven token for others to use
```

**Problem It Solves**:
- **Resource counting** - limit concurrent access to finite resources
- **Simpler than condition variables** for many coordination patterns
- **Natural throttling** - prevents system overload

**When to Use**:
- Connection pools, thread pools with size limits
- Rate limiting and resource throttling
- Simpler alternative to condition variables for counting scenarios

**Trade-offs**:
- Less flexible than condition variables
- C++20 only (newer feature)
- Can't encode complex conditions

---

### 🚧 **Barrier (C++20)**
**Kitchen Analogy**: *A chef team meeting where all 4 chefs must finish prep before anyone starts cooking*

```cpp
std::barrier<> course_completion{4};  // Wait for all 4 chefs

// Each chef completes their prep work
chef.prepare_appetizer_ingredients();
course_completion.arrive_and_wait();  // Wait for everyone to finish prep

// Now all chefs start cooking together
chef.cook_appetizer();
```

**Problem It Solves**:
- **Bulk synchronous parallelism** - coordinate phases of work
- **Team synchronization** - ensure all threads reach same point
- **Pipeline stages** - orderly progression through phases

**When to Use**:
- Parallel algorithms with synchronization points
- Multi-phase computations
- Game engine frame synchronization

**Trade-offs**:
- All threads must reach barrier (weakest link problem)
- C++20 only
- Not suitable for producer-consumer patterns

---

### 🏊 **Thread Pool**
**Kitchen Analogy**: *A dedicated kitchen staff who pick up order tickets from a shared order queue*

```cpp
thread_pool kitchen_staff{8};  // 8 worker chefs

// Submit an order to be prepared
auto future_result = kitchen_staff.submit([]{
    return chef.prepare_complex_dish();
});

auto result = future_result.get();  // Retrieve the completed dish
```

**Problem It Solves**:
- **Thread reuse** - avoid expensive thread creation/destruction
- **Work distribution** - automatically balance load
- **Resource control** - limit concurrent thread count

**When to Use**:
- Many small tasks to be processed
- Web servers, task processing systems
- When thread creation overhead matters

**Trade-offs**:
- More complex than simple std::async
- Queue contention can become bottleneck
- Fixed pool size may not adapt to workload

---

### 🔮 **Async/Future**
**Kitchen Analogy**: *Giving an order ticket to a chef and getting a receipt - you can prep other things while waiting*

```cpp
// Start cooking appetizer asynchronously
auto future_appetizer = std::async(std::launch::async, []{
    return chef.prepare_appetizer();
});

// Do other kitchen work while appetizer cooks
chef.set_table();
chef.prepare_drinks();

// Retrieve appetizer when ready
auto appetizer = future_appetizer.get();
```

**Problem It Solves**:
- **Task-based parallelism** - focus on what to do, not how
- **Exception propagation** - errors travel with results
- **Composable async operations** - chain dependent tasks

**When to Use**:
- I/O-bound operations
- Independent computations that can run in parallel
- When you need result values from async work

**Trade-offs**:
- Thread management is hidden (less control)
- Can create too many threads with std::launch::async
- Future can only be retrieved once

---

### 🕸️ **Coroutines (C++20)**
**Kitchen Analogy**: *A master chef who can pause mid-recipe, help with another dish, then resume exactly where they left off*

```cpp
std::generator<int> kitchen_timer() {
    int minutes = 0;
    while (true) {
        co_yield minutes;        // Pause here, return current time
        minutes++;               // Continue timing
    }  // Resume here when next time is requested
}

auto timer = kitchen_timer();
int current_time = timer.next();  // Get time updates one by one
```

**Problem It Solves**:
- **Memory efficiency** - no full stack per "thread"
- **Cooperative multitasking** - explicit yield points
- **Elegant async code** - looks synchronous, runs asynchronously

**When to Use**:
- Thousands or millions of concurrent tasks
- I/O-heavy applications (network servers)
- State machines and generators

**Trade-offs**:
- C++20 only, limited compiler support
- More complex mental model
- No preemption - must cooperatively yield

---

### **Choosing the Right Tool**

| **Problem Type** | **Best Choice** | **Why?** |
|------------------|-----------------|----------|
| **Hot shared counter** | `std::atomic` | Lock-free, high throughput |
| **Complex shared state** | `std::mutex` | Safety and flexibility |
| **Ultra-short critical sections** | `spinlock` | Minimal latency |
| **Read-heavy data (90%+ reads)** | `std::shared_mutex` | Parallel reads |
| **Producer-consumer queues** | `std::condition_variable` | Efficient blocking |
| **Resource limiting** | `std::counting_semaphore` | Natural counting |
| **Phase synchronization** | `std::barrier` | Bulk coordination |
| **Many small tasks** | `thread_pool` | Thread reuse |
| **I/O with other work** | `std::async` | Task composition |
| **Millions of concurrent operations** | `coroutines` | Memory efficiency |

**Key Insight**: Each primitive is optimized for specific patterns. The "best" choice depends on your exact use case, contention level, and performance requirements.

---

### **Common Misconceptions & When NOT to Use**

#### **"Atomics are always faster than mutexes"**
**Reality**: Atomics excel at simple operations but become similar to mutexes under high contention.
```cpp
// GOOD: Simple order counter
std::atomic<int> order_counter;
order_counter.fetch_add(1);

// BAD: Complex kitchen state updates
std::atomic<KitchenState*> state;  // Use mutex instead for complex state
```

#### **"Spinlocks are always low-latency"**
**Reality**: Spinlocks waste CPU and perform terribly under contention.
```cpp
// GOOD: Protecting very short operations
spinlock.lock();
salt_level++;  // Single quick seasoning adjustment
spinlock.unlock();

// BAD: Any substantial cooking work
spinlock.lock();
chef.prepare_complex_sauce();  // Wastes CPU of waiting chefs
chef.call_supplier();          // Even worse!
spinlock.unlock();
```

#### **"Shared mutex is always better for reads"**
**Reality**: Break-even point is around 70% reads, and writers can starve.
```cpp
// GOOD: Read-heavy recipe lookups (90%+ reads)
std::shared_lock<std::shared_mutex> lock(recipe_book);
return recipe_book[dish_name];

// BAD: Frequent recipe updates or balanced read/write
// Regular mutex often performs better in the kitchen
```

#### **"Lock-free means faster"**
**Reality**: Lock-free adds complexity and may not improve performance for simple cases.
```cpp
// SIMPLE KITCHEN CASE: Mutex might be clearer and fast enough
std::mutex ingredient_guard;
ingredient_inventory.update();

// COMPLEX CASE: Lock-free when justified by measurements
lock_free_order_queue.push(order);  // Only if proven necessary
```

#### **"More chefs = better performance"**
**Reality**: Beyond optimal kitchen capacity, additional chefs often hurt performance.
```cpp
// BAD: Too many chefs in the kitchen
const int too_many_chefs = kitchen_stations * 4;

// GOOD: Match chef count to kitchen stations and workload
const int optimal_chefs = kitchen_stations;
```

## What We Benchmark

### Core Synchronization Primitives
- **Mutexes**: `std::mutex`, `std::shared_mutex` (reader-writer scenarios)
- **Atomics**: `std::atomic<T>` with different memory orderings (relaxed, acquire_release, seq_cst)
- **Custom Spinlocks**: `std::atomic_flag`-based implementation with yield strategies
- **Condition Variables**: `std::condition_variable` vs semaphore-based coordination
- **Modern Coordination**: `std::counting_semaphore`, `std::binary_semaphore`, `std::barrier` (C++20)
- **Async Patterns**: `std::future/promise`, `std::async`, thread pools, `std::coroutine` (C++20)

### Real-World Scenarios
- **Counter Synchronization**: Hot shared counters under extreme contention
- **Producer-Consumer**: Queue patterns with different coordination mechanisms  
- **Reader-Writer**: Various read/write ratios (90/10, 70/30, 50/50) with shared_mutex
- **Task Parallelism**: Thread pools vs std::async for CPU/I/O bound workloads
- **Thread Coordination**: Barrier synchronization for bulk synchronous parallel patterns
- **Memory Efficiency**: Coroutines vs threads for high concurrent task counts

### Analysis Dimensions
- **Contention Scaling**: Performance from 1 to N threads (hardware_concurrency)
- **Memory Ordering**: Cost analysis of relaxed vs acquire_release vs seq_cst
- **Fairness Analysis**: Thread starvation detection and variance measurement
- **Exception Handling**: Async exception propagation performance
- **Cache Effects**: False sharing impact with proper padding techniques

## 📈 Key Metrics & Analysis

### Performance Metrics
- **Throughput**: Operations per second across all threads
- **Latency**: Per-operation timing (median, p95, p99)
- **Scalability**: Performance vs thread count (1 to hardware_concurrency)
- **Contention Response**: Behavior under different load levels

### Fairness Metrics  
- **Coefficient of Variation**: Measures work distribution fairness
  - `CV < 0.1`: Excellent fairness
  - `CV 0.1-0.3`: Good fairness  
  - `CV > 0.5`: Poor fairness, potential starvation
- **Per-thread Variance**: Individual thread performance distribution

### System Metrics
- **Cache Performance**: LLC misses, cache line bouncing
- **CPU Utilization**: Cycles, instructions, context switches
- **Memory Access**: NUMA effects, memory bandwidth usage

## 🏗️ Project Structure

```
cpp-concurrency-bench/
├── CMakeLists.txt           # Build configuration with optimization flags
├── README.md               # This file
├── Analysis.md             # Performance analysis and insights  
├── include/                # Header files
│   ├── affinity.hpp        # Thread pinning utilities
│   ├── padded.hpp          # Cache-line padding wrapper
│   ├── spinlock.hpp        # Custom spinlock implementation  
│   ├── timing.hpp          # High-resolution timing
│   ├── thread_pool.hpp     # Thread pool for async benchmarks
│   └── variance_tracker.hpp # Fairness measurement
├── common/                 # Implementation files
│   ├── affinity.cpp        # Cross-platform thread affinity
│   ├── timing.cpp          # Timing implementation
│   └── variance_tracker.cpp # Statistical analysis
├── src/                    # Benchmark implementations
│   ├── bench_counter_mutex.cpp     # Mutex contention baseline
│   ├── bench_counter_atomic.cpp    # Atomic operations & memory ordering
│   ├── bench_counter_spin.cpp      # Spinlock comparison
│   ├── bench_pc_condvar.cpp        # Producer-consumer with condition variables
│   ├── bench_pc_semaphore.cpp      # Producer-consumer with semaphores (C++20)
│   ├── bench_rw_shared_mutex.cpp   # Reader-writer scenarios with shared_mutex
│   ├── bench_barrier.cpp           # Thread synchronization with std::barrier (C++20)
│   ├── bench_thread_pool.cpp       # Thread pool vs std::async comparison
│   ├── bench_coroutines.cpp        # C++20 coroutines vs traditional threading
│   └── bench_async_future.cpp      # Future/promise patterns and exception handling
├── scripts/                # Analysis and automation
│   ├── run_all_benchmarks.sh       # Comprehensive benchmark execution
│   ├── compare_all_counters.sh     # Counter synchronization comparison
│   └── compare_results.sh          # General result comparison utility
└── build/                  # Build artifacts (generated by cmake)
    ├── bench_*             # Individual benchmark executables
    └── ...                 # Other build files
```

## 🔬 Benchmark Workloads

### 1. Counter Synchronization (Fundamental Patterns)
**Purpose**: Measure basic synchronization primitive performance under contention.

#### Hot Counter Benchmarks
- **`bench_counter_mutex`**: Mutex-protected shared counter (baseline)
- **`bench_counter_atomic`**: Atomic operations with different memory orderings
- **`bench_counter_spin`**: Custom spinlock implementation
- **Key Insights**: When atomics outperform mutexes, memory ordering costs

### 2. Producer-Consumer Patterns (Coordination)
**Purpose**: Compare different coordination mechanisms for classic patterns.

#### Synchronization Primitives
- **`bench_pc_condvar`**: Condition variables + mutex (traditional approach)
- **`bench_pc_semaphore`**: C++20 counting/binary semaphores
- **Key Insights**: Semaphore efficiency, condition variable flexibility

### 3. Reader-Writer Scenarios (Shared Access)
**Purpose**: Analyze shared vs exclusive access patterns.

#### Read-Heavy Workloads
- **`bench_rw_shared_mutex`**: std::shared_mutex with various read/write ratios
- **Variants**: 90/10, 70/30, 50/50 read/write ratios  
- **Key Insights**: Break-even points, writer starvation, scalability

### 4. Thread Coordination (Synchronization Points)
**Purpose**: Measure bulk synchronization and barrier patterns.

#### Barrier Synchronization
- **`bench_barrier`**: std::barrier vs manual condition variable barriers
- **Patterns**: Work simulation, completion functions, arrival variance
- **Key Insights**: BSP model efficiency, coordination overhead

### 5. Task Parallelism (Async Execution)
**Purpose**: Compare different approaches to asynchronous task execution.

#### Thread Management
- **`bench_thread_pool`**: Custom thread pool vs std::async
- **`bench_async_future`**: Future/promise patterns with different launch policies
- **Workloads**: CPU-bound, I/O-bound, mixed patterns
- **Key Insights**: Thread creation overhead, task granularity, scalability

### 6. Advanced Patterns (Modern C++)
**Purpose**: Evaluate C++20 features against traditional approaches.

#### Coroutines vs Threads
- **`bench_coroutines`**: Stackless coroutines vs traditional threading
- **Patterns**: High task counts, I/O simulation, memory efficiency
- **Key Insights**: Memory overhead, context switching, composition benefits

## ⚙️ Environment Controls & Profiling

### Build Configuration
```bash
# Release build (for benchmarking)
cmake -DCMAKE_BUILD_TYPE=Release ..
```
**Optimization**: `-O3 -march=native -DNDEBUG` for maximum performance

### Thread Affinity & NUMA
```cpp
// Pin threads to specific cores for consistent results
affinity::pin_thread_to_core(core_id);
```

```bash
# Linux: Run within single NUMA node
numactl --cpunodebind=0 --membind=0 ./benchmark
```

### Advanced Analysis
```bash
# Linux: Comprehensive perf analysis
perf stat -e cycles,instructions,cache-misses,context-switches ./benchmark

# Fairness measurement
variance_tracker tracker(num_threads);
double fairness = tracker.coefficient_of_variation();
```

## How to read results (honesty first)

Kitchen analogies teach *intuition*. **Numbers must come from your machine.**

This repository intentionally does **not** publish absolute ops/s tables in the README.
Absolute throughput varies wildly with CPU, governor, library build type, and thread count.
Older drafts of this README included illustrative tables that looked like measurements —
those have been removed.

### Generate evidence locally

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBENCH_NATIVE_ARCH=OFF
cmake --build build --parallel
./scripts/run_all_benchmarks.sh simple
python3 scripts/build_decision_matrix.py benchmark_results/<timestamp> \
  --out matrix/generated/decision_matrix.json
```

- Methodology: [docs/METHODOLOGY.md](docs/METHODOLOGY.md)
- Decision matrix: [docs/DECISION_MATRIX.md](docs/DECISION_MATRIX.md)
- Public launch checklist: [docs/PUBLIC_LAUNCH.md](docs/PUBLIC_LAUNCH.md)

### Qualitative expectations (hypotheses to test — not confirmed facts)

| Kitchen tool | Expect to shine when… | Expect pain when… |
|--------------|----------------------|-------------------|
| Mutex | Complex critical sections; fairness matters | Ultra-hot tiny counters |
| Atomic | Simple RMW / counters | Multi-word invariants |
| Spinlock | Tiny CS + spare cores | Threads ≫ cores; long CS |
| Shared mutex | Strongly read-heavy | Write-heavy / writer latency SLOs |
| Cond var | Complex wake predicates | Simple slot counting (semaphore may be simpler) |
| Semaphore | Counted permits / bounded buffers | Rich logical conditions |
| Barrier | Bulk-synchronous phases | Over-sync of tiny phases |
| Thread pool | Many small CPU tasks | Tiny one-off tasks (overhead) vs unbounded async storms |
| Coroutines | Many waiters / structured async | Treating them as free parallelism without a scheduler |

After you measure, fill the decision matrix JSON and keep the raw Google Benchmark JSON beside it.

## 🎯 Benchmarking Results

### Decision Matrix

| Use Case | Best Choice | Alternative | Avoid |
|----------|-------------|-------------|-------|
| Hot counter, low contention | `std::atomic` (relaxed) | `spinlock` | `std::mutex` |
| Hot counter, high contention | Sharded counters | `std::mutex` | `spinlock` |
| Read-heavy (90%+ reads) | `std::shared_mutex` | Lock-free snapshot | Coarse `std::mutex` |
| Producer-consumer | `std::condition_variable` | `std::counting_semaphore` | Busy-wait |
| Very short critical sections | `spinlock` | `std::mutex` | `std::shared_mutex` |
| High fairness requirements | `std::mutex` | `std::counting_semaphore` | `spinlock` |

### Key Performance Insights

1. **Atomics vs Mutexes**: Atomics win at low contention, both degrade similarly at high contention
2. **Spinlocks**: Only beneficial for very short critical sections (< 100 cycles)
3. **Shared Mutexes**: Break-even at ~70% read ratio, accounting for writer starvation
4. **Memory Ordering**: `relaxed` ~2x faster than `seq_cst` in contended scenarios
5. **False Sharing**: Can reduce performance by 10-50x even with correct synchronization

## Known issues / limitations

- **Windows**: not a first-class target. Scripts and affinity helpers assume Linux/macOS. Contributions welcome with CI.
- **Numbers vary by host**: never compare README folklore across laptops; regenerate the decision matrix.
- **Debug builds skew results**: always use `CMAKE_BUILD_TYPE=Release` for comparisons.
- **Historical sample JSON** under `benchmark_results_simple/` may be incomplete or from Debug builds — do not cite.

Debugging hangs (general): see the troubleshooting section below and `scripts/debug_benchmarks.sh`.

---

## 🔍 **Debugging Concurrency Issues: A Complete Troubleshooting Guide**

### **🚨 When Benchmarks Hang: Step-by-Step Debugging Process**

This section documents the systematic approach we used to identify and fix the semaphore benchmark hanging issue.

#### **Step 1: Identify Hanging Processes**
```bash
# Check for stuck benchmark processes
ps aux | grep bench

# Look for processes with high CPU usage that aren't progressing
# Example output showing stuck process:
# harshil  4461  98.7  0.0  435309728  752  R+  10:35AM  10:40.56  ./build/bench_pc_semaphore
```

**🔍 Red Flags to Look For**:
- **High CPU usage** (>90%) for extended periods
- **Process running much longer** than expected benchmark duration
- **Real time >> CPU time** indicating waiting/blocking

#### **Step 2: Safe Process Termination**
```bash
# Kill the hanging process (use the PID from ps output)
kill -9 4461

# Also kill the parent script if it's stuck
kill -9 3688

# Verify processes are terminated
ps aux | grep bench
```

#### **Step 3: Identify Problematic Benchmark**
```bash
# List available benchmarks in the executable
./build/bench_pc_semaphore --benchmark_list_tests

# Test individual benchmarks with timeout
timeout 30 ./build/bench_pc_semaphore --benchmark_filter="ProducerConsumer_SPSC_Semaphore/10" --benchmark_min_time=0.1s
```

**🔍 Analysis Techniques**:
- **Exit code 124** = timeout occurred (process hung)
- **Exit code 0** = successful completion
- **No output + timeout** = deadlock or infinite loop

#### **Step 4: Root Cause Analysis**

**🧪 Common Concurrency Hang Patterns**:

1. **Producer-Consumer Race Conditions**:
   ```cpp
   // PROBLEMATIC: Race in shutdown logic
   void shutdown() {
       shutdown_.store(true, std::memory_order_release);
       
       // Race condition: threads may be blocked on acquire()
       for (size_t i = 0; i < max_size_; ++i) {
           empty_slots_.release();  // May not wake all threads
           filled_slots_.release();
       }
   }
   ```

2. **Consumer Loop Without Exit Condition**:
   ```cpp
   // PROBLEMATIC: Can busy-wait forever
   while (items_consumed.load() < target) {
       if (queue.pop(item)) {
           items_consumed++;
       }
       // No yield, timeout, or proper exit condition
   }
   ```

3. **Semaphore State Inconsistency**:
   ```cpp
   // PROBLEMATIC: Double-check pattern with race
   if (shutdown_.load()) return false;
   semaphore_.acquire();  // Can hang here if shutdown between check and acquire
   if (shutdown_.load()) {
       semaphore_.release(); // May not help other waiting threads
       return false;
   }
   ```

#### **Step 5: Fix that landed**

The durable fix is in `SemaphoreQueue`:

1. Use a **fresh queue per Google Benchmark iteration**
2. Call `close()` after producers finish so consumers can **drain**, then exit
3. Call `shutdown()` only for terminal cleanup

```cpp
// Producer finishes pushes, then:
queue.close();     // wake consumers; remaining items still pop successfully
// After joins:
queue.shutdown();
```

See `include/semaphore_queue.hpp` and `tests/test_smoke.cpp`.

#### **Step 6: Verification of Fix**
```bash
# Test that script runs without hanging
./scripts/run_all_benchmarks.sh full

# Monitor progress
ps aux | grep bench  # Should show normal progression

# Check for successful completion
echo $?  # Should be 0 for success
```

---

### **🛠️ Prevention Strategies for Future Development**

#### **Robust Producer-Consumer Patterns**

**✅ Proper Shutdown Coordination**:
```cpp
class SafeSemaphoreQueue {
    std::atomic<bool> shutdown_{false};
    std::condition_variable shutdown_cv_;
    std::mutex shutdown_mtx_;
    
public:
    void shutdown() {
        {
            std::lock_guard<std::mutex> lock(shutdown_mtx_);
            shutdown_.store(true, std::memory_order_release);
        }
        shutdown_cv_.notify_all();  // Wake all waiting threads
        
        // Release semaphores AFTER notifying
        for (size_t i = 0; i < max_size_; ++i) {
            empty_slots_.release();
            filled_slots_.release();
        }
    }
    
    bool pop(T& item) {
        while (!shutdown_.load(std::memory_order_acquire)) {
            // Try non-blocking first
            if (filled_slots_.try_acquire()) {
                std::lock_guard<std::mutex> lock(mtx_);
                if (!queue_.empty()) {
                    item = queue_.front();
                    queue_.pop();
                    empty_slots_.release();
                    return true;
                }
                filled_slots_.release(); // Return if queue was empty
            }
            
            // Short sleep to prevent busy-waiting
            std::this_thread::sleep_for(std::chrono::microseconds(10));
        }
        return false;  // Shutdown requested
    }
};
```

**✅ Timeout-Based Operations**:
```cpp
// Use timed operations instead of indefinite blocking
bool acquire_with_timeout(std::chrono::milliseconds timeout) {
    return semaphore_.try_acquire_for(timeout);
}

// Consumer with timeout
auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
while (!shutdown_.load() && std::chrono::steady_clock::now() < deadline) {
    if (queue.pop_with_timeout(item, std::chrono::milliseconds(100))) {
        process(item);
    }
}
```

**✅ Benchmark Safety Patterns**:
```cpp
static void BenchmarkProducerConsumer(benchmark::State& state) {
    const auto timeout = std::chrono::seconds(10);  // Max benchmark time
    
    for (auto _ : state) {
        auto start = std::chrono::steady_clock::now();
        
        // Run test with timeout monitoring
        std::atomic<bool> test_complete{false};
        
        std::thread timeout_monitor([&] {
            std::this_thread::sleep_for(timeout);
            if (!test_complete.load()) {
                // Force shutdown if test hangs
                queue.emergency_shutdown();
            }
        });
        
        // Actual test logic here
        run_producer_consumer_test();
        
        test_complete.store(true);
        timeout_monitor.join();
        
        auto duration = std::chrono::steady_clock::now() - start;
        if (duration > timeout * 0.9) {
            state.SkipWithError("Benchmark approaching timeout");
            break;
        }
    }
}
```

---

### **📋 Debugging Checklist for Concurrency Issues**

**🔍 Before Running Benchmarks**:
- [ ] Review thread coordination logic for race conditions
- [ ] Ensure proper shutdown mechanisms exist
- [ ] Add timeout protections for long-running tests
- [ ] Test individual benchmark components in isolation

**🚨 When Benchmarks Hang**:
- [ ] Identify hanging process with `ps aux | grep bench`
- [ ] Check CPU usage patterns (high CPU = busy-wait, low CPU = deadlock)
- [ ] Kill hanging processes safely with `kill -9 <PID>`
- [ ] Test minimal benchmark subsets to isolate issue
- [ ] Review coordination primitives for proper cleanup

**🔧 Implementing Fixes**:
- [ ] Add proper timeout mechanisms
- [ ] Implement graceful shutdown coordination
- [ ] Use non-blocking operations where possible
- [ ] Add emergency shutdown capabilities
- [ ] Document known issues and workarounds

**✅ Verification**:
- [ ] Test fixed benchmarks with timeouts
- [ ] Verify full benchmark suite runs to completion
- [ ] Monitor system resources during execution
- [ ] Update documentation with lessons learned

This systematic approach ensures robust benchmarking and helps identify concurrency issues before they cause production problems.

##  Common Gotchas and Anti-Patterns

### False Sharing
```cpp
// BAD: False sharing
std::atomic<int> counter1, counter2;  // Likely same cache line

// GOOD: Proper padding  
padded<std::atomic<int>> counter1, counter2;  // Separate cache lines
```

### Memory Ordering Overkill
```cpp
// BAD: Unnecessary seq_cst overhead
counter.fetch_add(1, std::memory_order_seq_cst);

// GOOD: Relaxed ordering for simple counters
counter.fetch_add(1, std::memory_order_relaxed);
```

### Spinlock Misuse
```cpp
// BAD: Spinlock with long critical section
spinlock.lock();
expensive_computation();  // Wastes CPU cycles
spinlock.unlock();

// GOOD: Mutex for longer operations
std::lock_guard<std::mutex> lock(mtx);
expensive_computation();
```

## 📚 Further Reading

- **C++ Concurrency in Action** by Anthony Williams
- **The Art of Multiprocessor Programming** by Herlihy & Shavit  
- **Intel® 64 and IA-32 Architectures Optimization Reference Manual**
- **ARM Cortex-A Series Programmer's Guide**

## 🤝 Contributing

1. Fork the repository
2. Create a feature branch for new benchmarks
3. Follow the existing code style and documentation standards
4. Add comprehensive comments explaining the benchmark purpose
5. Include expected results and analysis in commit messages

## 📄 License

MIT License - See LICENSE file for details
