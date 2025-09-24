---
name: Bug Report
about: Report a bug in the benchmark suite
title: '[BUG] '
labels: ['bug']
assignees: ''
---

## 🐛 Bug Description
A clear and concise description of the bug.

## 🔄 Steps to Reproduce
1. Build configuration used: `cmake -DCMAKE_BUILD_TYPE=...`
2. Command that caused the issue: `./scripts/run_all_benchmarks.sh ...`
3. Specific benchmark: `./build/bench_...`
4. Any special environment setup

## ⚡ Expected vs Actual Behavior
- **Expected**: What should happen
- **Actual**: What actually happens

## 💾 System Information
- **OS**: [e.g., Ubuntu 22.04, macOS 13, Windows 11]
- **Compiler**: [e.g., GCC 11, Clang 14, MSVC 2022]
- **CPU**: [e.g., Intel i7-12700K, AMD Ryzen 5950X, Apple M2]
- **Memory**: [e.g., 32GB DDR4-3200]
- **Cores**: [e.g., 8 cores / 16 threads]

## 📋 Benchmark Output
```
Paste relevant benchmark output, error messages, or logs here
```

## 🔍 Additional Context
- Is this a **hanging benchmark**? (Check our debugging guide)
- Does it happen consistently or intermittently?
- Any relevant performance monitoring data
- Screenshots if GUI-related

## 🏥 Debugging Information
If benchmark hangs:
- [ ] Process ID from `ps aux | grep bench`
- [ ] CPU usage pattern (high CPU = busy-wait, low CPU = deadlock)
- [ ] Did you follow the debugging checklist in README.md?

## 🎯 Priority
- [ ] Critical (benchmark suite doesn't work)
- [ ] High (affects core functionality)
- [ ] Medium (affects specific use cases)
- [ ] Low (minor issue or enhancement)