# Concurrency Decision Matrix

The unique research/education artifact for this project: a **machine-local,
schema-validated matrix** mapping workload shapes to measured primitive performance.

## Why this exists

Most concurrency write-ups either:

- tell stories without runnable code, or
- publish one-off microbench numbers without fairness or hardware tags.

This matrix is meant to be **regenerable**, **comparable**, and **honest** —
empty cells stay empty until you measure them.

## Workload catalog (stable IDs)

| ID | Shape | Primary benches |
|----|-------|-----------------|
| `W.counter.hot` | Contended counter | `bench_counter_*` |
| `W.atomic.mo` | Memory-order sweep | `bench_counter_atomic` |
| `W.rwlock.read_heavy` | ~90% reads | `bench_rw_shared_mutex` |
| `W.pc.bounded` | Producer/consumer | `bench_pc_*` |
| `W.barrier.phases` | Bulk sync | `bench_barrier` |
| `W.tasks.cpu_many` | Many small tasks | `bench_thread_pool`, `bench_async_future` |
| `W.coro.io_many` | Many delayed tasks | `bench_coroutines` |

## Qualitative guidance (not measurements)

Use this only as a starting heuristic; confirm on your hardware:

| Workload | Start with | Be careful with |
|----------|------------|-----------------|
| Hot simple counter | `atomic` (relaxed if sound) | Coarse mutex; spinning under oversubscription |
| Complex critical section | `mutex` | Atomics bolted onto rich invariants |
| Ultra-short CS + spare cores | spinlock | Spinlocks when threads &gt; cores |
| Read-mostly shared state | `shared_mutex` | Writer starvation; write-heavy mixes |
| Counted resources / slots | semaphore | Complex predicates (prefer CV) |
| Phase-parallel algorithms | `barrier` | Over-synchronizing tiny phases |
| Many tiny CPU tasks | thread pool | Unbounded `std::async` storms |
| Huge concurrent waiters | coroutines / async IO | One OS thread per waiter |

## Generate your matrix

```bash
./scripts/run_all_benchmarks.sh simple
python3 scripts/build_decision_matrix.py benchmark_results/<timestamp> \
  --host "$(uname -n)" \
  --out matrix/generated/decision_matrix.json
```

Schema: [`matrix/schema.json`](../matrix/schema.json).
Example empty template: [`matrix/templates/empty_matrix.json`](../matrix/templates/empty_matrix.json).

## Research / product path

1. Collect matrices from several CPU families (publish under `matrix/corpus/` with consent).
2. Add fairness + throughput panels per workload ID.
3. Optional: GitHub Action that diffs a PR’s matrix against a pinned corpus baseline.
