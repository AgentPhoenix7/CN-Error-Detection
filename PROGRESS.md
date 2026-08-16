# Project Progress and Session Handoff

## How to Resume in a New Conversation

Attach or reference this file and say:

> Read `AGENTS.md`, `PROGRESS.md`, and inspect the current repository. Continue the guided C implementation from the exact current step. Do not implement code unless I explicitly request it.

`AGENTS.md` is authoritative for repository layout, formatting, testing, and the guided-learning workflow.

## Assignment Objective

Build an error-detection simulation with separate sender and receiver programs communicating through sockets. The sender reads a file, creates Ethernet-like frames, optionally injects bit errors, and sends checksum/CRC codewords. The receiver calculates syndromes and accepts or rejects each dataword.

Required schemes:

- 16-bit one's-complement checksum
- CRC-8: lower polynomial `0xD5`
- CRC-10: lower polynomial `0x233`
- CRC-16: lower polynomial `0x8005`
- CRC-32: lower polynomial `0x04C11DB7`

Required error patterns include single-bit, two isolated single-bit, odd-number, and burst errors. Results must compare detection performance, execution time, and resource utilization using tables and graphs.

Demonstration: **10–14 August 2026**. Report submission: **17–21 August 2026**.

## Collaboration Rules

- This is a guided learning project. The user writes implementation steps unless explicitly asking the agent to edit.
- Every suggested snippet must explain what it does, why it is needed, how it integrates, and how to test it immediately.
- Run strict compilation and affected tests after every change.
- Current work stays in `C/`. Future C++, Python, and Java implementations belong in separate top-level directories.
- Use C17, two-space indentation, `snake_case`, fixed-width integers, and function opening braces on new lines.
- Public header documentation uses Doxygen `/** ... */` comments.

## Working Design Decisions

- Use pure C17, GCC, POSIX sockets, and TCP. The professor allows either TCP or UDP; TCP isolates the experiment from transport loss and reordering.
- Test checksum and all four CRC variants on equivalent data and error patterns.
- CRC uses academic polynomial division with the listed polynomial, initial remainder zero, no reflection, and no final XOR.
- CRC-10 is serialized in network byte order with its 10 remainder bits
  left-aligned in the 2-byte FCS field. The lower six transmitted bits are
  zero padding. A parser must reject nonzero padding before shifting the
  remainder right by six bits for `crc_verify()`.
- Custom frame header: source MAC 6 bytes, destination MAC 6 bytes, valid
  payload length 2 bytes, sequence number 1 byte; total header size 15 bytes.
- The two-byte payload-length field stores the actual unpadded payload length
  in network byte order; it does not store complete frame length.
- Zero padding is transmitted but excluded from the payload-length field. The
  receiver derives the minimum transmitted payload from the selected FCS size
  so every complete frame is at least 64 bytes.
- The FCS protects the serialized 15-byte header and complete padded payload;
  the FCS field itself is excluded from its calculation.
- Sender and receiver agree on the active checksum or CRC scheme at startup;
  that scheme determines the serialized FCS size.
- TCP carries each simulated frame as a two-byte network-order record length
  followed by the frame bytes. The prefix is transport metadata, is not part
  of the simulated frame or FCS, and is not corrupted by error injection. A
  zero record length terminates the transfer.
- Complete simulated frame range: 64–1518 bytes.
- Absolute padded payload range: 45–1502 bytes. CRC-8 uses 48–1502 bytes;
  the two-byte schemes use 47–1501 bytes; CRC-32 uses 45–1499 bytes.
- FCS sizes: checksum 2, CRC-8 1, CRC-10 2, CRC-16 2, CRC-32 4 bytes.
- Final incomplete data is zero-padded. Its header preserves the true payload
  length so binary files that legitimately end in zero bytes remain unambiguous.
- Injected-pattern transfers independently and reproducibly select each frame
  with probability `1 / ERROR_INTERVAL`. The compatibility name remains in the
  CLI and CSV. The default denominator is two, and `none` remains a fully clean,
  byte-for-byte control.
- Frame selection and error-position choices use separate seeded generators;
  the selection seed is the run seed XOR `0x9E3779B9`. A denominator of one
  selects every frame without advancing the selection generator.
- Controlled error functions use deterministic bit positions. Random selectors
  use caller-seeded state, and the two-isolated-bit pattern chooses
  nonadjacent positions.
- A controlled burst flips every bit in one contiguous range. The complete
  range is validated before mutation so rejected bursts cannot be partially
  applied.
- Reproducible random selection uses caller-owned `error_injection_rng_t`
  state backed by a fixed-width `uint32_t` value. State advancement uses the
  fixed recurrence `state = state * 1664525 + 1013904223`; defined `uint32_t`
  wraparound makes sequences reproducible.

## Completed Work

### Repository and tooling

