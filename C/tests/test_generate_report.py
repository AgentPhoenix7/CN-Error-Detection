"""Tests for deterministic Markdown report generation."""

from __future__ import annotations

import sys
import unittest
from pathlib import Path


TOOLS_DIRECTORY = Path(__file__).resolve().parents[1] / "tools"
sys.path.insert(0, str(TOOLS_DIRECTORY))

from generate_report import render_report  # noqa: E402


TOKENS = (
    "{{CAMPAIGN_SUMMARY}}",
    "{{OUTCOME_TABLE}}",
    "{{PATTERN_TABLE}}",
    "{{REQUIRED_CASES}}",
    "{{TIMING_TABLE}}",
    "{{RESOURCE_TABLE}}",
    "{{OBSERVED_MISSES}}",
)
SCHEMES = ("checksum", "crc8", "crc10", "crc16", "crc32")
PATTERNS = ("none", "single", "two", "odd", "burst")


def report_template() -> str:
    return "# Generated report\n\n" + "\n\n".join(TOKENS) + "\n"


def campaign_summaries() -> list[dict[str, str | float | int]]:
    summaries = []
    for scheme in SCHEMES:
        for pattern in PATTERNS:
            if pattern == "none":
                summaries.append(
                    {
                        "scheme": scheme,
                        "error_pattern": pattern,
                        "runs": 1,
                        "input_file": "input_files/performance.bin",
                        "error_interval": 0,
                        "frames_clean": 40,
                        "frames_injected": 0,
                        "frames_rejected": 0,
                        "undetected_frames": 0,
                        "detection_rate": 0.0,
                        "undetected_rate": 0.0,
                        "elapsed_seconds": 0.2,
                        "elapsed_stdev": 0.01,
                        "sender_elapsed_seconds": 0.1,
                        "sender_cpu_seconds": 0.03,
                        "receiver_cpu_seconds": 0.07,
                        "sender_max_rss_kb": 1000.0,
                        "receiver_max_rss_kb": 1100.0,
                        "input_bytes": 8_388_608.0,
                        "output_bytes": 8_388_608.0,
                    }
                )
                continue

            rejected = 10
            if scheme == "checksum" and pattern == "burst":
                rejected = 8
            elif scheme == "crc10" and pattern == "two":
                rejected = 9
            undetected = 10 - rejected
            summaries.append(
                {
                    "scheme": scheme,
                    "error_pattern": pattern,
                    "runs": 1,
                    "trial_signature": "paired-test",
                    "input_file": "input_files/detection.bin",
                    "error_interval": 2,
                    "frames_clean": 10,
                    "frames_injected": 10,
                    "frames_rejected": rejected,
                    "undetected_frames": undetected,
                    "detection_rate": rejected / 10,
                    "undetected_rate": undetected / 10,
                    "elapsed_seconds": 0.05,
                    "elapsed_stdev": 0.0,
                    "sender_elapsed_seconds": 0.04,
                    "sender_cpu_seconds": 0.02,
                    "receiver_cpu_seconds": 0.03,
                    "sender_max_rss_kb": 900.0,
                    "receiver_max_rss_kb": 950.0,
                    "input_bytes": 1_048_576.0,
                    "output_bytes": 524_288.0,
                }
            )
    return summaries


class GenerateReportTest(unittest.TestCase):
    def test_renders_complete_deterministic_report(self) -> None:
        summaries = campaign_summaries()

        first = render_report(report_template(), summaries, "results/test.csv")
        second = render_report(report_template(), summaries, "results/test.csv")

        self.assertEqual(first, second)
        self.assertTrue(first.startswith("<!--\nGenerated file; do not edit directly."))
        self.assertIn("Source CSV: `results/test.csv`", first)
        self.assertIn("25 runs", first)
        self.assertIn("Checksum only | two | CRC-10 | 10 | 10 | 9 | 1", first)
        self.assertIn("CRC only | burst | CRC-16 | 10 | 8 | 10 | 2", first)
        self.assertIn("Checksum | 40 | 40 | 38 | 2 | 95.0000%", first)
        self.assertIn("CRC-10 two-bit: 1 of 10", first)
        self.assertNotIn("{{", first)

    def test_rejects_missing_placeholder(self) -> None:
        template = report_template().replace("{{RESOURCE_TABLE}}\n\n", "")

        with self.assertRaisesRegex(ValueError, "RESOURCE_TABLE"):
            render_report(template, campaign_summaries(), "results/test.csv")

    def test_rejects_duplicate_placeholder(self) -> None:
        template = report_template() + "{{TIMING_TABLE}}\n"

        with self.assertRaisesRegex(ValueError, "TIMING_TABLE"):
            render_report(template, campaign_summaries(), "results/test.csv")


if __name__ == "__main__":
    unittest.main()
