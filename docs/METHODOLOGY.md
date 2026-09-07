# Benchmark methodology

This document defines how numbers from this repository should be produced and cited.
**Do not cite illustrative teaching tables as measured results.**

## What we measure

| Metric | Meaning |
|--------|---------|
| Throughput | Operations or items per second (`items_per_second` from Google Benchmark) |
| Fairness CV | Coefficient of variation of per-thread op counts (lower is fairer) |
| Wall time | Real-time duration of a workload window or task batch |

Counter benches use a fixed wall-time contention window with **per-iteration workers**
(see `include/bench_harness.hpp`). Setup/teardown is excluded via `PauseTiming` where applicable.

## Required build flags for comparable runs

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBENCH_NATIVE_ARCH=OFF
cmake --build build --parallel
```

- `Release` + `BENCH_NATIVE_ARCH=OFF` is the **public baseline** (matches CI).
- `-DBENCH_NATIVE_ARCH=ON` is allowed for local exploration only; label those runs `native`.

## Environment fingerprint (always record)

Capture at least:

- OS and kernel (`uname -a`)
- CPU model and logical core count
- Compiler id/version (`${CXX} --version`)
- CMake build type and `BENCH_NATIVE_ARCH`
- Google Benchmark commit (pinned in `CMakeLists.txt`)
- Whether frequency scaling / turbo was uncontrolled (assume yes unless documented)

Google Benchmark JSON already includes `context.num_cpus`, `mhz_per_cpu`, `host_name`, `date`, and cache info — keep the full JSON.

## How to generate publishable results

```bash
./scripts/run_all_benchmarks.sh simple   # quick
./scripts/run_all_benchmarks.sh full     # broader
python3 scripts/build_decision_matrix.py benchmark_results/<timestamp> \
  --out matrix/generated/decision_matrix.json
```

The generator writes only values present in JSON. Missing benches become `null`, never invented.

## Citation rules

1. Quote **machine + build flags + suite timestamp** with every number.
2. Prefer median aggregates when repetitions &gt; 1.
3. Do not average across different hosts without labeling each.
4. Teaching analogies in the README are **intuition aids**, not evidence.

## Platforms

Supported for CI and scripts today: **Linux and macOS**.
Windows may build some targets with MSVC/Clang-cl, but affinity helpers and shell scripts are Unix-oriented and are **not** claimed as fully supported.
