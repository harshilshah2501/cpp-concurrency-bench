---
name: Performance Issue
about: Report unexpected benchmark performance or results
title: '[PERFORMANCE] '
labels: ['performance']
assignees: ''
---

## ⚡ Performance Issue Summary
Brief description of the unexpected performance behavior.

## 📊 Benchmark Results
### Current Results
```
Paste your benchmark output here
```

### Expected Results
What performance characteristics did you expect based on:
- [ ] Theoretical analysis
- [ ] Our documented hypotheses
- [ ] Previous runs on similar hardware
- [ ] Literature/research papers

## 🔧 System Configuration
### Hardware
- **CPU**: [e.g., Intel i7-12700K @ 3.6GHz]
- **Cores/Threads**: [e.g., 8P+4E cores, 20 threads]
- **Memory**: [e.g., 32GB DDR4-3200 dual channel]
- **Cache**: [e.g., L1: 32KB, L2: 1.25MB, L3: 25MB]
- **NUMA Topology**: [e.g., single socket, 2 nodes]

### Software
- **OS**: [e.g., Ubuntu 22.04.3 LTS]
- **Kernel**: [e.g., Linux 6.2.0-26-generic]
- **Compiler**: [e.g., GCC 11.4.0 with -O3 -march=native]
- **Build Type**: [e.g., Release with CMAKE_BUILD_TYPE=Release]

### Environment
- **CPU Scaling**: [e.g., performance governor]
- **Thread Affinity**: [e.g., pinned vs unpinned]
- **System Load**: [e.g., idle system vs background processes]
- **Hyperthreading**: [e.g., enabled/disabled]

## 📈 Performance Analysis
### Scaling Behavior
How does performance change with:
- [ ] Thread count (1, 2, 4, 8, 16+ threads)
- [ ] Contention level (light, moderate, heavy)
- [ ] Data size (cache-friendly vs cache-unfriendly)
- [ ] Memory access patterns

### Unexpected Observations
- [ ] Counter-intuitive results (e.g., more threads = worse performance)
- [ ] Non-linear scaling
- [ ] High variance in results
- [ ] Platform-specific differences

## 🎯 Kitchen Analogy Context
Using our restaurant kitchen theme, how does this performance issue translate?

Example: *"The digital order counter (atomic) is slower than expected when 16 chefs try to use it simultaneously - like the counter machine is getting overwhelmed"*

## 🔍 Debugging Steps Taken
- [ ] Ran multiple times to confirm reproducibility
- [ ] Tested with different thread counts
- [ ] Checked system resource usage during benchmark
- [ ] Compared with simpler benchmark variants
- [ ] Profiled with perf/instruments/other tools

### Profiling Data (if available)
```
Paste relevant perf/profiler output here
```

## 🤔 Hypothesis
What do you think might be causing this performance issue?

- [ ] Cache line bouncing / false sharing
- [ ] Memory bandwidth saturation
- [ ] NUMA effects
- [ ] Scheduler behavior
- [ ] Compiler optimization issues
- [ ] Hardware-specific bottlenecks
- [ ] Benchmark implementation problems

## 📋 Comparison Data
If you have comparison data from:
- [ ] Different hardware
- [ ] Different compilers
- [ ] Different OS versions
- [ ] Reference implementations

## 🎯 Impact Assessment
- [ ] Affects educational value of benchmark
- [ ] Contradicts documented hypotheses  
- [ ] Impacts production decision making
- [ ] Platform-specific issue
- [ ] General performance regression