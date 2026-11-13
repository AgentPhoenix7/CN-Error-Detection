# Repository Guidelines

## Assignment Sources and Confirmed Project Requirements

Use `ErrorDetection(Checksum and CRC).pdf` as the assignment specification and
`instruction.jpg` as the supplemental protocol illustration. See
`docs/assignment-compliance.md` for the requirement-to-code evidence matrix,
confirmed clarifications, and open issues. Preserve the reference files.

- The user confirmed on November 13, 2026 that the illustration's **64--1518
  bytes** is the complete-frame range to use instead of the PDF's ambiguous
  64--1500-byte range.
- **Sequence numbering is required by the project**, even though the PDF does
  not mention it: retain the one-byte field, start at zero, increment modulo
  256, serialize/parse it, and protect it with the FCS. Do not remove it to
  match the PDF's omissions. The existing receiver does not enforce sequence
  continuity or implement ARQ; do not claim those features exist.
- The PDF's separate minimum payload of **46 bytes** is not waived by the
  complete-frame clarification. CRC-32 currently pads to 45 bytes; this is an
  open code gap, not verified compliance. The user explicitly excluded this
  repair (review problem 6) from the November 13 follow-up fixes; preserve it
  unless a later request authorizes the coordinated frame change.
- The existing two-byte header length stores valid payload bytes; the image
  labels it frame length. Record this semantic difference pending clarification.
- The CLI selects the four supplied CRC polynomials by scheme name. It does
  not accept arbitrary polynomial coefficients from the command line.
- No separate report-format rules are available, as confirmed by the user.
  Mark formatting and delivery pending; do not invent instructor requirements
  or claim the report has been submitted/uploaded.

Required schemes are Checksum-16 and CRC-8/10/16/32 with lower polynomials
`0xD5`, `0x233`, `0x8005`, and `0x04C11DB7`. Demonstrate all four error classes
and the three checksum-versus-CRC detection cases, with tables, graphs, timing,
and per-process resources. These use the documented academic CRC convention,
not complete Bluetooth/USB/Ethernet profiles.

## Project Structure & Module Organization

The repository root contains shared assignment references, including the specification PDFs, `instruction.jpg`, and the main `README.md`. Language implementations belong in separate directories; the active implementation is under `C/`.

Within `C/`, public interfaces live in `include/`, implementations in `src/`, and module tests in `tests/`. Test fixtures belong in `test_data/`; large files intended for transfer belong in `input_files/`, and receiver outputs go in ignored `output_files/`. Generated experiment data and plots go in `results/`, Python experiment and plotting utilities in `tools/`, and written analysis in `report/`. Keep generated binaries and object files in `C/build/`. The Python tools use the uv project declared by `C/pyproject.toml` and `C/uv.lock`; keep the local environment in ignored `C/.venv/`.

Future C++, Python, and Java work must remain in their corresponding top-level directories, with independent build files and `.gitignore` rules.

## Build, Test, and Development Commands

Run commands from the repository root. Preserve the Makefile's simple interface as modules are added:

```bash
make -C C all               # build or validate current modules
make -C C test              # run all available C checks
make -C C test_checksum     # run checksum tests
make -C C test_crc          # run CRC tests
make -C C test_error_injection # run error-injection tests
make -C C test_frame        # run frame serialization tests
make -C C test_socket_utils # run exact-length socket helper tests
make -C C test_evaluation   # demonstrate all three required detection cases
make -C C test_sender_random # verify mixed random-pattern coverage
make -C C test_tools        # run Python validator/report/runner checks
make -C C test_end_to_end   # transfer a control file with every scheme
make -C C test_mixed_end_to_end # verify clean/error frame mixing end to end
make -C C check_crc_header  # validate the CRC interface
make -C C check_config_header # validate frame constants
make -C C generate_test_data # regenerate deterministic fixtures through uv
make -C C experiments       # run the seeded experiment matrix through uv
make -C C experiments_inference # replace the final 450-row campaign
make -C C plots             # regenerate SVG plots from the results CSV
make -C C report            # regenerate report.md from its template and CSV
make -C C results           # regenerate both plots and report
make -C C clean             # remove generated C build output
```

