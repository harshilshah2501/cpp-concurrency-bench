# Changelog

All notable changes to the C++ Concurrency Benchmarking Suite will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Fixed
- Semaphore queue lifecycle hang across Google Benchmark iterations
- Coroutine awaiter use-after-free and unreliable completion tracking
- Counter benchmark methodology (per-iteration workers, PauseTiming, non-cumulative ops)
- Barrier arrival timing data race
- Unresolved merge conflict markers in `.gitignore` and `README.md`
- CI failures from deprecated `upload-artifact@v3` and obsolete runner matrix
- Missing `padded.hpp` includes and unimplemented `affinity::is_affinity_supported`
- Misleading thread-pool "work-stealing" documentation
- Scripts assuming Make generator / wrong binary paths
- Static canned analysis report replaced with JSON-derived summary

### Added
- `AUDIT.md` comprehensive project audit
- Shared `bench_harness.hpp` and `semaphore_queue.hpp`
- Smoke unit tests (`tests/test_smoke.cpp`) via CTest
- `BENCH_NATIVE_ARCH` CMake option (default OFF for reproducible builds)
- Google Benchmark pinned to commit SHA for tag v1.8.3

### Changed
- CI matrix to ubuntu-22.04/24.04 + macos-14; fail on benchmark timeouts
- Build uses `target_include_directories` / `target_compile_options`

### Added (historical notes carried forward)
- Comprehensive kitchen analogy system for all concurrency primitives
- Benchmark results validation proving theoretical hypotheses
- Complete debugging infrastructure for hanging benchmarks
- Timeout protection and process monitoring
- Educational documentation with real performance data

## [1.0.0] - 2024-09-24

### Added
- Initial release of C++ Concurrency Benchmarking Suite
- Core benchmarks for all major concurrency primitives:
  - Mutex, Atomic, Spinlock synchronization
  - Shared mutex reader-writer patterns
  - Condition variables and semaphores (C++20)
  - Barrier synchronization (C++20)
  - Thread pools vs async task management
  - Coroutines vs traditional threading (C++20)
- Comprehensive educational documentation
- Kitchen analogy system for intuitive understanding
- Automated benchmark execution scripts
- Performance analysis and comparison tools
- Cross-platform support (Linux, macOS, Windows)

### Technical Features
- Thread affinity support for consistent results
- Cache line padding to prevent false sharing
- Fairness measurement with coefficient of variation
- Memory ordering analysis (relaxed, acquire_release, seq_cst)
- NUMA-aware benchmarking capabilities
- JSON output format for automated analysis

### Documentation
- Complete setup and usage instructions
- Interview-ready performance insights
- Real-world use case recommendations
- Common anti-patterns and gotchas
- Troubleshooting guide for concurrency issues

---

## Version History Notes

### Versioning Strategy
- **Major** (X.0.0): New benchmark categories or breaking changes
- **Minor** (1.X.0): New benchmarks, features, or significant improvements
- **Patch** (1.0.X): Bug fixes, documentation updates, minor improvements

### Development Focus
- **Educational Value**: Each change should improve learning outcomes
- **Data-Driven**: All claims backed by real benchmark results
- **Production Ready**: Code quality suitable for professional use
- **Cross-Platform**: Support for major development environments