- Repository root is `Assignment 1/`; C code is under `C/`.
- Root and C-specific `.gitignore` files exist; this progress file is trackable.
- VS Code Makefile Tools uses `C/` as its Make directory.
- Local VS Code launch configurations exist for the checksum, CRC,
  error-injection, and frame test binaries.
- Make targets include `all`, `test`, every module-specific test,
  `test_end_to_end`, `test_mixed_end_to_end`, header checks, fixture
  generation, experiments, `plots`, `report`, `results`, and `clean`.
- `C/pyproject.toml` and `C/uv.lock` define the Python tool environment.
- `generate_test_data.py` recreates deterministic empty, text, maximum-payload,
  and multi-frame fixtures.

### Configuration and checksum

- `C/include/config.h` defines and compile-checks frame constants.
- `checksum16_compute()` handles even, odd, and empty input.
- `checksum16_verify()` accepts valid data and rejects corrupted data or checksum fields.
- Seven permanent checksum tests pass.

### CRC

- `crc_parameters_t` contains `width` and lower `polynomial` fields.
- Four read-only parameter objects are declared in `crc.h` and defined in `crc.c`.
- `crc_compute()` implements generic MSB-first polynomial division safely for widths 1 through 32.
- Known vectors pass for CRC-8, CRC-10, CRC-16, and CRC-32.
- `crc_verify()` accepts valid data and rejects corrupted data or CRC fields.
- Thirteen permanent CRC tests pass.

### Error injection

- `error_injection_flip_bit()` flips deterministic MSB-first bit positions and
  safely rejects null or out-of-range input.
- `error_injection_flip_two_bits()` validates both distinct positions before
  mutation, preventing partially applied patterns.
- `error_injection_flip_odd_bits()` validates a positive odd count and every
  distinct position before reusing the single-bit primitive for mutation.
- Odd-pattern tests cover valid separated positions, even and zero counts,
  out-of-range and duplicate positions, null index arrays, null data, and
  atomic rejection.
- `error_injection_flip_burst()` validates a positive, overflow-safe contiguous
  range before flipping it.
- Burst tests cover a valid five-bit burst crossing a byte boundary, zero
  length, an out-of-range start, range overrun, null data, arithmetic overflow,
  and atomic rejection without partial mutation.
- `error_injection_rng_t` stores fixed-width state for reproducible
  pseudo-random error selection.
- `error_injection_select_frame()` maps one generator value to a scaled
  denominator-sized bucket. Tests pin its seeded sequence, denominator-one
  behavior, and atomic rejection of invalid input.
- `error_injection_rng_seed()` initializes caller-owned state, accepts every
  `uint32_t` seed including zero, and safely rejects a null state pointer.
- `error_injection_flip_random_bit()` advances the fixed generator, maps the
  result to a safe MSB-first bit position, and reuses the deterministic
  single-bit primitive. Tests cover a fixed reproducible vector, null data,
  null RNG state, zero length, overflow-sized length, and atomic rejection.
- `error_injection_flip_random_two_bits()` repeatedly selects until the two
  positions differ by more than one, then reuses the deterministic two-bit
  primitive. Its reproducible vector selects bits 7 and 2 from seed
  `0x12345678`. Tests cover null data, null RNG state, zero length,
  overflow-sized length, and atomic rejection.
- `error_injection_flip_random_odd_bits()` selects a caller-requested positive
  odd number of distinct positions, stores them in a checked temporary array,
  and reuses `error_injection_flip_odd_bits()` for mutation. Its three-bit
  reproducible vector produces `0x61` from seed `0x12345678`.
- Random odd-bit tests cover null data, null RNG state, zero and
  overflow-sized lengths, zero and even counts, counts exceeding available
  bits, temporary-allocation size overflow, and duplicate-position
  reselection. Rejected inputs preserve both data and RNG state.
- `error_injection_flip_random_burst()` selects one valid contiguous start
  position with one RNG advance and reuses `error_injection_flip_burst()`.
- Random-burst tests cover a reproducible cross-byte vector, null data, null
  RNG state, zero and overflow-sized data lengths, zero and excessive burst
  lengths, full-buffer burst length, and atomic rejection.
- Fifty-one permanent error-injection tests pass.

### Frame handling

- `frame_header_t` is a logical in-memory representation containing source and
  destination MAC addresses, actual unpadded payload length, and sequence
  number. It is not copied directly to the wire.
- `frame_header_serialize()` writes exactly 15 bytes in field order and encodes
  payload length in network byte order.
- Serialization rejects null pointers and payload lengths above
  `MAX_PAYLOAD_SIZE` before modifying output.
- `frame_header_parse()` validates and decodes the wire header without changing
  output on failure.
- Helpers calculate and enforce FCS-specific padded-payload limits and complete
  frame sizes for every scheme.
