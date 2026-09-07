#!/usr/bin/env python3
"""Build a Concurrency Decision Matrix from Google Benchmark JSON outputs.

Only measured fields are written. Missing data becomes null / empty entries —
never invented throughput numbers.
"""

from __future__ import annotations

import argparse
import json
import platform
import re
from datetime import datetime, timezone
from pathlib import Path
from typing import Any


WORKLOAD_RULES: list[tuple[str, list[tuple[str, str, re.Pattern[str]]]]] = [
    (
        "W.counter.hot",
        [
            ("mutex", "Counter_Mutex_Hot", re.compile(r"Counter_Mutex_Hot")),
            ("atomic_relaxed", "Counter_Atomic_Relaxed", re.compile(r"Counter_Atomic_Relaxed")),
            ("spinlock", "Counter_Spin_Hot", re.compile(r"Counter_Spin_Hot")),
        ],
    ),
    (
        "W.atomic.mo",
        [
            ("seq_cst", "Counter_Atomic_SeqCst", re.compile(r"Counter_Atomic_SeqCst")),
            ("acq_rel", "Counter_Atomic_AcqRel", re.compile(r"Counter_Atomic_AcqRel")),
            ("relaxed", "Counter_Atomic_Relaxed", re.compile(r"Counter_Atomic_Relaxed")),
        ],
    ),
    (
        "W.rwlock.read_heavy",
        [
            ("shared_mutex", "SharedMutex_ReadHeavy_90_10", re.compile(r"SharedMutex_ReadHeavy")),
            ("exclusive_mutex", "ExclusiveMutex_ReadHeavy_90_10", re.compile(r"ExclusiveMutex_ReadHeavy")),
        ],
    ),
    (
        "W.pc.bounded",
        [
            ("condvar", "PC_CondVar", re.compile(r"PC_CondVar")),
            ("semaphore_spsc", "ProducerConsumer_SPSC_Semaphore", re.compile(r"ProducerConsumer_SPSC")),
        ],
    ),
    (
        "W.barrier.phases",
        [
            ("std_barrier", "StdBarrier_WithWork", re.compile(r"StdBarrier_WithWork")),
            ("manual_barrier", "ManualBarrier_WithWork", re.compile(r"ManualBarrier_WithWork")),
        ],
    ),
    (
        "W.tasks.cpu_many",
        [
            ("thread_pool", "ThreadPool_CPUBound", re.compile(r"ThreadPool_CPUBound")),
            ("std_async", "StdAsync_CPUBound", re.compile(r"StdAsync_CPUBound")),
        ],
    ),
    (
        "W.coro.io_many",
        [
            ("coroutines_io", "Coroutines_IOBound", re.compile(r"Coroutines_IOBound")),
        ],
    ),
]


def load_benchmarks(results_dir: Path) -> list[tuple[str, dict[str, Any], dict[str, Any]]]:
    rows: list[tuple[str, dict[str, Any], dict[str, Any]]] = []
    for path in sorted(results_dir.glob("*.json")):
        try:
            data = json.loads(path.read_text())
        except json.JSONDecodeError:
            continue
        if data.get("error"):
            continue
        context = data.get("context") or {}
        for bench in data.get("benchmarks") or []:
            name = bench.get("name") or bench.get("run_name") or ""
            # Prefer aggregate mean rows when present.
            if name.endswith("_stddev") or name.endswith("_cv") or name.endswith("_median"):
                continue
            rows.append((path.name, context, bench))
    return rows


def pick_best(rows: list[tuple[str, dict[str, Any], dict[str, Any]]], pattern: re.Pattern[str]):
    matches = [(f, c, b) for f, c, b in rows if pattern.search(b.get("name") or b.get("run_name") or "")]
    if not matches:
        return None
    # Prefer *_mean aggregates, else first match.
    for item in matches:
        name = item[2].get("name") or ""
        if name.endswith("_mean"):
            return item
    return matches[0]


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("results_dir", type=Path, help="Directory of Google Benchmark JSON files")
    parser.add_argument("--out", type=Path, required=True, help="Output matrix JSON path")
    parser.add_argument("--host", default=platform.node() or "unknown")
    parser.add_argument("--native-arch", action="store_true", default=False)
    args = parser.parse_args()

    if not args.results_dir.is_dir():
        raise SystemExit(f"Not a directory: {args.results_dir}")

    rows = load_benchmarks(args.results_dir)
    warnings: list[str] = []
    if not rows:
        warnings.append("No parseable benchmark JSON found")

    host_meta = {"name": args.host, "os": platform.platform(), "num_cpus": None, "mhz_per_cpu": None}
    build_type = None
    if rows:
        ctx = rows[0][1]
        host_meta["num_cpus"] = ctx.get("num_cpus")
        host_meta["mhz_per_cpu"] = ctx.get("mhz_per_cpu")
        build_type = ctx.get("library_build_type")
        if build_type and str(build_type).lower() == "debug":
            warnings.append("Results appear to be from a Debug build — do not cite as Release baseline")

    workloads = []
    for workload_id, primitives in WORKLOAD_RULES:
        entries = []
        for primitive, label, pattern in primitives:
            hit = pick_best(rows, pattern)
            if hit is None:
                entries.append(
                    {
                        "primitive": primitive,
                        "benchmark": label,
                        "items_per_second": None,
                        "real_time_ns": None,
                        "fairness_cv": None,
                        "source_file": None,
                    }
                )
                continue
            src, _ctx, bench = hit
            counters = bench.get("counters") or {}
            entries.append(
                {
                    "primitive": primitive,
                    "benchmark": bench.get("name") or label,
                    "items_per_second": bench.get("items_per_second"),
                    "real_time_ns": bench.get("real_time"),
                    "fairness_cv": counters.get("fairness_cv"),
                    "source_file": src,
                }
            )
        workloads.append({"id": workload_id, "entries": entries})

    matrix = {
        "schema_version": "1.0.0",
        "generated_at": datetime.now(timezone.utc).isoformat(),
        "host": host_meta,
        "build": {
            "build_type": build_type,
            "native_arch": bool(args.native_arch),
            "notes": f"Generated from {args.results_dir}",
        },
        "workloads": workloads,
        "warnings": warnings,
    }

    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(json.dumps(matrix, indent=2) + "\n")
    print(f"Wrote {args.out}")
    for w in warnings:
        print(f"warning: {w}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
