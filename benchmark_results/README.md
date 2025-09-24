# Benchmark Results Directory

This directory stores benchmark execution results organized by timestamp.

## Structure
```
benchmark_results/
├── YYYYMMDD_HHMMSS/     # Timestamp-based directories
│   ├── *.json           # Individual benchmark results
│   └── analysis_report.md # Generated analysis (if available)
└── README.md           # This file
```

## Usage

Results are automatically generated when running benchmarks:

```bash
# Full benchmark suite
./scripts/run_all_benchmarks.sh full

# Results saved to benchmark_results/$(date +%Y%m%d_%H%M%S)/
```

## Cleanup

Old results can be archived to save space:

```bash
# Archive results older than 7 days
find benchmark_results/ -name "202*" -mtime +7 -type d -exec tar -czf archive/old_results_$(date +%Y%m%d).tar.gz {} \; -exec rm -rf {} \;
```

## Analysis

Use the comparison scripts to analyze results:

```bash
# Compare different synchronization primitives
./scripts/compare_results.sh benchmark_results/TIMESTAMP1 benchmark_results/TIMESTAMP2
```