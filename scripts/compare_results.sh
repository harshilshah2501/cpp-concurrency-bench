#!/bin/bash

# =============================================================================
# Complete C++ Concurrency Primitives Comparison
# =============================================================================
# Runs counter benchmarks from the build directory and prints a comparison.

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-${ROOT_DIR}/build}"

if [ ! -x "${BUILD_DIR}/bench_counter_mutex" ]; then
  echo "Benchmarks not found in ${BUILD_DIR}."
  echo "Build first: cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j"
  exit 1
fi

echo "======================================================================="
echo "C++ CONCURRENCY PRIMITIVES PERFORMANCE COMPARISON"
echo "======================================================================="
echo "Test System: $(uname -s) $(uname -m)"
echo "CPU Cores: $(nproc 2>/dev/null || sysctl -n hw.ncpu)"
echo "Build dir: ${BUILD_DIR}"
echo "Date: $(date)"
echo

extract_perf() {
    local benchmark_output="$1"
    local pattern="$2"
    echo "$benchmark_output" | grep "$pattern" | awk '{
        threads = gensub(/.*\/([0-9]+)\/.*/, "\\1", "g", $1)
        throughput = gensub(/.*items_per_second=([0-9.]+[KMG]?).*/, "\\1", "g", $0)
        fairness = gensub(/.*fairness_cv=([0-9.]+).*/, "\\1", "g", $0)
        printf "%2s threads: %10s ops/s (fairness: %s)\n", threads, throughput, fairness
    }'
}

echo "=== 1. MUTEX PERFORMANCE ==="
mutex_output=$("${BUILD_DIR}/bench_counter_mutex" 2>/dev/null)
extract_perf "$mutex_output" "Counter_Mutex_Hot"
echo

echo "=== 2. ATOMIC PERFORMANCE (Sequential Consistency) ==="
atomic_output=$("${BUILD_DIR}/bench_counter_atomic" 2>/dev/null)
extract_perf "$atomic_output" "Counter_Atomic_SeqCst"
echo

echo "=== 3. ATOMIC PERFORMANCE (Relaxed Ordering) ==="
extract_perf "$atomic_output" "Counter_Atomic_Relaxed"
echo

echo "=== 4. ATOMIC PERFORMANCE (Acquire-Release Ordering) ==="
extract_perf "$atomic_output" "Counter_Atomic_AcqRel"
echo

echo "=== 5. SPINLOCK PERFORMANCE ==="
spin_output=$("${BUILD_DIR}/bench_counter_spin" 2>/dev/null)
extract_perf "$spin_output" "Counter_Spin_Hot"
echo

echo "=== 6. SPINLOCK WITH WORK ==="
extract_perf "$spin_output" "Counter_Spin_WithWork"
echo

echo "=== 7. SPINLOCK FAIRNESS ==="
extract_perf "$spin_output" "Counter_Spin_Fairness"
echo

echo "======================================================================="
echo "Decision guidelines (qualitative — verify against numbers above)"
echo "======================================================================="
echo "1. Short critical sections / counters: prefer atomics when possible"
echo "2. Complex critical sections: std::mutex"
echo "3. Extremely short + spare cores: spinlock (measure carefully)"
echo "4. High contention: reduce sharing (sharding) before switching locks"
echo "======================================================================="
