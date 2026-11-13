# Assignment 1: requirements and verification audit

Audited on November 13, 2026 against the current C implementation and saved
450-run campaign. This is a traceability record, not an instructor-issued
report template or a claim that every submission requirement is complete.

## Sources and confirmed clarifications

- [Assignment PDF](<../ErrorDetection(Checksum and CRC).pdf>): course
  CSE/PC/B/S/314, CO1, Assignment 1; checksum/CRC requirements, evaluation cases,
  tables/graphs, execution time, and resource comparison.
- [Protocol illustration](../instruction.jpg): socket communication, header /
  payload / FCS layout, zero padding, source and destination MACs, a two-byte
  length, a one-byte sequence number, and complete frames of 64--1518 bytes.
- User clarification on November 13: use the illustration's **64--1518-byte
  complete-frame range** to resolve the PDF's 64--1500-byte wording.
- User clarification on November 13: **the project must include sequence
  numbers**, even though the PDF does not mention them.
- User clarification on November 13: no separate report-preparation rules are
  available; **final report formatting remains pending**.

The reference PDF and illustration are preserved unchanged. Their labels are
not silently rewritten to match the implementation. TCP, initial-zero
non-reflected CRC processing, and the precise payload-length semantics are
implementation choices, not additional statements from the PDF. The layout is
an Ethernet-sized simulation, not a complete IEEE 802.3 wire implementation.

## Requirement matrix

| ID | Requirement and source | Implementation / evidence | Status |
| --- | --- | --- | --- |
| A01 | Implement reusable checksum and CRC schemes; PDF | `C/include/checksum.h`, `C/include/crc.h`, corresponding `src/` modules; seven checksum and thirteen CRC checks | Implemented and tested |
| A02 | Sender accepts a test-file name on the command line; PDF | `sender INPUT SCHEME PATTERN [HOST] [PORT] [SEED] [ERROR_INTERVAL]`; binary control transfers | Implemented and tested |
| A03 | Sender and receiver communicate through sockets; illustration | POSIX TCP helpers and two separate executables; socket and end-to-end tests | Implemented and tested |
| A04 | Header / payload / FCS frame; illustration | Explicit 15-byte serialized header, zero-padded payload, scheme-specific FCS | Implemented and tested |
| A05 | Complete frame range 64--1518; confirmed illustration clarification | `config.h`; `frame_wire_size()`; scheme-specific payload capacities | Implemented and tested |
| A06 | Minimum padded payload of 46 bytes; PDF | CRC-32 currently permits 45-byte padding; other schemes pad to 47 or 48 bytes | **Open code gap** |
| A07 | Two six-byte MACs, two-byte length, one-byte sequence; illustration and user | Field sizes match. Length currently means valid unpadded payload bytes, rather than the illustration's “Length of Frame” label | Sizes implemented; **length meaning needs clarification** |
| A08 | Retain sequence numbers; user and illustration | Sender starts at 0, increments modulo 256; serialized byte 14 is parsed and FCS-protected | Implemented; live wrap and corruption probes passed |
| A09 | Zero padding after actual data in the last frame; illustration | `frame_serialize()` pads; receiver writes exactly the header's valid payload length | Implemented and tested |
| A10 | 16-bit one's-complement checksum; PDF | Network-order 16-bit words, end-around carry, odd-byte handling, complement and verifier | Implemented and tested |
| A11 | Four given CRC polynomials supplied as input; PDF | CLI selects `crc8`, `crc10`, `crc16`, or `crc32`; generic C API accepts parameter objects | Given catalogue implemented; **arbitrary polynomial CLI input not provided** |
| A12 | Separate error module, random positions, random sender invocation; PDF / illustration | Dedicated selection and mutation RNGs; mutation happens after FCS calculation and before sending | Implemented and tested |
| A13 | Single, two isolated, odd-weight, and burst errors; PDF | `single`, nonadjacent `two`, three-bit `odd`, 16-bit `burst`; additional mixed `random` mode | Implemented and tested; campaign uses one odd count and burst length |
| A14 | Receiver accepts/rejects datawords; PDF / illustration | Structural and FCS verification; rejected payload omitted; accepted payload can still contain undetected corruption | Implemented and tested |
| A15 | Both detect, checksum only, CRC only; PDF | Three deterministic fixtures in `test_evaluation.c`; campaign marginal bounds in generated report | All three demonstrated directly |
| A16 | Tables, graphs, execution time, resources; PDF | Validated CSV; four SVGs; per-process elapsed time, user/system CPU and Linux peak RSS | Implemented; environment-specific measurements |
| A17 | Follow separately supplied report-preparation instructions; PDF | No separate rules available, as confirmed by user | **Pending formatting requirements** |
| A18 | Demonstration, report submission and Drive softcopy; PDF | Original demonstration dates: August 10--14, 2026; report dates: August 17--21, 2026 | Dates recorded; delivery/upload not verified |

## Exact CRC catalogue

The CLI scheme is the polynomial input selector for the current application.
The four listed lower polynomials match the PDF, with the leading term implicit:

| Scheme | Polynomial | Lower hexadecimal polynomial | FCS bytes |
| --- | --- | --- | ---: |
| CRC-8 | x^8 + x^7 + x^6 + x^4 + x^2 + 1 | `0xD5` | 1 |
| CRC-10 | x^10 + x^9 + x^5 + x^4 + x + 1 | `0x233` | 2 |
| CRC-16 | x^16 + x^15 + x^2 + 1 | `0x8005` | 2 |
| CRC-32 | x^32 + x^26 + x^23 + x^22 + x^16 + x^12 + x^11 + x^10 + x^8 + x^7 + x^5 + x^4 + x^2 + x + 1 | `0x04C11DB7` | 4 |

