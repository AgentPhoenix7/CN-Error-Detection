# Error Detection with Checksum and CRC

This repository contains the C17 implementation of a sender/receiver error-
detection simulation. The programs transfer files over TCP using an
Ethernet-like frame, optionally corrupt selected transmitted frames, and record
which frames are accepted or rejected by a 16-bit one's-complement checksum or
one of four CRC variants.

## Assignment requirements and current status

The sources are the [assignment PDF](<ErrorDetection(Checksum and CRC).pdf>)
and [protocol illustration](instruction.jpg). The
[requirements audit](docs/assignment-compliance.md) maps each requirement to
implementation and verification evidence.

The user confirmed that complete frames use the illustration's **64--1518-byte
range** and that **one-byte sequence numbering remains a project requirement**,
even though the PDF does not mention it. The separate PDF requirement of a
minimum 46-byte padded payload remains open: CRC-32 currently permits 45 bytes.
Header-length semantics and whether polynomial input must accept arbitrary
coefficients also need clarification. No separate report-format rules are
available, so formatting and submission remain pending. Passing tests do not
certify these unresolved assignment points.

## Implemented schemes

| Scheme | Polynomial or rule | Serialized FCS |
| --- | --- | ---: |
| Checksum-16 | One's-complement sum of network-order 16-bit words | 2 bytes |
| CRC-8 | `0xD5` | 1 byte |
| CRC-10 | `0x233` | 2 bytes, remainder left-aligned |
| CRC-16 | `0x8005` | 2 bytes |
| CRC-32 | `0x04C11DB7` | 4 bytes |

All CRCs use an initial remainder of zero, MSB-first division, no reflection,
and no final XOR.

The CLI selects one of these supplied polynomials by scheme name. The reusable
C API takes a width/polynomial parameter object; arbitrary polynomial CLI input
is not implemented. Sharing a polynomial with Bluetooth, USB, or Ethernet does
not make these academic parameter conventions identical to those protocols.

## Frame and TCP record format

The simulated frame contains a 15-byte header, a zero-padded payload, and the
selected FCS:

| Header field | Size |
| --- | ---: |
| Source MAC address | 6 bytes |
| Destination MAC address | 6 bytes |
| Actual payload length, network byte order | 2 bytes |
| Sequence number | 1 byte |

The FCS protects the complete serialized header and padded payload. Payload
limits depend on the selected FCS size: the absolute transmitted range is
45--1502 bytes, while each scheme pads and chunks data so the complete
simulated frame remains within 64--1518 bytes.

These are current implementation limits, not a waiver of the PDF's 46-byte
minimum payload. In particular, short CRC-32 frames use 45 padded payload bytes.
The two-byte header field contains valid unpadded payload length, while the
illustration calls it frame length; the distinction is tracked in the audit.

Sequence numbers start at zero for every transfer and increment modulo 256.
The one-byte value is serialized, parsed, and protected by the FCS. TCP keeps
records ordered; the receiver does not separately enforce sequence continuity,
request retransmissions, or implement ARQ. Rejecting a frame leaves a gap in
the accepted sequence, and later valid frames are still accepted.

TCP is a byte stream, so each simulated frame is preceded by a two-byte
network-order record length. That prefix is transport metadata and is not part
of the simulated frame or its FCS. A zero record length marks the end of the
transfer. This preserves frame boundaries even when an injected error corrupts
the simulated header's length field.

## Build and test

From the repository root:

```bash
make -C C all
make -C C test
```

The build uses `-std=c17 -Wall -Wextra -Wpedantic -Werror`. Module-specific
targets include `test_checksum`, `test_crc`, `test_error_injection`,
`test_frame`, `test_socket_utils`, `test_evaluation`, `test_sender_random`, and
`test_tools`. Per-source objects and compiler-generated `.d` files in `C/build/`
ensure header edits rebuild every affected program and test.

The programs require Linux with readable `/proc/self/status` for peak RSS
measurement. They read `VmHWM` from their own post-exec address space; the old
`getrusage().ru_maxrss` figures included the Python launcher's memory footprint.

Socket tests and end-to-end tests require permission to create local sockets:

```bash
make -C C test_end_to_end
make -C C test_mixed_end_to_end
```

The current suite contains 104 C checks and 26 Python tests. The audit also
captured sequence wrap from 255 to 0 for all schemes and checked rejection of
each single-bit change to the sequence field.

## Open the C implementation in CLion

