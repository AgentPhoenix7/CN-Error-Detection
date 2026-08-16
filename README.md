# Error Detection with Checksum and CRC

This repository contains the C17 implementation of a sender/receiver error-
detection simulation. The programs transfer files over TCP using an
Ethernet-like frame, optionally corrupt selected transmitted frames, and record
which frames are accepted or rejected by a 16-bit one's-complement checksum or
one of four CRC variants.

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
`test_frame`, `test_socket_utils`, `test_evaluation`, and `test_tools`.

Socket tests and end-to-end tests require permission to create local sockets:

```bash
make -C C test_end_to_end
make -C C test_mixed_end_to_end
```

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

Rejected frames are not written to the receiver's output file. Consequently,
only a `none` control run is expected to reproduce the input byte-for-byte.

## Reproducible Python tooling with uv

The Python utilities use only the standard library, but their interpreter is
pinned through the uv project in `C/`:

```bash
cd C
uv venv .venv
uv sync
uv run tools/generate_test_data.py
uv run tools/run_experiments.py input_files/detection_1m.txt --repeats 3 --error-interval 2
uv run tools/plot_results.py results/experiments_mixed.csv results/plots
```

Equivalent Make targets are `generate_test_data`, `experiments`, `plots`,
`report`, and `results`. The `results` target regenerates both the SVG plots and
the Markdown report from the same validated CSV.
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

The runner combines sender-side clean/injected counts with receiver-side
accepted/rejected counts. It records file sizes plus elapsed time,
user/system CPU time, and peak resident-set size separately for sender and
receiver. The plotting and report-generation tools share one schema validator
and aggregation module and use no third-party Python packages. Edit
`C/report/report_template.md`; `C/report/report.md` is generated and should not
be edited directly. The superseded 11-column result format is retained only in
`C/results/raw/experiments_legacy_schema.csv`.

See [C/report/report.md](C/report/report.md) for design details, final results,
the three assignment-required comparison cases, and interpretation guidance.
