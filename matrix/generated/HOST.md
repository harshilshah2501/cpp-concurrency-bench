# Example generated matrix (CI-like cloud host)

- Build: Release, `BENCH_NATIVE_ARCH=OFF`
- Workloads populated: `W.counter.hot` (partial suite smoke)
- Other workload IDs intentionally have null/empty entries until full suite is run

Regenerate with:

```bash
python3 scripts/build_decision_matrix.py benchmark_results/<timestamp> \
  --out matrix/generated/decision_matrix.json
```
