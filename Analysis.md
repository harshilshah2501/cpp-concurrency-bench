# Analysis notes

Personal scratch notes for interview talking points. **Not** a results publication.

For citable numbers, follow [docs/METHODOLOGY.md](docs/METHODOLOGY.md) and generate a
decision matrix with `scripts/build_decision_matrix.py`.

## Talking points (qualitative)

1. **Why relaxed atomics can beat seq_cst (single thread / low contention)**  
   Fewer barriers / weaker ordering; still an atomic RMW.

2. **Why contended counters collapse**  
   Cache-line bouncing and memory interconnect traffic dominate lock choice.

3. **Fairness**  
   `fairness_cv ≈ 0` on short windows can look “perfect”; longer runs and
   preemption expose spinlock / RW-lock unfairness — measure before claiming.
