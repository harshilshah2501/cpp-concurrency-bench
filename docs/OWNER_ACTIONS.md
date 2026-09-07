# Owner actions (GitHub UI)

The agent cannot change repository visibility, topics, description, or releases
(API returns 403). Do these after merging PR #3:

## 1. Merge

Merge **https://github.com/harshilshah2501/cpp-concurrency-bench/pull/3** into `main`.
Close #1 and #2 as superseded.

## 2. Repository settings

**About** (gear on repo home):

- Description: `C++20 concurrency benchmarks with kitchen analogies — measure locally, learn trade-offs`
- Topics: `cpp` `cpp20` `concurrency` `benchmark` `education` `multithreading`
- Homepage (optional): leave empty or point at `docs/METHODOLOGY.md` blob URL

**Danger Zone / General:**

- Change visibility → **Public**

## 3. Release

Create release tag `v1.1.0` from `main` with notes:

- Correctness + CI remediation
- Honest docs / decision matrix / learning path + exercises
- Link `docs/METHODOLOGY.md`, `docs/DECISION_MATRIX.md`, `docs/EXERCISES.md`, `AUDIT.md`

## 4. Verify

- Actions green on `main`
- Clone works anonymously
- README learning path runs on a clean machine