Override experiment defaults with `EXPERIMENT_INPUT=<path>` and
`EXPERIMENT_REPEATS=<positive-count>`. Run Python scripts through `uv run`
rather than the system interpreter.

Python requires version 3.11 or newer; `pyproject.toml`/`uv.lock` do not pin one
exact interpreter. Applications require Linux procfs for post-exec peak RSS.
Build objects and compiler-generated `.d` files live under `C/build/`; preserve
automatic header dependency tracking. Smoke tests must not overwrite the
durable experiment CSV.

The Makefile is the current build definition. For CLion, load `C/Makefile` as a
Makefile project; CMake is optional and not configured. Opening the repository
root as a plain folder does not establish a loaded build model. IDE setup has
been documented, not verified interactively.

## Coding Style & Naming Conventions

Use C17, two-space indentation, and braces on a new line for functions. Use `snake_case` for functions and variables, `UPPER_SNAKE_CASE` for macros, and fixed-width integer types for wire data. Prefix public functions with their module, such as `checksum16_compute`. Keep private helpers `static`. Headers contain declarations and Doxygen `/** ... */` API documentation; source files contain implementations. Treat in-memory frame structures as logical representations; serialize wire headers field by field rather than copying C structs directly.

## Testing Guidelines

Use small C test executables with assertions or explicit pass/fail return codes; no external test framework is currently required. Name tests `tests/test_<module>.c`. Cover normal, boundary, malformed, and error-injection cases. For reproducible random-error tests, pin the seed, final RNG state, and mutated bytes. Invalid-input tests for random operations must confirm that neither data nor RNG state changes. Frame serialization tests must pin wire bytes and network byte order; rejected serialization must leave output unchanged. Socket tests must distinguish clean EOF from a truncated record. End-to-end control transfers must compare binary output byte-for-byte for every FCS scheme. Every source change must compile with strict warnings and pass the affected tests before continuing.

Use multiply-high rejection sampling for bounded mutation/pattern choices;
avoid low-bit modulo choices from the LCG. Keep two random bit positions
nonadjacent and odd subsets distinct. Preserve tests for parity/checksum-weight
coverage, mixed-pattern coverage, inherited-memory exclusion, and header rebuilds.
Sequence checks must distinguish field serialization/FCS protection from a
receiver continuity policy that is not implemented.

Preserve transactional result writes for replacement and append, including
both phases of `experiments_inference`. Validate per-repeat input path, byte
count, and error denominator before aggregation; append must continue group
repeat identities. Cover failure rollback, bounded receiver startup, and child
cleanup. CSV staging is deliberately adjacent to its destination for atomic
rename.

Keep published documentation self-contained. Cite repository sources, tests,
recorded results, and reproducible commands rather than ignored local diagnostics.

Edit `C/report/report_template.md`, then regenerate `C/report/report.md`.
Validate the same CSV for plots and reports. Report the three direct fixtures
separately from campaign counts; use bounds when aggregate marginals cannot
determine exact paired intersections. Receiver rejection counts include
structural/padding checks. Do not infer a zero theoretical miss rate from zero
observed misses. Preserve historical plans but label superseded assumptions.

## Commit & Pull Request Guidelines

Use short, imperative, scoped messages consistent with the existing history, for example `feat: add random odd-bit validation test`. Pull requests should explain the change, identify affected modules, list executed tests, and link relevant issues. Include screenshots only for graphs or report-rendering changes.

## Agent-Specific Instructions

Treat this as a guided learning project. Do not implement code unless explicitly requested. For every suggested snippet, explain what it does, why it is needed, how it integrates, and how the user can test it immediately.
