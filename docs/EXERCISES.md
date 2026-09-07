# Exercises (predict → measure → explain)

Use these after the [README learning path](../README.md#learning-path-recommended).
Always build **Release** with `-DBENCH_NATIVE_ARCH=OFF` for comparable runs.

For each exercise:

1. Write your prediction in one sentence.
2. Run the listed command.
3. Record one number (or a small table) from the output.
4. Explain *why* — cache lines, blocking, fairness, or scheduling.

## E1 — Counter showdown (Day 1)

**Predict:** At 1 thread, rank mutex / atomic relaxed / spinlock by throughput.

```bash
./build/bench_counter_mutex  --benchmark_filter='Counter_Mutex_Hot/1/'  --benchmark_min_time=0.5s
./build/bench_counter_atomic --benchmark_filter='Counter_Atomic_Relaxed/1/' --benchmark_min_time=0.5s
./build/bench_counter_spin   --benchmark_filter='Counter_Spin_Hot/1/'   --benchmark_min_time=0.5s
```

**Follow-up:** Repeat with the highest thread count your machine reports. Who loses the most? Why?

## E2 — Memory order cost (Day 1)

**Predict:** Relative order of relaxed vs acq_rel vs seq_cst under contention.

```bash
./build/bench_counter_atomic --benchmark_filter='Counter_Atomic_' --benchmark_min_time=0.5s
```

**Explain:** What guarantee are you paying for with seq_cst that this microbench barely needs?

## E3 — When spinlocks hurt (Day 1)

**Predict:** Spinlock beats mutex only for tiny critical sections.

```bash
./build/bench_counter_spin --benchmark_filter='Counter_Spin_' --benchmark_min_time=0.5s
```

Compare `Counter_Spin_Hot` vs `Counter_Spin_WithWork` as threads increase. Where does spinning waste cores?

## E4 — Read-heavy maps (Day 2)

**Predict:** Shared mutex beats exclusive mutex at ~90% reads.

```bash
./build/bench_rw_shared_mutex --benchmark_filter='ReadHeavy_90_10' --benchmark_min_time=0.5s
```

**Twist:** Run the 50/50 case. Does the win shrink or reverse? What about writer latency (not just throughput)?

## E5 — Bell vs tokens (Day 2)

**Predict:** For a simple bounded queue, semaphore and condition variable are close; CV wins on complex predicates.

```bash
./build/bench_pc_condvar   --benchmark_min_time=0.5s
./build/bench_pc_semaphore --benchmark_filter='ProducerConsumer_SPSC_Semaphore/100/' --benchmark_min_time=0.5s
```

## E6 — Pool vs async (Day 3)

**Predict:** Thread pool wins for many tiny CPU tasks; `std::async` pays thread-create costs.

```bash
./build/bench_thread_pool --benchmark_filter='ThreadPool_CPUBound/.*/real_time$|StdAsync_CPUBound/.*/real_time$' --benchmark_min_time=0.05s
```

## E7 — Build your matrix (capstone)

```bash
./scripts/run_all_benchmarks.sh simple
python3 scripts/build_decision_matrix.py benchmark_results/<timestamp> \
  --out matrix/corpus/$(uname -n | tr ' /' '__')/decision_matrix.json
```

Fill `HOST.md` next to it (CPU model, OS, compiler). Compare one cell to [matrix/corpus/cursor/](../matrix/corpus/cursor/) — same shape, different absolute numbers is expected.

## Facilitator notes

- Wrong predictions are valuable; grade the *reasoning*, not matching a golden ops/s.
- Never ask learners to cite README folklore tables (there are none by design).
- Prefer fairness_cv discussions when comparing spinlocks under oversubscription.
