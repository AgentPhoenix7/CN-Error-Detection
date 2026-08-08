# Error Detection with Checksum and CRC

This repository contains the C17 implementation of a sender/receiver error-
detection simulation. The programs transfer binary files over TCP using an
Ethernet-like frame, optionally corrupt each transmitted frame, and record
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

The FCS protects the complete serialized header and padded payload. Actual
payload length is at most 1499 bytes; transmitted payload is padded to at least
48 bytes. This keeps complete simulated frames within 64--1518 bytes.

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
`test_frame`, and `test_socket_utils`.

Socket tests and end-to-end tests require permission to create local sockets:

```bash
make -C C test_end_to_end
```

## Run sender and receiver manually

Start the receiver first:

```bash
C/build/receiver received.bin crc32 4321 C/results/experiments.csv single
```

In another terminal, start the sender:

```bash
C/build/sender C/test_data/multi_frame.bin crc32 single 127.0.0.1 4321 0x12345678
```

Sender syntax:

```text
sender INPUT SCHEME PATTERN [HOST] [PORT] [SEED]
```

Receiver syntax:

```text
receiver OUTPUT SCHEME [PORT] [CSV_PATH] [PATTERN_LABEL]
```

Schemes are `checksum`, `crc8`, `crc10`, `crc16`, and `crc32`. Error patterns
are `none`, `single`, `two`, `odd`, `burst`, and `random`. The explicit error
patterns corrupt every frame. `random` deterministically chooses one of the
four injected patterns for each frame from the supplied seed.

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
uv run tools/run_experiments.py test_data/multi_frame.bin --repeats 3
uv run tools/plot_results.py results/experiments.csv results/plots
```

Equivalent Make targets are `generate_test_data`, `experiments`, and `plots`.
Override the experiment input or repeat count with, for example:

```bash
make -C C experiments EXPERIMENT_INPUT=test_data/exact_frame.bin EXPERIMENT_REPEATS=10
```

The receiver records detection counts, wall-clock time, user/system CPU time,
and maximum resident-set size in CSV. The plotting tool creates SVG figures
without third-party Python packages.

See [C/report/report.md](C/report/report.md) for design details, pilot results,
and interpretation guidance.
