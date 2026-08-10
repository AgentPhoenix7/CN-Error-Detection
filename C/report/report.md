# Checksum and CRC Error-Detection Simulation

## 1. Technical summary

The sender now transfers large text inputs using a mixture of clean and
error-injected frames. In an injected-pattern run, one frame in every two is
selected for corruption by default; the other frames are transmitted without
modification and act as an internal acceptance control. A separate `none` run
keeps every frame clean and must reproduce the complete input byte-for-byte.

The checked-in pilot contains 75 runs. Error-detection trials use a 1 MiB text
file with three repeats for every scheme and error pattern. No-error resource
controls use an 8 MiB text file with three repeats per scheme. Across the
injected-pattern trials, each scheme processed about 4,200 clean and 4,200
injected frames. CRC-16 and CRC-32 rejected every injected frame in this
sample; the observed misses were four checksum burst frames, twelve CRC-8
two-bit frames, and one CRC-10 two-bit frame. These are observations for the
selected data and seeds, not theoretical guarantees.

Resource measurements now cover both processes. The CSV records sender and
receiver elapsed time, user CPU time, system CPU time, and peak resident-set
size (RSS), together with input and output file sizes. On the 8 MiB controls,
mean receiver elapsed time ranged from 0.052 seconds for checksum to 1.671
seconds for CRC-32. This reflects the transparent bitwise implementations on
one loopback host; it is not a measurement of real network latency.

## 2. Scope and experiment files

Files intended for transmission are stored separately from received files:

- `C/input_files/detection_1m.txt` is exactly 1,048,576 bytes;
- `C/input_files/performance_8m.txt` is exactly 8,388,608 bytes; and
- `C/output_files/` receives the latest output for each input, scheme, and
  pattern combination.

Both large inputs are deterministic, human-readable ASCII text and can be
regenerated. Experiment outputs are ignored by Git because corrupted runs can
produce many large, incomplete files. The directory itself remains present
through `.gitkeep`.

## 3. Implementation

The implementation is pure C17 and uses POSIX TCP sockets. TCP was selected so
transport loss and reordering do not confound the error-detection experiment.
The sender deliberately corrupts selected serialized frames after calculating
their FCS; the TCP record prefix itself is never corrupted.

### 3.1 Simulated frame

The fixed 15-byte header is serialized field-by-field:

| Field | Bytes | Encoding |
| --- | ---: | --- |
| Source MAC | 6 | Raw bytes |
| Destination MAC | 6 | Raw bytes |
| Actual payload length | 2 | Unsigned, network byte order |
| Sequence number | 1 | Unsigned byte |

Payload padding and maximum chunk size depend on the selected FCS. CRC-8 uses
48--1502 transmitted payload bytes; the two-byte schemes use 47--1501 bytes;
and CRC-32 uses 45--1499 bytes. These limits keep every complete frame within
64--1518 bytes. The FCS covers the serialized header and complete padded
payload, but not the FCS field itself.

CRC-10 uses a two-byte network-order FCS. Its ten remainder bits are
left-aligned and its low six bits are zero. The receiver rejects nonzero
padding bits before shifting the remainder right for verification.

### 3.2 Error-detection algorithms

The checksum treats adjacent input bytes as network-order 16-bit words, folds
end-around carries, and complements the final sum. An odd final byte occupies
the high half of its word.

All CRC variants use generic MSB-first polynomial division:

| Scheme | Width | Lower polynomial |
| --- | ---: | ---: |
| CRC-8 | 8 | `0xD5` |
| CRC-10 | 10 | `0x233` |
| CRC-16 | 16 | `0x8005` |
| CRC-32 | 32 | `0x04C11DB7` |

The initial remainder is zero, there is no reflection, and there is no final
XOR. Width masking is safe for the complete supported range of 1--32 bits.

### 3.3 Mixed clean and error-injected frames

The sender supports one random bit, two distinct nonadjacent bits, three
distinct bits, a contiguous 16-bit burst, and a random per-injection choice
among those patterns. Frame index zero and then every `ERROR_INTERVAL`th frame
are selected for injection. The default interval is two, so approximately
half the frames remain clean. Setting the interval to one intentionally
restores the old all-frames-corrupted behavior for a controlled comparison.

The fixed generator recurrence is
`state = state * 1664525 + 1013904223` using defined `uint32_t` wraparound.
Every injection operation validates its complete request before modifying
data, so rejected operations preserve both bytes and generator state.

## 4. Receiver behavior and metric definitions

TCP records use an external two-byte length followed by one complete simulated
frame. A zero length ends the transfer. The receiver validates the frame and
writes only the actual unpadded payload of accepted frames. Rejected frames are
not written, so an error-run output is normally shorter than its input.

The Python runner combines sender and receiver process metrics into one CSV
row. The important frame categories are:

- `frames_clean`: frames not selected for error injection;
- `frames_injected`: frames modified after FCS calculation;
- `frames_rejected`: frames whose corruption was detected;
- `undetected_frames = frames_injected - frames_rejected`; and
- `detection_rate = frames_rejected / frames_injected`.

