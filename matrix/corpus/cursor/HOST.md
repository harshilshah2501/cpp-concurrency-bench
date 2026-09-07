# Host: cursor (Cloud Agent VM)

| Field | Value |
|-------|-------|
| Role | Example **Release** corpus entry for learners |
| OS | Linux 6.12 (glibc 2.39) |
| CPUs | 4 logical @ ~2400 MHz (as reported by Google Benchmark) |
| Build | `CMAKE_BUILD_TYPE=Release`, `BENCH_NATIVE_ARCH=OFF` |
| Compiler | g++ 13.x |
| Generated | via `scripts/build_decision_matrix.py` from a curated suite run |

## How to read this

Absolute ops/s are **host-specific**. Use this file to learn the matrix *shape*
and to practice the predict→measure loop on your machine
([docs/EXERCISES.md](../../docs/EXERCISES.md)).

Do not treat these numbers as universal rankings.