Load `C/Makefile` explicitly as a Makefile project, rather than opening only
the repository root as a plain folder. Right-click it and choose **Load
Makefile Project**, or use **File → Open**, select it, and choose **Open as
Project**. Allow clean-and-reload so CLion can discover compilation commands.
Use build target `all` and clean target `clean` if those need setting. See the
[JetBrains Makefile project instructions](https://www.jetbrains.com/help/clion/makefiles-support.html).

No `CMakeLists.txt` is configured or required for the current Makefile workflow.
CLion's actual UI load has not been verified in this audit.

## Run sender and receiver manually

Large text files intended for transfer live in `C/input_files/`. Received
files are written to `C/output_files/`; generated experiment outputs in that
folder are ignored by Git.

Start the receiver first:

```bash
C/build/receiver C/output_files/received.txt crc32 4321
```

In another terminal, start the sender:

```bash
C/build/sender C/input_files/detection_1m.txt crc32 single 127.0.0.1 4321 0x12345678 2
```

Sender syntax:

```text
sender INPUT SCHEME PATTERN [HOST] [PORT] [SEED] [ERROR_INTERVAL]
```

Receiver syntax:

```text
receiver OUTPUT SCHEME [PORT]
```

Schemes are `checksum`, `crc8`, `crc10`, `crc16`, and `crc32`. Error patterns
are `none`, `single`, `two`, `odd`, `burst`, and `random`. For a non-`none`
pattern, the default error interval is two: each frame is independently and
reproducibly selected for injection with probability `1/2`. In general,
`ERROR_INTERVAL=N` means selection probability `1/N`; the compatibility name
is retained in the command line and CSV schema. `random` deterministically
chooses one of the four injected patterns at each selected frame. Selection
and error positions use separate seeded generators. Set the interval to one
only when every frame should be corrupted.

Bit positions and random pattern choices use multiply-high bounded sampling
with rejection, avoiding the LCG's low-bit correlations and modulo bias.
Two-bit errors use nonadjacent positions; odd-bit errors use Floyd sampling
for distinct subsets. Random mutation buffers must fit `size_t` and contain at
most 2^32 bits. The repaired sampler changes the deterministic position vectors;
the current CSV and report were regenerated with it.

Rejected frames are not written to the receiver's output file. Consequently,
only a `none` control run is expected to reproduce the input byte-for-byte.

## Reproducible Python tooling with uv

The Python utilities use only the standard library and require Python 3.11 or
newer. The uv project in `C/` manages the environment; it does not pin an exact
interpreter version. The current audit used CPython 3.14.10:

```bash
cd C
uv venv .venv
uv sync
uv run tools/generate_test_data.py
uv run tools/run_experiments.py test_data/multi_frame.bin --repeats 1 --results build/manual-smoke/experiments.csv --output-directory build/manual-smoke/received
cd ..
```

Equivalent Make targets are `generate_test_data`, `experiments`, `plots`,
`report`, and `results`. The `results` target regenerates both the SVG plots and
the Markdown report from the same validated CSV.

Results are committed only after the whole requested batch succeeds and its
rows validate. Failed runs preserve the previous CSV; receiver startup and
transfers have timeouts, and the runner stops and reaps its receiver on failure.
Atomic staging files temporarily live beside the destination CSV.

Use `--append` to add repeats to existing scheme/pattern groups. Repeat numbers
continue after each group's highest recorded repeat; seeds use the supplied
base seed plus that repeat offset, with uint32 wraparound. Keep the same base
seed to continue its sequence. Append rejects changes to a group's input path,
input byte count, or error denominator. It cannot detect an edited input with
the same path and byte count, so retain the original fixtures.
Override the experiment input or repeat count with, for example:

```bash
make -C C experiments EXPERIMENT_INPUT=test_data/exact_frame.bin EXPERIMENT_REPEATS=10
```

For report-quality inference rather than a quick smoke test, use the dedicated
fixtures and experiment target:

```bash
make -C C experiments_inference
make -C C results
```

This runs 20 repeats of the 1 MiB text input for each injected-error pattern
(about 7,000 expected injected and 7,000 expected clean frames per
scheme/pattern with the default probability) and 10 no-error repeats of the
8 MiB text input. The combined 450-run results are written to
`C/results/experiments_mixed.csv`: error rows are used for detection inference,
and no-error rows are used for timing and resource comparison.
Both input phases and complete-campaign validation must succeed before this
target replaces the destination. To isolate a trial campaign, override
`INFERENCE_RESULTS` and `INFERENCE_OUTPUT_DIRECTORY` with separate trial paths.

The runner combines sender-side clean/injected counts with receiver-side
accepted/rejected counts. It records file sizes plus elapsed time,
user/system CPU time, and peak resident-set size separately for sender and
receiver. The plotting and report-generation tools share one schema validator
and aggregation module and use no third-party Python packages. Edit
`C/report/report_template.md`; `C/report/report.md` is generated and should not
be edited directly. The superseded 11-column result format is retained only in
`C/results/raw/experiments_legacy_schema.csv`.

The comparison table reports bounds when marginal detection totals cannot
determine the exact number of checksum-only or CRC-only outcomes. The three
deterministic cases in `test_evaluation.c` directly demonstrate each category.

See [C/report/report.md](C/report/report.md) for design details, final results,
the three assignment-required comparison cases, and interpretation guidance.

## Documentation and submission

| Document | Purpose |
| --- | --- |
| [Assignment audit](docs/assignment-compliance.md) | Requirement matrix, confirmed clarifications, open gaps, and evidence |
| [AGENTS.md](AGENTS.md) | Repository conventions and requirements future agents must preserve |
| [PROGRESS.md](PROGRESS.md) | Current handoff, verification, and remaining work |
| [Report template](C/report/report_template.md) | Editable technical narrative and campaign placeholders |
| [Generated report](C/report/report.md) | Current campaign results, cases, conclusions, and limitations |
| [Original evaluation design](docs/superpowers/specs/2026-08-16-evaluation-repair-design.md) | Historical design, with superseded assumptions annotated |
| [Original evaluation plan](docs/superpowers/plans/2026-08-16-evaluation-repair.md) | Historical implementation sequence, not the active task list |
| [Sampling/memory/build repair](docs/superpowers/plans/2026-11-13-sampling-memory-build-repair.md) | Completed scoped repair and verification evidence |

The PDF records demonstration dates of August 10--14, 2026 and report dates of
August 17--21, 2026, and calls for a Drive softcopy. These are source dates;
actual demonstration, final formatting, submission packaging, and Drive upload
are not established by this repository audit. The reference PDFs and image are
kept unchanged. No report formatting, page limit, or cover-page convention is
claimed to have been supplied by the instructor.