- The sender selects its read chunk size from the active FCS so maximum-sized
  serialized frames remain within `MAX_FRAME_SIZE`.
- `frame_serialize()` zero-pads the payload and writes checksum, CRC-8, CRC-10,
  CRC-16, or CRC-32 in the agreed network representation.
- `frame_verify_and_extract()` rejects malformed sizes, nonzero payload
  padding, nonzero CRC-10 padding bits, and FCS mismatches before exposing
  parsed output.
- Seventeen permanent frame tests cover wire vectors, boundary sizes, parsing,
  all FCS formats, corruption, malformed input, zero-length payloads, and
  atomic rejection.

### Socket transport and applications

- `socket_utils_connect()`, `socket_utils_listen()`, and
  `socket_utils_accept()` provide the TCP connection operations.
- `socket_utils_send_all()` and `socket_utils_receive_all()` handle partial
  system calls and interrupted operations. Receive results distinguish clean
  EOF from a truncated record.
- Five permanent socket tests cover exact byte transfer, clean EOF, partial
  record rejection, zero-length operations, and invalid inputs.
- `sender` reads input in payload-sized chunks, builds sequenced frames,
  independently selects each frame with the configured seeded probability,
  applies the selected error pattern after calculating the FCS, and reports
  clean/injected counts plus its own elapsed time, CPU time, and peak RSS.
- `receiver` validates complete records, writes only accepted unpadded payload,
  and reports accepted/rejected counts plus its own elapsed time, CPU time,
  and peak RSS.
- Command-line schemes are `checksum`, `crc8`, `crc10`, `crc16`, and `crc32`.
  Error modes are `none`, `single`, `two`, `odd`, `burst`, and `random`.

### Experiments, plots, and documentation

- `C/input_files/` contains deterministic 1 MiB and 8 MiB text inputs;
  `C/output_files/` contains the latest received file for each experiment
  combination and ignores generated outputs while retaining the directory.
- `run_experiments.py` starts sender/receiver pairs, uses reproducible seeds,
  validates lossless controls and frame-count invariants, combines both
  processes' metrics, and records input/output sizes.
- `plot_results.py` uses only the Python standard library to aggregate repeats
  and produce mixed-frame outcome, undetected-rate, execution-time, and
  sender/receiver resource SVGs.
- `C/results/experiments_mixed.csv` contains the final 450-run campaign: 20
  repeats of four mixed error patterns on 1 MiB and 10 no-error resource
  controls on 8 MiB for all five schemes.
- The seeded probability-one-half campaign produced 7,055 paired injected
  frames for each two-byte scheme and pattern. Observed misses were 33 checksum
  burst frames, 60 CRC-8 two-bit frames, and 11 CRC-10 two-bit frames; CRC-16
  and CRC-32 had no observed misses in this sample.
- Every 8 MiB no-error control reproduced its input byte-for-byte. Detection
  rate is rejected divided by injected frames, never divided by all received
  frames.
- `experiment_results.py` provides shared strict CSV validation and aggregation
  for plots and reports. It rejects schema drift, duplicate experiment grains,
  broken frame identities, non-finite metrics, incorrect stored rates, and
  incomplete campaigns.
- `README.md` documents build, protocol, program usage, uv setup, experiments,
  plotting, and report regeneration. `C/report/report_template.md` contains
  editable prose; `C/report/report.md` is generated deterministically with
  campaign tables, required comparison cases, charts, and limitations.
- The legacy 11-column CSV is archived as
  `C/results/raw/experiments_legacy_schema.csv` and is never consumed by the
  current analysis pipeline.

## Current Verification

Run from the repository root:

```bash
make -C C test
```

Current result: 99 C checks pass: seven checksum, thirteen CRC, fifty-four
error-injection, seventeen frame, five socket, and three direct evaluation
comparisons. Ten Python validation/report/runner tests also pass. Builds use
`-std=c17 -Wall -Wextra -Wpedantic -Werror` and validate `config.h` as part of
the normal `all` and `test` targets.

Additional verification completed:

- GCC `-fanalyzer` build with warnings treated as errors;
- lossless three-frame control transfers for all five schemes;
- mixed three-frame transfers for all five schemes, each with two clean and one
  injected frame for the pinned seed and consistent sender/receiver counts;
- a complete 450-run seeded-selection campaign with validated lossless controls;
- lossless 1499-byte maximum-payload transfers for all five schemes;
- empty-file transfer; and
- syntax validation of Python tools and structural/XML validation of the four
  generated SVG plots.

## Reproduce the Final Evaluation

Run from the repository root:

```bash
make -C C experiments_inference
make -C C results
```

The experiment target replaces the CSV with 450 new rows. The `results` target
validates that campaign, regenerates all four SVGs, and rebuilds the Markdown
report from its template. The remaining project work is demonstration rehearsal
and submission packaging.
