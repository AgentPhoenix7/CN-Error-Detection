# Review Follow-up Implementation Plan

> **For agentic workers:** Use superpowers:executing-plans to implement this plan task by task.

**Goal:** Fix reviewed problems 1--5 while explicitly preserving problem 6.

**Scope:** Safe experiment failure handling, usable CSV append, consistent
repeat metadata, atomic sender-dispatch RNG state, and valid VS Code receiver
arguments. The user excluded the CRC-32 minimum-payload repair.

**Architecture:** Stage CSV output beside its destination for atomic rename;
clean up receiver processes in `finally`; append new repeat/seed identities
after existing groups; validate metadata before aggregation; use a local RNG
copy in the sender dispatcher. Preserve the existing CLI and CSV schema.

**Tech stack:** C17, Linux/POSIX sockets, Python 3.11+ standard library, uv, Make.

## Constraints

- Do not change frame geometry, sequence numbering, or the 45-byte CRC-32 minimum.
- Preserve valid-call RNG sequences and the existing recorded 450-run dataset.
- Transactional CSV staging must be adjacent to the destination so rename
  cannot cross filesystems.
- Do not replace or partially append a durable CSV after a failed batch.
- Keep cleanup bounded and reap only processes started by the runner.

## Review focus

- A sender that exits before connecting must not leave its receiver alive.
- Startup and transfer timeouts must preserve existing CSV contents.
- Append must continue repeat/seed identities and reject incompatible metadata.
- Per-repeat file, byte count, and error interval must not be hidden by averages.
- The full two-input Make campaign must commit only after both phases succeed.

## Tasks

- [x] Add failing metadata regressions, validate each group, and verify Python checks.
- [x] Add failing dispatch-state regressions, commit RNG only on success, and verify C checks.
- [x] Add failing runner rollback, cleanup, timeout, and append regressions; implement atomic results and preflight validation.
- [x] Protect the combined Make inference campaign and test failure of its second phase.
- [x] Correct the local receiver launch arguments and verify actual startup.
- [x] Update affected docs, regenerate the report, run the full suite/static checks, review independently, and verify preserved-file hashes.

## Verification record

- The new regressions failed against the preceding behavior, then passed after
  the corresponding repairs. The final suite passed 104 C checks and 26 Python
  tests, plus five clean and five mixed end-to-end transfers.
- A separate two-input smoke campaign successfully committed all 25 scheme/pattern
  groups. A missing second input preserved the previous campaign byte-for-byte.
- GCC `-fanalyzer` built all targets with strict warnings treated as errors.
- The corrected local VS Code receiver launch accepted its three arguments
  and completed an empty loopback transfer. Its ignored settings remain local.
- Independent review found an additional result/receiver-output collision;
  a regression reproduced it, and preflight now rejects it before any transfer.
  The reviewer confirmed the correction with no further findings.
- Ten Markdown documents passed link/fence checks, and the regenerated report
  exactly matches the retained 450 rows / 25 groups. The recorded campaign was
  not rerun or replaced during this follow-up.
- SHA-256 checks preserved `config.h`, `frame.c`, `test_frame.c`, and the saved
  campaign from the start of this follow-up. Problem 6 remains explicitly open.
