# Evaluation Repair Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make frame injection PDF-compliant and make every evaluation artifact, including `report.md`, reproducible and mutually consistent.

**Architecture:** Add a tested seeded frame-selection primitive and give the sender independent selection and mutation RNG streams. Centralize CSV validation and aggregation in one Python module consumed by both the plotting and report generators, then rerun the final campaign and regenerate every derived artifact.

**Tech Stack:** C17, POSIX sender/receiver applications, Python 3.11 standard library through uv, Make, Markdown, SVG.

## Global Constraints

- Keep the existing sender CLI position and CSV field name `error_interval` for compatibility; interpret `N` as a seeded `1/N` frame-selection probability.
- `N=1` selects every frame, and invalid selection input does not advance RNG state.
- Use separate deterministic selection and mutation RNG streams.
- Do not change checksum, CRC, frame-wire, or TCP-record semantics.
- Treat `C/report/report_template.md` as editable source and `C/report/report.md` as generated output.
- Use only the Python standard library and run Python through uv.
- Preserve current generated results until the replacement campaign completes successfully.

---

### Task 1: Seeded random frame selection

**Files:**
- Modify: `C/include/error_injection.h`
- Modify: `C/src/error_injection.c`
- Modify: `C/tests/test_error_injection.c`

**Interfaces:**
- Produces: `bool error_injection_select_frame(error_injection_rng_t *rng, uint32_t denominator, bool *selected)`.
- Semantics: multiply-high bucket mapping `((uint64_t)next * denominator) >> 32`; bucket zero is selected.

- [ ] **Step 1: Add failing selection tests**

Add tests for this exact 12-decision vector from seed `0x8C032FC1` and denominator 2:

```c
static const bool expected[] = {
  true, false, false, true, true, true,
  true, false, false, false, false, false
};
```

Also test denominator 1 returns `true` without advancing state, and null RNG,
null output, and denominator zero return `false` without changing state/output.

- [ ] **Step 2: Run the error-injection test and verify RED**

Run: `make -C C test_error_injection`

Expected: compilation fails because `error_injection_select_frame` is undeclared.

- [ ] **Step 3: Declare and implement the minimal helper**

Document the public API with Doxygen. Validate every pointer and denominator
before advancing the RNG. Special-case denominator 1. For other values, advance
once and set:

```c
bucket = ((uint64_t)error_injection_rng_next(rng) * denominator) >> 32U;
*selected = bucket == 0U;
```

- [ ] **Step 4: Run the error-injection test and verify GREEN**

Run: `make -C C test_error_injection`

Expected: all existing tests and the new selection tests pass.

### Task 2: Integrate independent sender RNG streams

**Files:**
- Modify: `C/src/sender.c`
- Modify: `C/Makefile`

**Interfaces:**
- Consumes: `error_injection_select_frame` from Task 1.
- Produces: sender behavior where `ERROR_INTERVAL=N` means expected one selected frame in N.

- [ ] **Step 1: Strengthen the mixed end-to-end assertion before changing sender behavior**

Add `C/tests/validate_mixed_csv.py` using `csv.DictReader`. It must require one
row per scheme and assert `frames_injected > 0`, `frames_clean > 0`, and every
existing count invariant. Update `test_mixed_end_to_end` to run it after the
three-frame transfer.

- [ ] **Step 2: Run mixed end-to-end and verify the validator fails**

Run: `make -C C test_mixed_end_to_end`

Expected: failure because the validator target/file is not yet wired completely.

- [ ] **Step 3: Replace fixed modulo selection**

Use:

```c
#define SELECTION_SEED_XOR UINT32_C(0x9E3779B9)
error_injection_rng_t selection_rng;
error_injection_rng_t error_rng;
bool inject_frame;
```

Seed `selection_rng` with `seed ^ SELECTION_SEED_XOR` and `error_rng` with
`seed`. For each non-`none` frame, call `error_injection_select_frame`; apply
the error only when selected. Keep `N=1` compatible and revise usage/output text
to say "seeded probability 1/N".

