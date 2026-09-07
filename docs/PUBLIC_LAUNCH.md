# Public launch checklist

Use this before flipping the GitHub repository to **public**.

## P0 — trust

- [ ] README contains **no** fabricated “Actual Results” / impossible memory tables
- [ ] Methodology doc linked from README
- [ ] Remediation merged; **CI green on `main`**
- [ ] Known-issues section matches reality (semaphore hang fixed)
- [ ] Platform claims: Linux/macOS only unless Windows CI exists

## P1 — packaging

- [ ] Repository visibility: public
- [ ] Topics: `cpp`, `cpp20`, `concurrency`, `benchmark`, `education`, `multithreading`
- [ ] Description one-liner matches README (no “guaranteed production decisions”)
- [ ] Named copyright in `LICENSE`
- [ ] Release tag (e.g. `v1.1.0`) with notes pointing at `AUDIT.md` + methodology
- [ ] Optional: GitHub Pages or `matrix/corpus/` with 1–2 real host matrices

## P2 — differentiation

- [ ] At least one regenerated `matrix/generated/decision_matrix.json` from CI or a documented host
- [ ] Short blog/paper draft citing **only** measured cells
- [ ] Workshop outline (kitchen curriculum) if pursuing education wedge

## Do not ship

- Invented GB-scale coroutine vs thread tables
- Cross-machine averages without per-host labels
- “All hypotheses validated” banners
