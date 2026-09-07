# Public launch checklist

Track what is done in-repo vs what only the repo owner can click in GitHub.

## Done in repository (PR #3)

- [x] README contains **no** fabricated “Actual Results” / impossible memory tables
- [x] Methodology doc linked from README (`docs/METHODOLOGY.md`)
- [x] Known-issues section matches reality (semaphore hang fixed)
- [x] Platform claims: Linux/macOS first-class
- [x] Named copyright in `LICENSE`
- [x] Learning path in README + `docs/EXERCISES.md`
- [x] Decision matrix schema/generator + sample `matrix/generated/`
- [x] Real host corpus entry: `matrix/corpus/cursor/`
- [x] Owner click-path documented: `docs/OWNER_ACTIONS.md`
- [x] CI green on polish branch (merge to `main` still required)

## Owner actions (GitHub UI — see `docs/OWNER_ACTIONS.md`)

- [ ] Merge PR #3 into `main`; close #1/#2
- [ ] Confirm **CI green on `main`**
- [ ] Repository visibility: **public**
- [ ] Topics: `cpp` `cpp20` `concurrency` `benchmark` `education` `multithreading`
- [ ] Description: `C++20 concurrency benchmarks with kitchen analogies — measure locally, learn trade-offs`
- [ ] Release tag `v1.1.0` with links to methodology / exercises / AUDIT

## Optional after launch

- [ ] Short blog citing **only** measured corpus cells
- [ ] Second host matrix (e.g. Apple Silicon) under `matrix/corpus/`
- [ ] Workshop slides from the learning path

## Do not ship

- Invented GB-scale coroutine vs thread tables
- Cross-machine averages without per-host labels
- “All hypotheses validated” banners
