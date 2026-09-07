#!/bin/bash

# =============================================================================
# C++ Concurrency Benchmarking Suite - Complete Analysis
# =============================================================================
# This script runs all concurrency benchmarks and generates comprehensive
# performance comparisons for technical interview preparation.
#
# Usage: ./run_all_benchmarks.sh [mode] [output_dir]
#   mode: 'simple' for quick comparison, 'full' for comprehensive analysis
#
# Benchmark categories:
# 1. Counter Synchronization: mutex, atomic, spinlock
# 2. Producer-Consumer: condition variables, semaphores  
# 3. Reader-Writer: shared_mutex patterns
# 4. Thread Coordination: barriers
# 5. Task Parallelism: thread pools, async/futures
# 6. Advanced Patterns: coroutines
# =============================================================================

OUTPUT_DIR="${2:-benchmark_results}"
MODE="${1:-simple}"
BUILD_DIR="build"
TIMESTAMP=$(date +"%Y%m%d_%H%M%S")
RESULTS_DIR="${OUTPUT_DIR}/${TIMESTAMP}"

# Portable timeout binary (GNU coreutils on Linux; gtimeout via brew on macOS)
if command -v timeout >/dev/null 2>&1; then
    TIMEOUT_BIN="timeout"
elif command -v gtimeout >/dev/null 2>&1; then
    TIMEOUT_BIN="gtimeout"
else
    echo "Error: timeout/gtimeout not found. On macOS: brew install coreutils" >&2
    exit 1
fi

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Create results directory
mkdir -p "${RESULTS_DIR}"

# Function to monitor for hanging processes
monitor_hanging_processes() {
    echo -e "${YELLOW}Checking for any remaining benchmark processes...${NC}"
    local hanging_procs=$(ps aux | grep "bench_" | grep -v grep | grep -v "run_all_benchmarks")
    
    if [ -n "$hanging_procs" ]; then
        echo -e "${RED}Warning: Found potentially hanging benchmark processes:${NC}"
        echo "$hanging_procs"
        echo -e "${YELLOW}Consider killing these with: pkill -f 'bench_'${NC}"
        return 1
    else
        echo -e "${GREEN}✓ No hanging benchmark processes detected${NC}"
        return 0
    fi
}

echo -e "${BLUE}=== C++ Concurrency Benchmarking Suite ===${NC}"
echo "Mode: ${MODE}"
echo "Results will be saved to: ${RESULTS_DIR}"
echo "Timestamp: ${TIMESTAMP}"
echo

# Function to run a single benchmark with timeout protection
run_benchmark() {
    local name="$1"
    local executable="$2"
    local filter="$3"
    local min_time="${4:-1s}"
    local timeout_duration="${5:-300}"  # 5 minute default timeout
    
    # Adjust timing based on mode
    if [ "$MODE" = "simple" ]; then
        min_time="0.2s"
        timeout_duration="60"  # 1 minute for simple mode
    fi
    
    echo -e "${YELLOW}Running ${name}...${NC}"
    
    if [ ! -f "${BUILD_DIR}/${executable}" ]; then
        echo -e "${RED}Error: ${executable} not found. Please build first.${NC}"
        return 1
    fi
    
    # Record start time for monitoring
    local start_time=$(date +%s)
    
    local output_file="${RESULTS_DIR}/${name}.json"
    local filter_arg=""
    
    if [ -n "$filter" ]; then
        filter_arg="--benchmark_filter=${filter}"
    fi
    
    # Run benchmark with timeout protection
    echo "  Timeout: ${timeout_duration}s | Min time: ${min_time}"
    # shellcheck disable=SC2086 # filter_arg intentionally empty or a single flag
    "${TIMEOUT_BIN}" "${timeout_duration}s" ./${BUILD_DIR}/${executable} \
        --benchmark_format=json \
        --benchmark_out="${output_file}" \
        --benchmark_min_time="${min_time}" \
        --benchmark_repetitions=3 \
        --benchmark_report_aggregates_only=true \
        ${filter_arg}
    
    local exit_code=$?
    local end_time=$(date +%s)
    local duration=$((end_time - start_time))
    
    if [ $exit_code -eq 0 ]; then
        echo -e "${GREEN}✓ ${name} completed (${duration}s)${NC}"
    elif [ $exit_code -eq 124 ]; then
        echo -e "${RED}✗ ${name} TIMED OUT after ${timeout_duration}s${NC}"
        echo -e "${YELLOW}  This indicates a potential deadlock or infinite loop${NC}"
        
        # Kill any remaining processes
        pkill -f "$executable" 2>/dev/null
        
        # Create empty result file to prevent script errors
        echo '{"benchmarks":[],"error":"timeout"}' > "${output_file}"
        
        return 124
    else
        echo -e "${RED}✗ ${name} failed with exit code ${exit_code} (${duration}s)${NC}"
        return $exit_code
    fi
}

