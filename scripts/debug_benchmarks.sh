#!/bin/bash

# C++ Concurrency Benchmarking - Debug Helper Script
# This script helps debug hanging or problematic benchmarks

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

echo -e "${BLUE}=== C++ Concurrency Benchmark Debugger ===${NC}"
echo

# Function to check for hanging processes
check_hanging_processes() {
    echo -e "${YELLOW}1. Checking for hanging benchmark processes...${NC}"
    local hanging_procs=$(ps aux | grep "bench_" | grep -v grep | grep -v "debug_benchmarks")
    
    if [ -n "$hanging_procs" ]; then
        echo -e "${RED}Found hanging benchmark processes:${NC}"
        echo "$hanging_procs"
        echo
        
        read -p "Kill these processes? (y/N): " -n 1 -r
        echo
        if [[ $REPLY =~ ^[Yy]$ ]]; then
            pkill -f "bench_"
            echo -e "${GREEN}✓ Processes killed${NC}"
        fi
    else
        echo -e "${GREEN}✓ No hanging processes found${NC}"
    fi
    echo
}

# Function to test individual benchmarks
test_individual_benchmarks() {
    echo -e "${YELLOW}2. Testing individual benchmarks with timeout...${NC}"
    
    local build_dir="build"
    local timeout_duration=30
    
    for executable in ${build_dir}/bench_*; do
        if [ -f "$executable" ]; then
            local bench_name=$(basename "$executable")
            echo -e "${BLUE}Testing ${bench_name}...${NC}"
            
            # List available benchmarks
            echo "  Available benchmarks:"
            timeout 5 "$executable" --benchmark_list_tests 2>/dev/null | head -5 | sed 's/^/    /'
            
            # Test first benchmark with short timeout
            local first_bench=$(timeout 5 "$executable" --benchmark_list_tests 2>/dev/null | head -1)
            if [ -n "$first_bench" ]; then
                echo "  Testing: $first_bench"
                timeout $timeout_duration "$executable" --benchmark_filter="$first_bench" --benchmark_min_time=0.1s >/dev/null 2>&1
                
                local exit_code=$?
                if [ $exit_code -eq 0 ]; then
                    echo -e "    ${GREEN}✓ OK${NC}"
                elif [ $exit_code -eq 124 ]; then
                    echo -e "    ${RED}✗ TIMEOUT (likely hanging)${NC}"
                else
                    echo -e "    ${YELLOW}! Error (exit code: $exit_code)${NC}"
                fi
            else
                echo -e "    ${YELLOW}! Could not list benchmarks${NC}"
            fi
        fi
    done
    echo
}

# Function to analyze system resources
check_system_resources() {
    echo -e "${YELLOW}3. System Resource Analysis...${NC}"
    
    echo "CPU cores: $(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 'unknown')"
    echo "Load average: $(uptime | awk -F'load average:' '{print $2}')"
    
    # Check memory usage
    if command -v free >/dev/null 2>&1; then
        echo "Memory usage:"
        free -h | grep -E "(Mem|Swap)" | sed 's/^/  /'
    elif command -v vm_stat >/dev/null 2>&1; then
        echo "Memory pressure (macOS):"
        vm_stat | head -6 | sed 's/^/  /'
    fi
    
    # Check for high CPU processes
    echo "Top CPU consumers:"
    if command -v top >/dev/null 2>&1; then
        top -b -n1 2>/dev/null | head -10 | tail -5 | sed 's/^/  /' || echo "  (top command failed)"
    else
        ps aux --sort=-%cpu 2>/dev/null | head -6 | tail -5 | sed 's/^/  /' || echo "  (ps command failed)"
    fi
    echo
}

# Function to provide recommendations
provide_recommendations() {
    echo -e "${YELLOW}4. Debugging Recommendations:${NC}"
    echo
    echo -e "${GREEN}For hanging benchmarks:${NC}"
    echo "  • Check producer-consumer coordination logic"
    echo "  • Look for missing shutdown() calls"
    echo "  • Verify semaphore/condition variable cleanup"
    echo "  • Add timeout mechanisms to infinite loops"
    echo
    echo -e "${GREEN}For performance issues:${NC}"
    echo "  • Build in Release mode: cmake -DCMAKE_BUILD_TYPE=Release"
    echo "  • Check for false sharing (use padded<> types)"
    echo "  • Verify memory ordering is appropriate"
    echo "  • Consider NUMA topology for multi-socket systems"
    echo
    echo -e "${GREEN}For inconsistent results:${NC}"
    echo "  • Disable frequency scaling: sudo cpupower frequency-set --governor performance"
    echo "  • Set thread affinity: numactl --cpunodebind=0 --membind=0"
    echo "  • Close other applications to reduce noise"
    echo "  • Run multiple iterations and look for variance patterns"
    echo
    echo -e "${GREEN}Emergency procedures:${NC}"
    echo "  • Kill all benchmarks: pkill -f 'bench_'"
    echo "  • Check system logs: dmesg | tail"
    echo "  • Monitor with: watch 'ps aux | grep bench'"
    echo
}

# Run all debug functions
main() {
    check_hanging_processes
    test_individual_benchmarks
    check_system_resources
    provide_recommendations
    
    echo -e "${BLUE}=== Debug Analysis Complete ===${NC}"
    echo "For more detailed analysis, check the README.md debugging section."
}

# Allow running individual functions
if [ $# -eq 0 ]; then
    main
else
    case "$1" in
        "hanging"|"processes") check_hanging_processes ;;
        "test"|"individual") test_individual_benchmarks ;;
        "system"|"resources") check_system_resources ;;
        "help"|"recommendations") provide_recommendations ;;
        *) 
            echo "Usage: $0 [hanging|test|system|help]"
            echo "  hanging     - Check for hanging processes"
            echo "  test        - Test individual benchmarks"
            echo "  system      - Analyze system resources"
            echo "  help        - Show recommendations"
            echo "  (no args)   - Run full debug analysis"
            ;;
    esac
fi