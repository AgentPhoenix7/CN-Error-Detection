# Repository Guidelines

## Project Structure & Module Organization

The repository root contains shared assignment references, including the specification PDFs, `instruction.jpg`, and the main `README.md`. Language implementations belong in separate directories; the active implementation is under `C/`.

Within `C/`, public interfaces live in `include/`, implementations in `src/`, and module tests in `tests/`. Test inputs belong in `test_data/`. Generated experiment data and plots go in `results/`, plotting utilities in `tools/`, and written analysis in `report/`. Keep generated binaries and object files in `C/build/`.

Future C++, Python, and Java work must remain in their corresponding top-level directories, with independent build files and `.gitignore` rules.

## Build, Test, and Development Commands

Run commands from the repository root. Preserve the Makefile's simple interface as modules are added:

```bash
make -C C all               # build or validate current modules
make -C C test              # run all available C checks
make -C C test_checksum     # run checksum tests
make -C C test_crc          # run CRC tests
make -C C test_error_injection # run error-injection tests
make -C C check_crc_header  # validate the CRC interface
make -C C clean             # remove generated C build output
```

## Coding Style & Naming Conventions

Use C17, two-space indentation, and braces on a new line for functions. Use `snake_case` for functions and variables, `UPPER_SNAKE_CASE` for macros, and fixed-width integer types for wire data. Prefix public functions with their module, such as `checksum16_compute`. Keep private helpers `static`. Headers contain declarations and Doxygen `/** ... */` API documentation; source files contain implementations.

## Testing Guidelines

Use small C test executables with assertions or explicit pass/fail return codes; no external test framework is currently required. Name tests `tests/test_<module>.c`. Cover normal, boundary, malformed, and error-injection cases. For reproducible random-error tests, pin the seed, final RNG state, and mutated bytes. Invalid-input tests for random operations must confirm that neither data nor RNG state changes. Every source change must compile with strict warnings and pass the affected tests before continuing.

## Commit & Pull Request Guidelines

Use short, imperative, scoped messages consistent with the existing history, for example `feat: add random odd-bit validation test`. Pull requests should explain the change, identify affected modules, list executed tests, and link relevant issues. Include screenshots only for graphs or report-rendering changes.

## Agent-Specific Instructions

Treat this as a guided learning project. Do not implement code unless explicitly requested. For every suggested snippet, explain what it does, why it is needed, how it integrates, and how the user can test it immediately.
