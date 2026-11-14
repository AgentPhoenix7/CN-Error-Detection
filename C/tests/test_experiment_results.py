"""Tests for validated experiment-result loading and aggregation."""

from __future__ import annotations

import csv
import sys
import tempfile
import unittest
from pathlib import Path


TOOLS_DIRECTORY = Path(__file__).resolve().parents[1] / "tools"
sys.path.insert(0, str(TOOLS_DIRECTORY))

from experiment_results import (  # noqa: E402
    load_rows,
    load_summaries,
    required_comparisons,
    validate_complete_campaign,
)


CSV_FIELDS = (
    "input_file",
    "output_file",
    "input_bytes",
    "output_bytes",
    "scheme",
    "error_pattern",
    "repeat",
    "seed",
    "error_interval",
    "frames_sent",
    "frames_received",
    "frames_injected",
    "frames_clean",
    "frames_accepted",
    "frames_rejected",
    "undetected_frames",
    "detection_rate",
    "undetected_rate",
    "sender_elapsed_seconds",
    "sender_user_cpu_seconds",
    "sender_system_cpu_seconds",
    "sender_max_rss_kb",
    "receiver_elapsed_seconds",
    "receiver_user_cpu_seconds",
    "receiver_system_cpu_seconds",
    "receiver_max_rss_kb",
    "total_cpu_seconds",
)
SCHEMES = ("checksum", "crc8", "crc10", "crc16", "crc32")
PATTERNS = ("none", "single", "two", "odd", "burst")


# Builds a synthetic valid CSV row for validator tests. Derives clean,
# injected, accepted and missed counts from the chosen pattern and rejection
# count, then fills stable metadata and consistent process metrics.
def experiment_row(
    scheme: str,
    pattern: str,
    repeat: int = 1,
    *,
    injected: int = 10,
    rejected: int = 10,
) -> dict[str, str]:
    if pattern == "none":
        injected = 0
        rejected = 0
        clean = 20
        undetected = 0
        accepted = 20
        error_interval = 0
        input_bytes = output_bytes = 100
    else:
        clean = 10
        undetected = injected - rejected
        accepted = clean + undetected
        error_interval = 2
        input_bytes = 100
        output_bytes = accepted * 5

    return {
        "input_file": "input.bin",
        "output_file": f"{scheme}-{pattern}.bin",
        "input_bytes": str(input_bytes),
        "output_bytes": str(output_bytes),
        "scheme": scheme,
        "error_pattern": pattern,
        "repeat": str(repeat),
        "seed": "0x12345678",
        "error_interval": str(error_interval),
        "frames_sent": str(clean + injected),
        "frames_received": str(clean + injected),
        "frames_injected": str(injected),
        "frames_clean": str(clean),
        "frames_accepted": str(accepted),
        "frames_rejected": str(rejected),
        "undetected_frames": str(undetected),
        "detection_rate": f"{rejected / injected:.9f}" if injected else "0.000000000",
        "undetected_rate": f"{undetected / injected:.9f}" if injected else "0.000000000",
        "sender_elapsed_seconds": "0.100000000",
        "sender_user_cpu_seconds": "0.010000000",
        "sender_system_cpu_seconds": "0.020000000",
        "sender_max_rss_kb": "1000",
        "receiver_elapsed_seconds": "0.200000000",
        "receiver_user_cpu_seconds": "0.030000000",
        "receiver_system_cpu_seconds": "0.040000000",
        "receiver_max_rss_kb": "1100",
        "total_cpu_seconds": "0.100000000",
    }