- [ ] **Step 4: Run mixed and clean end-to-end tests**

Run: `make -C C test_end_to_end && make -C C test_mixed_end_to_end`

Expected: all five clean transfers remain byte-identical, and every mixed row
contains at least one clean and one injected frame with valid counts.

### Task 3: Pin the three required evaluation cases

**Files:**
- Create: `C/tests/test_evaluation.c`
- Modify: `C/Makefile`

**Interfaces:**
- Consumes: checksum, CRC, and deterministic bit-flip APIs.
- Produces: `test_evaluation` covering both-detect, checksum-only, and CRC-only cases.

- [ ] **Step 1: Write the failing evaluation test executable**

Use three exact fixtures:

1. Both detect: bytes `{0x12, 0x34}`, flip bit 0, compare Checksum-16 and CRC-16.
2. Checksum only: 256 zero bytes, flip bits 0 and 511. Checksum changes from
   `0xFFFF`; CRC-10 remains `0x000` because the separation equals its order 511.
3. CRC only: bytes `{0x00, 0x00, 0x00, 0x01}`, flip bits 15 and 31, producing
   `{0x00, 0x01, 0x00, 0x00}`. The checksum remains `0xFFFE`, while CRC-16
   changes from `0x8005` to `0x8017`.

Each test must calculate the original FCS, mutate through the real
error-injection functions, and assert the required verifier outcomes.

- [ ] **Step 2: Run and verify RED**

Run: `make -C C test_evaluation`

Expected: Make reports that the target does not exist.

- [ ] **Step 3: Add the strict C17 build and test targets**

Link `src/checksum.c`, `src/crc.c`, `src/error_injection.c`, and
`tests/test_evaluation.c`. Add the target to `all` and `test`.

- [ ] **Step 4: Run and verify GREEN**

Run: `make -C C test_evaluation`

Expected: three PASS lines and exit status zero.

### Task 4: Centralize CSV validation and summaries

**Files:**
- Create: `C/tools/experiment_results.py`
- Create: `C/tests/test_experiment_results.py`
- Modify: `C/tools/plot_results.py`
- Modify: `C/Makefile`

**Interfaces:**
- Produces: `load_rows(path)`, `load_summaries(path)`, `summary_for(...)`,
  `validate_complete_campaign(summaries)`, and `required_comparisons(summaries)`.
- `plot_results.py` imports `SCHEMES`, `PATTERNS`, `load_summaries`, and
  `summary_for` without changing SVG output for the pre-change CSV.

- [ ] **Step 1: Write failing standard-library unit tests**

Cover exact-schema rejection, duplicate `(scheme, pattern, repeat)` rejection,
all five row invariants, stored-rate verification, complete 25-group coverage,
consistent repeat counts, pooled aggregation, and the three comparison rows.
Use `tempfile.TemporaryDirectory` and `unittest`.

- [ ] **Step 2: Run and verify RED**

Run: `UV_CACHE_DIR=/tmp/assignment1-uv-cache uv run --project C python -m unittest discover -s C/tests -p 'test_*.py' -v`

Expected: import failure for `experiment_results`.

- [ ] **Step 3: Implement the shared module and refactor plotting imports**

Use the 27-column schema currently emitted by `run_experiments.py`. Raise
`ValueError` with the CSV line/group in every validation message. Preserve the
existing summary dictionary keys so plot rendering remains unchanged.

- [ ] **Step 4: Verify unit tests and plot compatibility**

Run the Python unit tests, generate plots into a `mktemp -d` directory from the
current CSV, and compare all four files byte-for-byte with checked-in SVGs.

### Task 5: Generate the report from the validated CSV

**Files:**
- Create: `C/report/report_template.md`
- Create: `C/tools/generate_report.py`
- Create: `C/tests/test_generate_report.py`
- Generate: `C/report/report.md`
- Modify: `C/Makefile`

