🎯 Critical Interview Points
1. Why Atomic Relaxed is 3x Faster (Single Thread)
```bash
// Relaxed: No memory barriers, just atomic read-modify-write
counter.fetch_add(1, std::memory_order_relaxed);

// SeqCst: Full memory barrier, global ordering guarantee
counter.fetch_add(1);  // defaults to seq_cst
```

2. Why Performance Collapses with Multiple Threads
Cache line bouncing: Counter lives on single cache line
Memory bus saturation: All cores competing for same memory location
False sharing prevention: Our padding helps, but can't eliminate core contention
3. Fairness Observation
Notice fairness_cv=0 - this indicates perfect fairness in these short tests, but real applications might show different patterns.