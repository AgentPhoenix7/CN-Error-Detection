# Sampling, Memory, and Build Repair Implementation Plan

## Current scope and assignment audit

The three scoped repairs below are complete. They do not certify full
assignment compliance. See [the current requirements audit](../../assignment-compliance.md)
for the accepted 64--1518-byte complete-frame range, the explicit user
requirement to retain one-byte sequence numbering, the separate 46-byte
minimum-payload gap, unresolved length/polynomial-input interpretation, and
pending report formatting. No separate report-format rules are available.

The sender's mixed-pattern dispatcher is tested in
`C/tests/test_sender_random.c`; it calls the public bounded RNG helper. A later
include cleanup extracted it to `C/src/sender_errors.c`, declared by
`C/include/sender_errors.h`, so the test links the dispatcher normally rather
than including the entire sender application and renaming its main function.
There is no separate `error_injection_flip_random_pattern()` API. The current receiver
parses and FCS-protects sequence metadata but has no continuity/ARQ state machine.

> **For agentic workers:** Use superpowers:executing-plans to implement this plan task by task.

**Goal:** Fix the three reproduced review findings and regenerate trustworthy evaluation artifacts.

**Architecture:** Retain the caller-owned seeded LCG, but map its full output to bounded choices using multiply-high rejection sampling. Preserve nonadjacent two-bit errors and use Floyd sampling for odd subsets. Read Linux post-exec peak RSS in a shared C module, and compile objects with generated dependency files.

**Tech Stack:** C17, GCC, POSIX sockets, Linux procfs, Python standard library through uv, GNU Make.

**Spec:** The user's November 13 request authorizes fixes to biased sampling, launcher-contaminated RSS, and stale header builds, and asks whether the supplied replacement fits the project.

## Global Constraints

- Preserve C17, two-space indentation, existing CLI and wire format.
- Keep invalid mutations atomic for data and RNG state.
- Keep reproducible seeds; document changed position vectors.
- Retain separate sender/receiver CPU and memory measurements.

## Review Focus

- Two isolated flips must never be adjacent or share an index.
- Bounded choices must reject unsupported sizes before advancing RNG state.
- Mixed random errors must exercise all four patterns.
- Memory results must remain independent of a large Python parent allocation.
- Report comparisons must handle real checksum misses without inventing per-frame intersections.

## Tasks

### 1. Sampling

- [x] Add and run a failing regression for two-bit parity/weight coverage and mixed pattern coverage.
- [x] Add checked `error_injection_rng_bounded()`, use it in the sender's existing mixed-pattern dispatcher, bound random input sizes, sample nonadjacent pairs, and use Floyd odd-subset sampling.
- [x] Update deterministic vectors, rejection/atomicity cases, and sender delegation; run affected C checks.

### 2. Memory and builds

- [x] Add failing regression tests for parent-memory independence and header-triggered recompilation.
- [x] Add `process_metrics_peak_rss_kb(long *)` using Linux `/proc/self/status` VmHWM, and integrate both programs.
- [x] Compile per-source objects with `-MMD -MP`, include `.d` files, and keep smoke outputs separate from recorded results.
- [x] Run strict build and the complete test suite.

### 3. Evaluation and documentation

- [x] Cover checksum misses in required comparison calculations; derive intersection bounds from paired aggregate marginals rather than assuming one scheme detects everything.
- [x] Regenerate the 450-run campaign, plots, and report, and update README and PROGRESS.
- [x] Verify control transfers, distribution regressions, parent-memory independence, header rebuilds, CSV invariants, report/plot reproducibility, static analysis, and diff cleanliness.

## Completion evidence

- 103 C checks and 14 Python tests passed, plus both socket smoke targets.
- Full 450-run campaign regenerated; reports and plots reproduce byte for byte.
- Original two-bit probe now covers matching checksum weights in 627/10,000 trials; random pattern counts are 245/257/243/255.
- Parent allocation regression: 128 MiB extra Python memory leaves native peaks at 2268--2284 KiB.
- GCC static analysis and AddressSanitizer/UndefinedBehaviorSanitizer checks passed.
- Independent core-code review found no actionable issues.
- Source, results, and tests remain uncommitted for user review.
