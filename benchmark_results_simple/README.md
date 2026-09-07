# Historical / sample results (not a public corpus)

These directories may contain **Debug** builds, truncated JSON, or incomplete suites.

**Do not cite these numbers** in articles, interviews, or papers.

For publishable data:

1. Build Release with `-DBENCH_NATIVE_ARCH=OFF`
2. Run `./scripts/run_all_benchmarks.sh simple` (or `full`)
3. Run `python3 scripts/build_decision_matrix.py benchmark_results/<timestamp> --out matrix/generated/decision_matrix.json`
4. Optionally commit under `matrix/corpus/<hostname>/` with a short `HOST.md` describing CPU/OS