# Function to extract performance summary from JSON
extract_summary() {
    local json_file="$1"
    local bench_name="$2"
    
    if [ ! -f "$json_file" ]; then
        echo "N/A | N/A | N/A"
        return
    fi
    
    local throughput=$(jq -r '.benchmarks[0].items_per_second // .benchmarks[0].throughput // "N/A"' "$json_file" 2>/dev/null)
    local latency=$(jq -r '.benchmarks[0].real_time // .benchmarks[0].cpu_time // "N/A"' "$json_file" 2>/dev/null)
    local threads=$(jq -r '.benchmarks[0].run_name' "$json_file" 2>/dev/null | grep -o '/[0-9]*' | tr -d '/' | head -1)
    
    [ -z "$threads" ] && threads="1"
    [ "$throughput" = "null" ] && throughput="N/A"
    [ "$latency" = "null" ] && latency="N/A"
    
    # Format numbers for readability
    if [ "$throughput" != "N/A" ]; then
        throughput=$(printf "%.2f" "$throughput" 2>/dev/null || echo "$throughput")
    fi
    if [ "$latency" != "N/A" ]; then
        latency=$(printf "%.3f ms" "$(echo "$latency / 1000000" | bc -l 2>/dev/null)" 2>/dev/null || echo "$latency")
    fi
    
    echo "${throughput} | ${latency} | ${threads}"
}

# Function to build all benchmarks
build_all() {
    echo -e "${YELLOW}Building all benchmarks...${NC}"

    if [ ! -d "${BUILD_DIR}" ]; then
        echo -e "${YELLOW}Configuring ${BUILD_DIR}...${NC}"
        cmake -S . -B "${BUILD_DIR}" -DCMAKE_BUILD_TYPE=Release
    fi

    if cmake --build "${BUILD_DIR}" --parallel; then
        echo -e "${GREEN}Build complete${NC}"
    else
        echo -e "${RED}Build failed${NC}"
        exit 1
    fi
    echo
}

# Build all benchmarks first
build_all

# Run benchmarks based on mode
if [ "$MODE" = "simple" ]; then
    echo -e "${BLUE}=== Simple Mode: Core Synchronization Comparison ===${NC}"
    
    # Counter benchmarks for direct comparison
    run_benchmark "counter_mutex" "bench_counter_mutex" "Counter_Mutex_Hot/1/"
    run_benchmark "counter_atomic" "bench_counter_atomic" "Counter_Atomic_Relaxed/1/"
    run_benchmark "counter_spin" "bench_counter_spin" "Counter_Spin_Hot/1/"
    
    # Reader-writer comparison (skip producer-consumer as they're inherently multi-threaded)
    run_benchmark "rw_shared_mutex" "bench_rw_shared_mutex" "SharedMutex_ReadHeavy_90_10/1/"
    
    # Generate simple comparison summary
    echo -e "${BLUE}=== Performance Summary ===${NC}"
    cat > "${RESULTS_DIR}/simple_summary.txt" << EOF
=================================================================
C++ Synchronization Primitives - Performance Comparison
=================================================================

Primitive               | Throughput (ops/s) | Latency      | Threads
-----------------------|-------------------|--------------|--------
EOF
    
    echo "Mutex (baseline)       | $(extract_summary "${RESULTS_DIR}/counter_mutex.json" "mutex")" >> "${RESULTS_DIR}/simple_summary.txt"
    echo "Atomic (relaxed)       | $(extract_summary "${RESULTS_DIR}/counter_atomic.json" "atomic")" >> "${RESULTS_DIR}/simple_summary.txt"
    echo "Spinlock               | $(extract_summary "${RESULTS_DIR}/counter_spin.json" "spinlock")" >> "${RESULTS_DIR}/simple_summary.txt"
    echo "Shared Mutex (reads)   | $(extract_summary "${RESULTS_DIR}/rw_shared_mutex.json" "shared_mutex")" >> "${RESULTS_DIR}/simple_summary.txt"
    
    cat >> "${RESULTS_DIR}/simple_summary.txt" << EOF

=================================================================
Key Insights:
- Atomic operations typically outperform mutex for simple counters
- Spinlocks excel in low-contention, short critical sections  
- Shared mutex beneficial for read-heavy workloads (>70% reads)
- Producer-consumer patterns need actual coordination (see full mode)
=================================================================
EOF
    
    echo -e "${GREEN}Simple comparison complete!${NC}"
    cat "${RESULTS_DIR}/simple_summary.txt"
    