Clean frames are deliberately excluded from the detection-rate denominator.
Using all received frames would incorrectly report a rate near 50% even if
every injected error were detected. For a `none` control, `frames_injected` is
zero and the reported detection and undetected rates are zero by definition.

Elapsed time is measured with `CLOCK_MONOTONIC`. User and system CPU time and
peak RSS come from `getrusage(RUSAGE_SELF)` in each process. RSS values are
per-process maxima; adding the two values would not establish the simultaneous
combined peak.

## 5. Verification

The automated C suite covers normal, boundary, malformed, and atomic-rejection
behavior for checksum, CRC, error injection, frame handling, and exact-length
socket I/O. Frame tests pin every FCS wire representation, CRC-10 padding,
network byte order, minimum padding, maximum payload length, corruption
rejection, and output preservation on invalid input.

End-to-end `none` controls compare output against input byte-for-byte for all
five schemes. The mixed-frame smoke test also checks these invariants for each
row:

- sent frames equal received frames;
- sent frames equal clean plus injected frames;
- received frames equal accepted plus rejected frames; and
- accepted frames equal clean plus undetected injected frames.

## 6. Pilot results

### 6.1 Approximately half the frames remained clean

The table aggregates three repeats across all four injected patterns. The
clean and injected counts differ by at most twelve frames because the sender
always selects frame index zero and some schemes produce an odd total number
of frames.

| Scheme | Clean frames | Injected frames | Rejected | Undetected | Observed detection |
| --- | ---: | ---: | ---: | ---: | ---: |
| Checksum | 4,188 | 4,200 | 4,196 | 4 | 99.9048% |
| CRC-8 | 4,188 | 4,200 | 4,188 | 12 | 99.7143% |
| CRC-10 | 4,188 | 4,200 | 4,199 | 1 | 99.9762% |
| CRC-16 | 4,188 | 4,200 | 4,200 | 0 | 100.0000% |
| CRC-32 | 4,200 | 4,200 | 4,200 | 0 | 100.0000% |

The composition chart makes the internal control visible: roughly half of
each bar is clean input accepted without an injected error, while the other
half is divided into detected and undetected injected errors.

![Frame outcomes](../results/plots/frame_outcomes.svg)

### 6.2 Undetected rates use only injected frames

The matrix separates the error patterns. The only misses in this pilot were
checksum burst errors (4 of 1,050), CRC-8 two-bit errors (12 of 1,050), and one
CRC-10 two-bit error (1 of 1,050). A zero cell means no miss was observed in
this sample; it does not mean the theoretical miss probability is zero.

![Undetected error rate](../results/plots/undetected_rate.svg)

### 6.3 No-error controls isolate implementation timing

Timing uses only the 8 MiB `none` controls. This avoids comparing a scheme that
processed and wrote clean payloads with another run that rejected half its
frames. Whiskers show one standard deviation across three runs.

![Execution time](../results/plots/execution_time.svg)

### 6.4 Sender and receiver resources are reported separately

The resource table reports CPU and peak RSS for both processes on the same 8
MiB controls. CRC CPU time is much higher than checksum time because the CRCs
use transparent bit-by-bit division. Peak RSS varied little and is therefore
not a useful discriminator in this pilot.

![Process resource usage](../results/plots/resource_usage.svg)

## 7. Limitations and robustness

The checked-in three-repeat campaign is a pilot, not a final probability
estimate. The configured `experiments_inference` target increases detection
trials to 20 repeats and resource controls to 10 repeats. Even that empirical
campaign cannot prove a theoretical probability of zero when no misses are
observed.

All timing uses two processes on one loopback host. The measurements capture
application serialization, FCS calculation, verification, socket copying, and
file I/O in this environment. They do not measure wide-area latency, packet
loss, or physical-network throughput. Scheduler load and OS caching can also
affect short runs.

The current CRC implementation intentionally favors an auditable academic
model over lookup-table optimization. The performance findings compare these
implementations rather than the best achievable implementation of each CRC.

## 8. Recommended next steps and reproduction

For the final submission, run the full inference target on the demonstration
machine, regenerate the plots, and preserve the resulting CSV with the machine
and compiler details. If timing variability remains large, increase the
8 MiB control size or repeat count before drawing performance conclusions.

From the repository root:

```bash
make -C C test
make -C C test_end_to_end
make -C C test_mixed_end_to_end
make -C C experiments_inference
make -C C plots
```

Python utilities run through uv using `C/pyproject.toml` and `C/uv.lock`.
Regenerate the deterministic large text inputs with:

```bash
make -C C generate_test_data
```

Change the mix with `ERROR_INTERVAL=<positive-count>`. For example,
`ERROR_INTERVAL=4` injects the selected pattern into one in every four frames
and leaves the other three frames clean.

## 9. Further questions

- Do the observed miss rates remain stable with more seeds and a different
  text corpus?
- Does selecting injection slots randomly, while preserving the same expected
  error rate, change any result compared with the fixed interval?
- How much do lookup-table CRC implementations reduce CPU time while retaining
  the same wire-level results?