All implementations use initial remainder zero, MSB-first processing, no
reflection, and no final XOR. The PDF names usage domains; it does not specify
those complete parameter conventions. These academic variants must not be
described as bit-for-bit implementations of Bluetooth, USB, or Ethernet FCS
profiles merely because the generator polynomials are the same.

## Sequence-number behavior

The byte is serialized after the two-byte payload length. It starts at zero for
each transfer and follows `0, 1, ..., 254, 255, 0, 1, ...`. Every header byte,
including this sequence byte, participates in FCS protection. The receiver
parses the value after verifying the frame.

TCP preserves record order. The receiver does not implement a separate
expected-sequence state machine, retransmissions, acknowledgements, or ARQ.
Rejected frames therefore leave holes in the accepted frame sequence; a
subsequent good frame must still be written. The sequence byte identifies a
position modulo 256; it is not a globally unique identifier. Undetected
sequence corruption is possible whenever a multi-bit error escapes the FCS.

Fresh diagnostic captures checked the sender's sequence bytes across 255-to-0
wrap for all five schemes on the 1 MiB input (699 frames for the two-byte
schemes and CRC-8; 700 for CRC-32). A separate probe flipped each of the eight
sequence bits independently and confirmed rejection for every scheme. These
probes are supplementary audit evidence, not new permanent test executables.

## Open issues and interpretation limits

1. **46-byte minimum payload:** accepted complete-frame bounds do not waive
   this independent PDF requirement. For a one-byte input, current CRC-32
   serialization produces a 15 + 45 + 4 = 64-byte frame. A 46-byte minimum
   padded payload would instead produce a valid 65-byte frame. Fixing this
   needs a coordinated change to configuration, frame sizing, boundary/wire
   tests, and affected evidence. The user explicitly excluded this repair
   (review problem 6) from the November 13 follow-up. It remains open.
2. **Length meaning:** the illustration says “Length of Frame”, while the
   implementation stores valid unpadded payload length. That preserves
   legitimate trailing zero bytes. The external TCP prefix stores complete
   serialized frame length, but it is not the same header field. Clarify the
   required semantics before changing the protocol.
3. **Polynomial input:** selecting a named supplied polynomial is supported.
   Literal runtime input of an arbitrary polynomial is not. Confirm whether
   the PDF requires coefficients/hexadecimal input beyond its four examples.
4. **Report preparation and submission:** Markdown sections, personal details,
   a cover page, page limits, PDF formatting, and upload conventions cannot be
   certified against unspecified rules. Formatting stays pending; no Drive
   upload, submission, or demonstration completion is claimed.

The initial documentation audit made no production source or wire-format
changes. The user subsequently authorized review repairs 1--5: experiment
failure handling, append identities, metadata validation, dispatcher RNG
atomicity, and local receiver launch arguments. Those repairs preserve frame
geometry and the recorded campaign. Do not mark this assignment fully compliant
or submission-ready while the open points remain.

## Evaluation evidence and limits

The campaign has 450 rows: 400 injected runs (five schemes, four patterns,
20 repeats on 1 MiB) and 50 clean controls (five schemes, 10 repeats on 8 MiB).
All four patterns are exercised for all five schemes. Error positions cover
the complete simulated frame, including header, payload padding, and FCS;
the two-byte TCP record prefix is excluded.

The three direct evaluation fixtures use the same original data and the same
flips for both algorithms: bit 0 in `12 34` (both detect), bits 0 and 511 in
256 zero bytes (checksum detects, CRC-10 misses), and bits 15 and 31 in
`00 00 00 01` (CRC-16 detects, checksum misses). They test reusable algorithms;
they are not presented as additional full sender/receiver campaign rows.

The CSV retains aggregate counts, not per-frame outcome intersections. The
checksum-only two-bit campaign count is bounded by 0--9, so it cannot prove a
nonzero checksum-only intersection by itself. The direct deterministic fixture
supplies that required evidence. Clean controls compare output byte for byte;
corrupted runs intentionally omit rejected payload and need not match input.

Reported detection counts include parser/padding rejections as well as FCS
mismatches. Treat them as receiver-level detection, not a pure count of nonzero
CRC/checksum syndromes. CPU and peak RSS are per-process measurements; timing
includes loopback transport and I/O. Finite zero-miss samples are not proofs of
zero theoretical miss probability.

## Repeatable verification

From the repository root:

```bash
make -C C all
make -C C test
make -C C test_end_to_end
make -C C test_mixed_end_to_end
make -C C results
```

Fresh follow-up checks passed: 104 C checks, 26 Python tests, five clean and five mixed
smoke transfers. Stored campaign validation covers 450 rows and 25 groups;
the report and plots are regenerated from that same CSV. The 450-run campaign
was produced during the preceding sampling/memory repair, not rerun solely to
edit documentation. Failure regressions are included in
`C/tests/test_run_experiments.py`, `C/tests/test_experiment_results.py`, and
`C/tests/test_sender_random.c`; the
[follow-up plan](superpowers/plans/2026-11-13-review-followup.md) records scope.

The documentation set is [README](../README.md), [agent guidelines](../AGENTS.md),
[progress handoff](../PROGRESS.md), this audit, the editable
[report template](../C/report/report_template.md), the generated
[report](../C/report/report.md), and the annotated historical plans/design under
`docs/superpowers/`. The historical files are not the current compliance verdict.
