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
    def setUp(self) -> None:
        self.temporary_directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary_directory.cleanup)
        self.csv_path = Path(self.temporary_directory.name) / "results.csv"

    def write_rows(self, rows: list[dict[str, str]]) -> None:
        with self.csv_path.open("w", newline="", encoding="utf-8") as output:
            writer = csv.DictWriter(output, fieldnames=CSV_FIELDS)
            writer.writeheader()
            writer.writerows(rows)

    def test_rejects_incompatible_schema(self) -> None:
        self.csv_path.write_text("scheme,error_pattern\nchecksum,none\n", encoding="utf-8")

        with self.assertRaisesRegex(ValueError, "schema"):
            load_rows(self.csv_path)

    def test_rejects_duplicate_experiment_grain(self) -> None:
        row = experiment_row("checksum", "single")
        self.write_rows([row, row.copy()])

        with self.assertRaisesRegex(ValueError, "duplicate"):
            load_rows(self.csv_path)

    def test_rejects_broken_frame_invariant(self) -> None:
        row = experiment_row("checksum", "single")
        row["frames_accepted"] = "9"
        self.write_rows([row])

        with self.assertRaises(ValueError):
            load_rows(self.csv_path)

    def test_rejects_incorrect_stored_rate(self) -> None:
        row = experiment_row("checksum", "single", rejected=9)
        row["detection_rate"] = "1.000000000"
        self.write_rows([row])

        with self.assertRaisesRegex(ValueError, "detection_rate"):
            load_rows(self.csv_path)

    def test_rejects_nonfinite_metric(self) -> None:
        row = experiment_row("checksum", "single")
        row["sender_elapsed_seconds"] = "nan"
        self.write_rows([row])

        with self.assertRaisesRegex(ValueError, "non-finite"):
            load_rows(self.csv_path)

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
                    "case_count": 10,
                },
                {
                    "case": "Checksum only",
                    "pattern": "two",
                    "crc_scheme": "crc10",
                    "trials": 10,
                    "checksum_detected": 10,
                    "crc_detected": 9,
                    "case_count": 1,
                },
                {
                    "case": "CRC only",
                    "pattern": "burst",
                    "crc_scheme": "crc16",
                    "trials": 10,
                    "checksum_detected": 8,
                    "crc_detected": 10,
                    "case_count": 2,
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


if __name__ == "__main__":
    unittest.main()
