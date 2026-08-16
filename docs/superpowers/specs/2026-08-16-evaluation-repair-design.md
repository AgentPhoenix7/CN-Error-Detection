# Evaluation Repair Design

## Goal

Make the checked-in implementation, experiment evidence, plots, report, README,
and progress handoff satisfy the assignment PDF's evaluation requirements without
weakening reproducibility or hiding empirical limitations.

## Scope

The repair covers six audit findings:

1. randomly select error-injected frames while retaining deterministic replay;
2. make the three required checksum-versus-CRC outcomes explicit;
3. replace obsolete pilot prose and tables with the final campaign results;
4. explain the relevant theoretical detection properties of each CRC;
5. make the Markdown report reproducible from the experiment CSV;
6. separate the obsolete pre-mixed CSV from active evaluation artifacts.

The checksum, CRC, frame, TCP, and error-mutation algorithms remain unchanged.

## Random frame selection

The sender keeps its existing final command-line parameter and calls it
`ERROR_INTERVAL` for compatibility. For a non-`none` pattern, the value `N`
means that every frame is independently selected with probability `1/N`.
`N=1` selects every frame.

Two caller-owned RNG states are used:

- the selection RNG decides whether a frame is injected;
- the error RNG chooses the mutation positions or the random error type.

Both streams are derived deterministically from the supplied seed. Separating
them prevents an injection decision from changing subsequent error positions.
Runs with the same seed, frame geometry, pattern, and interval therefore select
the same frames and apply the same mutations. The sender's metrics continue to
report actual clean and injected counts rather than an expected count.

The selection rule is isolated in a small public error-injection helper so it
can be tested directly. Invalid input must not advance RNG state.

## Evaluation evidence

The runner continues to record one row per scheme, pattern, and repeat. A new
analysis step derives a compact comparison table for same-geometry schemes:

- checksum versus CRC-16 for a case detected by both;
- checksum versus CRC-10 for checksum-only detection;
- checksum versus CRC-16 for CRC-only detection.

The final report must identify the pattern, trial denominator, detections, and
misses behind each comparison. Because the CSV stores run-level totals rather
than frame identities, the report will label these as aggregate paired-trial
evidence and will not claim retained per-frame intersections. A permanent C
test will also pin one concrete deterministic corruption for each required
outcome so the three logical cases are directly reproducible.

## Results and documentation

After the behavior change, rerun the configured final campaign:

- 20 repeats per injected pattern on the 1 MiB input;
- 10 clean controls per scheme on the 8 MiB input;
- all five schemes and all four required error patterns.

Regenerate all SVGs from the new CSV, recompute report tables from that CSV,
and update README and PROGRESS so their run counts, semantics, metrics, and
next steps agree. Move the obsolete legacy CSV to a clearly named archive under
`C/results/raw/` while retaining its historical contents.

The report will add a concise theoretical table covering single-bit errors,
two-bit distance limits, odd-weight detection through the `(x + 1)` factor,
and guaranteed burst detection up to the CRC width. Empirical zero misses will
remain explicitly distinguished from theoretical guarantees.

## Reproducible report generation

`C/report/report.md` becomes a generated artifact rather than the editable
source of empirical values. Its editable source is
`C/report/report_template.md`, which contains the stable technical narrative,
methodology, theoretical interpretation, limitations, and named placeholders
for campaign-derived sections.

`C/tools/experiment_results.py` owns CSV loading, schema validation, invariant
checks, grouping, rates, timing summaries, resource summaries, and the three
rubric-aligned comparison rows. Both `plot_results.py` and the new
`generate_report.py` import this module. This single calculation path prevents
the plots and report from applying different denominators or aggregations.

`generate_report.py` reads `experiments_mixed.csv`, renders the campaign scope,
outcome tables, required comparison cases, timing/resource tables, and exact
observed caveats into the template, then writes `C/report/report.md`. The output
starts with a generated-file notice naming the source CSV, template, generator,
and regeneration command.

The Makefile adds:

```text
make -C C report
make -C C results
```

`report` regenerates only `report.md`. `results` regenerates both plots and the
report from the same CSV. Running experiments remains separate so generating
documentation never starts network processes unexpectedly.

The generator rejects missing groups, inconsistent repeats, broken frame-count
invariants, incompatible schemas, and absent template placeholders instead of
publishing a partial report. A repeat generation must be byte-for-byte stable.

## Error handling and compatibility

- `ERROR_INTERVAL=0` remains invalid.
- `none` never consumes either injection RNG and remains lossless.
- `ERROR_INTERVAL=1` remains the all-injected compatibility mode.
- Sender output, runner parsing, and the CSV schema remain compatible.
- All generated output paths remain under `C/output_files/`.

## Verification

Use red-green TDD for the selection helper and three comparative detection
fixtures. Then run:

```text
make -C C test
make -C C test_end_to_end
make -C C test_mixed_end_to_end
make -C C experiments_inference
make -C C plots
make -C C report
```

Independently validate CSV uniqueness, group coverage, row invariants, rates,
control byte equality, SVG XML structure, regenerated-plot equality, report
numbers, byte-stable report regeneration, strict `-fanalyzer`, and
`git diff --check`.
