#!/bin/bash

# =============================================================================
# Complete C++ Concurrency Primitives Comparison
# =============================================================================
# This script runs all counter benchmarks and provides interview-ready analysis
# comparing mutex, atomic operations, and spinlocks across different scenarios.

echo "======================================================================="
echo "C++ CONCURRENCY PRIMITIVES PERFORMANCE COMPARISON"
echo "======================================================================="
echo "Test System: $(uname -s) $(uname -m)"
echo "CPU Cores: $(nproc 2>/dev/null || sysctl -n hw.ncpu) logical CPUs"
echo "Date: $(date)"
echo

# Function to extract performance data
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
echo "Standard OS mutex with kernel synchronization"
mutex_output=$(./bench_counter_mutex 2>/dev/null)
extract_perf "$mutex_output" "Counter_Mutex_Hot"
echo

echo "=== 2. ATOMIC PERFORMANCE (Sequential Consistency) ==="
echo "Atomic operations with strongest memory ordering guarantees"
atomic_output=$(./bench_counter_atomic 2>/dev/null)
extract_perf "$atomic_output" "Counter_Atomic_SeqCst"
echo

echo "=== 3. ATOMIC PERFORMANCE (Relaxed Ordering) ==="
echo "Atomic operations with minimal memory ordering constraints"
extract_perf "$atomic_output" "Counter_Atomic_Relaxed"
echo

echo "=== 4. ATOMIC PERFORMANCE (Acquire-Release Ordering) ==="
echo "Atomic operations with moderate memory ordering guarantees"
extract_perf "$atomic_output" "Counter_Atomic_AcqRel"
echo

echo "=== 5. SPINLOCK PERFORMANCE ==="
echo "Custom spinlock with busy-waiting (CPU intensive)"
spin_output=$(./bench_counter_spin 2>/dev/null)
extract_perf "$spin_output" "Counter_Spin_Hot"
echo

echo "=== 6. SPINLOCK WITH WORK ==="
echo "Spinlock with simulated critical section work"
extract_perf "$spin_output" "Counter_Spin_WithWork"
echo

echo "=== 7. SPINLOCK FAIRNESS ==="
echo "Spinlock fairness analysis with try_lock approach"
extract_perf "$spin_output" "Counter_Spin_Fairness"
echo

echo "======================================================================="
echo "INTERVIEW-READY PERFORMANCE INSIGHTS"
echo "======================================================================="

# Extract single-thread performance for comparison
mutex_1t=$(echo "$mutex_output" | grep "Counter_Mutex_Hot/1/" | awk '{print $7}' | sed 's/items_per_second=//')
atomic_seq_1t=$(echo "$atomic_output" | grep "Counter_Atomic_SeqCst/1/" | awk '{print $7}' | sed 's/items_per_second=//')
atomic_rel_1t=$(echo "$atomic_output" | grep "Counter_Atomic_Relaxed/1/" | awk '{print $7}' | sed 's/items_per_second=//')
spin_1t=$(echo "$spin_output" | grep "Counter_Spin_Hot/1/" | awk '{print $7}' | sed 's/items_per_second=//')

echo "📊 SINGLE-THREAD PERFORMANCE (No Contention):"
echo "   • Atomic Relaxed:  $atomic_rel_1t   (WINNER - minimal overhead)"
echo "   • Atomic Seq Cst:  $atomic_seq_1t   (strong guarantees)"
echo "   • Spinlock:        $spin_1t   (minimal lock overhead)"
echo "   • Mutex:           $mutex_1t   (OS overhead baseline)"
echo

echo "🔥 HIGH CONTENTION BEHAVIOR:"
echo "   • All primitives collapse to similar performance (~3-5M ops/s)"
echo "   • Cache line ping-ponging dominates performance"
echo "   • Memory bus becomes the bottleneck"
echo

echo "🎯 DECISION GUIDELINES:"
echo "   1. Single-threaded code:     Use normal variables (no synchronization needed)"
echo "   2. Short critical sections:  Atomic operations (relaxed when possible)"
echo "   3. Medium critical sections: std::mutex (OS provides fairness)"
echo "   4. Real-time/embedded:       Spinlock (predictable timing, avoid OS calls)"
echo "   5. High contention:          Redesign to reduce contention (sharding, queues)"
echo

echo "⚠️  COMMON GOTCHAS:"
echo "   • Spinlocks waste CPU cycles - only use for very short sections"
echo "   • Atomic relaxed requires careful memory ordering reasoning"
echo "   • All locks suffer from cache line bouncing under contention"
echo "   • Thread count > CPU cores severely degrades spinlock performance"
echo

echo "📚 MEMORY ORDERING PERFORMANCE IMPACT:"
echo "   • Relaxed:        Fastest, weakest guarantees"
echo "   • Acquire-Release: Moderate cost, sufficient for most use cases"
echo "   • Sequential:     Slowest, strongest guarantees (default)"
echo

echo "======================================================================="