**Interfaces:**
- Consumes: validated summaries and comparisons from Task 4.
- Produces: `render_report(template_text, summaries, source_name)` and CLI
  `generate_report.py CSV TEMPLATE OUTPUT`.

- [ ] **Step 1: Write failing report-generator tests**

Require exact-once replacement of these tokens:

```text
{{CAMPAIGN_SUMMARY}}
{{OUTCOME_TABLE}}
{{PATTERN_TABLE}}
{{REQUIRED_CASES}}
{{TIMING_TABLE}}
{{RESOURCE_TABLE}}
{{OBSERVED_MISSES}}
```

Test missing and duplicate placeholders, full numeric rendering, generated-file
notice, no unreplaced `{{...}}`, and byte-identical repeated rendering.

- [ ] **Step 2: Run and verify RED**

Run the Python unittest discovery command from Task 4.

Expected: import failure for `generate_report`.

- [ ] **Step 3: Implement template and generator**

Move stable prose from the existing report into the template. Add the CRC
theory table: CRC-8/10/16 contain `(x + 1)` while CRC-32 does not; guaranteed
burst lengths are 8/10/16/32; CRC-8 and CRC-10 two-bit recurrence orders are
93 and 511; CRC-16 order 32767 and CRC-32's order both exceed the 12,144-bit
maximum frame used here.

Render empirical zeroes as observations, not probability-zero claims. Include
the deterministic fixtures from Task 3 and aggregate paired-campaign evidence.

- [ ] **Step 4: Add `report` and `results` Make targets and verify GREEN**

`report` invokes the generator for `$(PLOT_INPUT)`. `results` depends on
`plots report`. Run Python unit tests and `make -C C report` twice, comparing
the two report hashes.

### Task 6: Replace the campaign and reconcile documentation

**Files:**
- Modify: `C/tools/run_experiments.py` help text
- Modify: `README.md`
- Modify: `PROGRESS.md`
- Rename: `C/results/experiments.csv` to `C/results/raw/experiments_before_mixed_frames.csv`
- Generate: `C/results/experiments_mixed.csv`
- Generate: `C/results/plots/*.svg`
- Generate: `C/report/report.md`

**Interfaces:**
- Consumes: all prior tasks.
- Produces: one final internally consistent submission evidence set.

- [ ] **Step 1: Archive the legacy 11-column CSV**

Use `git mv` so its historical contents remain tracked. Ensure README identifies
only `experiments_mixed.csv` as active report evidence.

- [ ] **Step 2: Update CLI and workflow documentation**

Replace fixed-slot wording with seeded probability `1/N`. Document `make -C C
results`, the generated-report notice, the final campaign, deterministic case
test, and current verification commands.

- [ ] **Step 3: Run the complete final campaign**

Run: `UV_CACHE_DIR=/tmp/assignment1-uv-cache make -C C experiments_inference`

Expected: 450 validated data rows covering 20 injected repeats and 10 controls.

- [ ] **Step 4: Regenerate plots and report from the same CSV**

Run: `UV_CACHE_DIR=/tmp/assignment1-uv-cache make -C C results`

Expected: four SVGs and `report.md` reflect the new random-selection counts and
contain no pilot/three-repeat language.

### Task 7: Full verification

**Files:**
- Verify all modified and generated files.

- [ ] **Step 1: Run strict functional checks**

```text
make -C C test
make -C C test_end_to_end
make -C C test_mixed_end_to_end
```

- [ ] **Step 2: Run strict static analysis**

Compile every C test/application source set with existing flags plus
`-fanalyzer`, using temporary binaries under `/tmp`.

- [ ] **Step 3: Independently audit artifacts**

Check 450 unique CSV rows, 25 groups, repeat counts, every row invariant and
rate, all retained clean outputs, SVG XML parsing, plot byte regeneration,
report byte regeneration, required comparison text, generated notice, and no
stale pilot statements.

- [ ] **Step 4: Inspect repository integrity**

Run `git diff --check`, inspect `git status --short`, and review the complete
diff without modifying unrelated files.
