# Public launch checklist

Track what is done in-repo vs what only the repo owner can click in GitHub.

## Done in repository (PR #3 + follow-ups)

- [x] README contains **no** fabricated “Actual Results” / impossible memory tables
- [x] Methodology doc linked from README (`docs/METHODOLOGY.md`)
- [x] Known-issues section matches reality (semaphore hang fixed)
- [x] Platform claims: Linux/macOS first-class
- [x] Named copyright in `LICENSE`
- [x] Learning path in README + `docs/EXERCISES.md`
- [x] Decision matrix schema/generator + sample `matrix/generated/`
- [x] Real host corpus entry: `matrix/corpus/cursor/`
- [x] Owner click-path documented: `docs/OWNER_ACTIONS.md`
- [x] **CI green on `main`** (merge of PR #3)
- [x] Repository visibility: **public**
- [x] Release tag `v1.1.0`
- [x] Anonymous HTTPS clone / public tarball download works
- [x] README learning path runs on a clean machine (Days 1–3)
- [x] Thread-pool bench no longer hangs under repeated create/join

## Owner actions (GitHub UI — see `docs/OWNER_ACTIONS.md`)

- [ ] Close superseded PRs #1/#2 if still open
- [ ] Topics: `cpp` `cpp20` `concurrency` `benchmark` `education` `multithreading`
- [ ] Description: `C++20 concurrency benchmarks with kitchen analogies — measure locally, learn trade-offs`

## Optional after launch

- [ ] Short blog citing **only** measured corpus cells
- [ ] Second host matrix (e.g. Apple Silicon) under `matrix/corpus/`
- [ ] Workshop slides from the learning path

## Do not ship

- Invented GB-scale coroutine vs thread tables
- Cross-machine averages without per-host labels
- “All hypotheses validated” banners