class ExperimentResultsTest(unittest.TestCase):
    # Creates an isolated results path for each validation test. Allocates a
    # temporary directory, registers its cleanup, and stores a CSV filename
    # inside it.
    def setUp(self) -> None:
        self.temporary_directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary_directory.cleanup)
        self.csv_path = Path(self.temporary_directory.name) / "results.csv"

    # Writes synthetic rows with the production CSV column order. Opens the
    # test file with CSV-safe newline handling, writes the fixed header, and
    # serializes the supplied row dictionaries.
    def write_rows(self, rows: list[dict[str, str]]) -> None:
        with self.csv_path.open("w", newline="", encoding="utf-8") as output:
            writer = csv.DictWriter(output, fieldnames=CSV_FIELDS)
            writer.writeheader()
            writer.writerows(rows)

    # Checks rejection of a CSV with missing columns. Writes only scheme and
    # error-pattern columns and requires load_rows() to raise a schema error.
    def test_rejects_incompatible_schema(self) -> None:
        self.csv_path.write_text("scheme,error_pattern\nchecksum,none\n", encoding="utf-8")

        with self.assertRaisesRegex(ValueError, "schema"):
            load_rows(self.csv_path)

    # Checks rejection of duplicate experiment identities. Writes two copies of
    # the same row and requires the loader to reject the repeated
    # scheme/pattern/repeat key.
    def test_rejects_duplicate_experiment_grain(self) -> None:
        row = experiment_row("checksum", "single")
        self.write_rows([row, row.copy()])

        with self.assertRaisesRegex(ValueError, "duplicate"):
            load_rows(self.csv_path)

    # Checks enforcement of receiver frame accounting. Changes a valid row's
    # accepted count so it no longer reconciles with received frames and
    # requires a validation error.
    def test_rejects_broken_frame_invariant(self) -> None:
        row = experiment_row("checksum", "single")
        row["frames_accepted"] = "9"
        self.write_rows([row])

        with self.assertRaises(ValueError):
            load_rows(self.csv_path)

    # Checks that a stored detection rate must match its counts. Creates nine
    # detections out of ten trials, overwrites the rate with one, and requires
    # a detection-rate error.
    def test_rejects_incorrect_stored_rate(self) -> None:
        row = experiment_row("checksum", "single", rejected=9)
        row["detection_rate"] = "1.000000000"
        self.write_rows([row])

        with self.assertRaisesRegex(ValueError, "detection_rate"):
            load_rows(self.csv_path)

    # Checks rejection of NaN process measurements. Replaces a valid elapsed-
    # time field with nan and requires the non-finite-number validation error.
    def test_rejects_nonfinite_metric(self) -> None:
        row = experiment_row("checksum", "single")
        row["sender_elapsed_seconds"] = "nan"
        self.write_rows([row])

        with self.assertRaisesRegex(ValueError, "non-finite"):
            load_rows(self.csv_path)

    # Checks full campaign aggregation and exact comparison cases. Builds all
    # 25 groups with controlled misses, validates them, and compares the
    # required-case dictionaries and checksum burst miss rate with expected
    # values.
    def test_validates_complete_campaign_and_required_cases(self) -> None:
        rows = []
        for scheme in SCHEMES:
            for pattern in PATTERNS:
                rejected = 10
                if scheme == "checksum" and pattern == "burst":
                    rejected = 8
                elif scheme == "crc10" and pattern == "two":
                    rejected = 9
                rows.append(experiment_row(scheme, pattern, rejected=rejected))
        self.write_rows(rows)

        summaries = load_summaries(self.csv_path)
        validate_complete_campaign(summaries)

        self.assertEqual(
            required_comparisons(summaries),
            [
                {
                    "case": "Both detect",
                    "pattern": "single",
                    "crc_scheme": "crc16",
                    "trials": 10,
                    "checksum_detected": 10,
                    "crc_detected": 10,
                    "case_count_min": 10,
                    "case_count_max": 10,
                },
                {
                    "case": "Checksum only",
                    "pattern": "two",
                    "crc_scheme": "crc10",
                    "trials": 10,
                    "checksum_detected": 10,
                    "crc_detected": 9,
                    "case_count_min": 1,
                    "case_count_max": 1,
                },
                {
                    "case": "CRC only",
                    "pattern": "burst",
                    "crc_scheme": "crc16",
                    "trials": 10,
                    "checksum_detected": 8,
                    "crc_detected": 10,
                    "case_count_min": 2,
                    "case_count_max": 2,
                },
            ],
        )

        checksum_burst = next(
            summary
            for summary in summaries
            if summary["scheme"] == "checksum"
            and summary["error_pattern"] == "burst"
        )
        self.assertEqual(checksum_burst["undetected_frames"], 2)
        self.assertAlmostEqual(float(checksum_burst["undetected_rate"]), 0.2)

    # Checks that aggregate counts yield honest overlap bounds. Makes checksum
    # miss two trials and CRC-10 miss one in the paired two-bit group, then
    # requires the checksum-only interval zero through one.
    def test_required_comparisons_bound_unknown_intersections(self) -> None:
        rows = [experiment_row(scheme, pattern) for scheme in SCHEMES for pattern in PATTERNS]
        rows = [
            experiment_row(row["scheme"], row["error_pattern"], rejected=8)
            if (row["scheme"], row["error_pattern"]) == ("checksum", "two")
            else experiment_row(row["scheme"], row["error_pattern"], rejected=9)
            if (row["scheme"], row["error_pattern"]) == ("crc10", "two")
            else row for row in rows
        ]
        self.write_rows(rows)
        comparison = required_comparisons(load_summaries(self.csv_path))[1]
        self.assertEqual(comparison["case_count_min"], 0)
        self.assertEqual(comparison["case_count_max"], 1)

    # Checks that comparison groups must represent paired trials. Changes only
    # the CRC-10 two-bit seed in an otherwise complete campaign and requires an
    # unpaired-trials error.
    def test_required_comparisons_reject_unpaired_seeds(self) -> None:
        rows = [experiment_row(scheme, pattern) for scheme in SCHEMES for pattern in PATTERNS]
        next(row for row in rows if row["scheme"] == "crc10" and row["error_pattern"] == "two")["seed"] = "0x12345679"
        self.write_rows(rows)
        with self.assertRaisesRegex(ValueError, "unpaired"):
            required_comparisons(load_summaries(self.csv_path))

    # Checks stable input metadata across repeats of one group. Changes the
    # second repeat's input path, byte count, or error denominator in turn and
    # requires a field-specific inconsistency error.
    def test_rejects_metadata_changes_within_group(self) -> None:
        for field, value in (("input_file", "other.bin"), ("input_bytes", "101"), ("error_interval", "3")):
            with self.subTest(field=field):
                first = experiment_row("crc8", "single", repeat=1)
                second = experiment_row("crc8", "single", repeat=2)
                second[field] = value
                self.write_rows([first, second])
                with self.assertRaisesRegex(ValueError, "inconsistent.*" + field):
                    load_summaries(self.csv_path)

    # Checks that campaign error groups use the same input corpus. Changes one
    # CRC-8 single-bit group's input path and requires complete-campaign
    # validation to reject the mixed corpora.
    def test_rejects_different_corpora_in_complete_campaign(self) -> None:
        rows = [experiment_row(scheme, pattern) for scheme in SCHEMES for pattern in PATTERNS]
        next(row for row in rows if row["scheme"] == "crc8" and row["error_pattern"] == "single")["input_file"] = "different.bin"
        self.write_rows(rows)
        with self.assertRaisesRegex(ValueError, "input files"):
            validate_complete_campaign(load_summaries(self.csv_path))


if __name__ == "__main__":
    unittest.main()
