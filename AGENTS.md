# Repository Guidelines

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
make -C C test_end_to_end   # transfer a control file with every scheme
make -C C test_mixed_end_to_end # verify clean/error frame mixing end to end
make -C C check_crc_header  # validate the CRC interface
make -C C generate_test_data # regenerate deterministic fixtures through uv
make -C C experiments       # run the seeded experiment matrix through uv
make -C C plots             # regenerate SVG plots from the results CSV
make -C C clean             # remove generated C build output
```

Override experiment defaults with `EXPERIMENT_INPUT=<path>` and
`EXPERIMENT_REPEATS=<positive-count>`. Run Python scripts through `uv run`
rather than the system interpreter.

## Coding Style & Naming Conventions

Use C17, two-space indentation, and braces on a new line for functions. Use `snake_case` for functions and variables, `UPPER_SNAKE_CASE` for macros, and fixed-width integer types for wire data. Prefix public functions with their module, such as `checksum16_compute`. Keep private helpers `static`. Headers contain declarations and Doxygen `/** ... */` API documentation; source files contain implementations. Treat in-memory frame structures as logical representations; serialize wire headers field by field rather than copying C structs directly.

## Testing Guidelines

Use small C test executables with assertions or explicit pass/fail return codes; no external test framework is currently required. Name tests `tests/test_<module>.c`. Cover normal, boundary, malformed, and error-injection cases. For reproducible random-error tests, pin the seed, final RNG state, and mutated bytes. Invalid-input tests for random operations must confirm that neither data nor RNG state changes. Frame serialization tests must pin wire bytes and network byte order; rejected serialization must leave output unchanged. Socket tests must distinguish clean EOF from a truncated record. End-to-end control transfers must compare binary output byte-for-byte for every FCS scheme. Every source change must compile with strict warnings and pass the affected tests before continuing.

## Commit & Pull Request Guidelines

Use short, imperative, scoped messages consistent with the existing history, for example `feat: add random odd-bit validation test`. Pull requests should explain the change, identify affected modules, list executed tests, and link relevant issues. Include screenshots only for graphs or report-rendering changes.

## Agent-Specific Instructions

Treat this as a guided learning project. Do not implement code unless explicitly requested. For every suggested snippet, explain what it does, why it is needed, how it integrates, and how the user can test it immediately.