else
    # Full mode - run all comprehensive benchmarks
    echo -e "${BLUE}=== Full Mode: Comprehensive Analysis ===${NC}"
    
    # 1. Counter Synchronization Benchmarks
    echo -e "${BLUE}=== Counter Synchronization Benchmarks ===${NC}"
    run_benchmark "counter_mutex" "bench_counter_mutex" "" "2s"
    run_benchmark "counter_atomic" "bench_counter_atomic" "" "2s"  
    run_benchmark "counter_spin" "bench_counter_spin" "" "2s"

    # 2. Producer-Consumer Benchmarks
    echo -e "${BLUE}=== Producer-Consumer Benchmarks ===${NC}"
    run_benchmark "pc_condvar" "bench_pc_condvar" "" "2s"
    run_benchmark "pc_semaphore" "bench_pc_semaphore" "" "2s"

    # 3. Reader-Writer Benchmarks
    echo -e "${BLUE}=== Reader-Writer Benchmarks ===${NC}"
    run_benchmark "rw_shared_mutex" "bench_rw_shared_mutex" "" "2s"

    # 4. Thread Coordination Benchmarks
    echo -e "${BLUE}=== Thread Coordination Benchmarks ===${NC}"
    run_benchmark "barrier" "bench_barrier" "" "2s"

    # 5. Task Parallelism Benchmarks
    echo -e "${BLUE}=== Task Parallelism Benchmarks ===${NC}"
    run_benchmark "thread_pool" "bench_thread_pool" "" "2s"
    run_benchmark "async_future" "bench_async_future" "" "2s"

    # 6. Advanced Pattern Benchmarks
    echo -e "${BLUE}=== Advanced Pattern Benchmarks ===${NC}"
    run_benchmark "coroutines" "bench_coroutines" "" "2s"
fi

# Generate summary report from measured JSON (not canned claims)
echo -e "${BLUE}=== Generating Analysis Report ===${NC}"

python3 - "${RESULTS_DIR}" <<'PY'
import json, sys, pathlib
from datetime import datetime, timezone

results_dir = pathlib.Path(sys.argv[1])
rows = []
for path in sorted(results_dir.glob("*.json")):
    try:
        data = json.loads(path.read_text())
    except Exception as exc:
        rows.append((path.stem, f"parse error: {exc}", "", ""))
        continue
    if data.get("error"):
        rows.append((path.stem, data["error"], "", ""))
        continue
    benches = data.get("benchmarks") or []
    if not benches:
        rows.append((path.stem, "no benchmarks", "", ""))
        continue
    b = benches[0]
    name = b.get("name") or b.get("run_name") or path.stem
    ips = b.get("items_per_second")
    rt = b.get("real_time")
    ips_s = f"{ips:.3g}" if isinstance(ips, (int, float)) else "n/a"
    rt_s = f"{rt/1e6:.3f} ms" if isinstance(rt, (int, float)) else "n/a"
    rows.append((name, ips_s, rt_s, path.name))

lines = [
    "# C++ Concurrency Benchmarking Analysis Report",
    "",
    f"Generated: {datetime.now(timezone.utc).isoformat()}",
    "",
    "This report is derived from the JSON files produced in this run.",
    "It does **not** invent cross-primitive speedup claims.",
    "",
    "## Measured results (first aggregate entry per file)",
    "",
    "| Benchmark | items/s | real_time | source |",
    "|---|---:|---:|---|",
]
for name, ips, rt, src in rows:
    lines.append(f"| `{name}` | {ips} | {rt} | `{src}` |")

lines += [
    "",
    "## Interpretation guide",
    "",
    "- Compare only rows collected on the **same machine / build flags**.",
    "- Counter benches use a fixed wall-time window with per-iteration workers.",
    "- Prefer median/mean aggregates from the JSON when repetitions > 1.",
    "- Re-run with `BENCH_NATIVE_ARCH=ON` only for local exploratory tuning;",
    "  CI builds keep native arch off for reproducibility.",
    "",
]
(results_dir / "analysis_report.md").write_text("\n".join(lines) + "\n")
print(f"Wrote {results_dir / 'analysis_report.md'}")
PY

echo -e "${GREEN}Analysis complete!${NC}"
echo "Results saved to: ${RESULTS_DIR}"

if command -v python3 >/dev/null 2>&1; then
    MATRIX_OUT="${RESULTS_DIR}/decision_matrix.json"
    if python3 scripts/build_decision_matrix.py "${RESULTS_DIR}" --out "${MATRIX_OUT}"; then
        echo -e "${GREEN}Decision matrix: ${MATRIX_OUT}${NC}"
        mkdir -p matrix/generated
        cp "${MATRIX_OUT}" matrix/generated/decision_matrix.json 2>/dev/null || true
    else
        echo -e "${YELLOW}Decision matrix generation skipped/failed${NC}"
    fi
fi
echo
echo -e "${BLUE}Quick summary of files:${NC}"
find "${RESULTS_DIR}" -name "*.json" -exec basename {} \; | sort
echo "analysis_report.md"
echo
echo -e "${YELLOW}To view results:${NC}"
if [ "$MODE" = "simple" ]; then
    echo "cat ${RESULTS_DIR}/simple_summary.txt"
else
    echo "cat ${RESULTS_DIR}/analysis_report.md"
fi
echo
echo -e "${YELLOW}To compare specific benchmarks:${NC}"
echo "./scripts/compare_results.sh ${RESULTS_DIR}/counter_*.json"
echo
echo -e "${BLUE}=== Debugging Information ===${NC}"
echo "If any benchmarks timed out or failed:"
echo
echo "1. Check for hanging processes:"
echo "   ps aux | grep bench"
echo
echo "2. Review timeout logs above for patterns"
echo
echo "3. Test individual benchmarks with shorter timeouts:"
echo "   timeout 30 ./build/bench_name --benchmark_min_time=0.1s"
echo
echo "4. For hanging issues, see README.md debugging section"
echo
monitor_hanging_processes
