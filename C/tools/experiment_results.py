#!/usr/bin/env python3
"""Load, validate, and aggregate error-detection experiment results."""

from __future__ import annotations

import csv
import json
from collections import defaultdict
from math import isfinite
from pathlib import Path
from statistics import mean, stdev


SCHEMES = ("checksum", "crc8", "crc10", "crc16", "crc32")
PATTERNS = ("none", "single", "two", "odd", "burst")
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
INTEGER_FIELDS = (
    "input_bytes",
    "output_bytes",
    "repeat",
    "error_interval",
    "frames_sent",
    "frames_received",
    "frames_injected",
    "frames_clean",
    "frames_accepted",
    "frames_rejected",
    "undetected_frames",
    "sender_max_rss_kb",
    "receiver_max_rss_kb",
)
FLOAT_FIELDS = (
    "detection_rate",
    "undetected_rate",
    "sender_elapsed_seconds",
    "sender_user_cpu_seconds",
    "sender_system_cpu_seconds",
    "receiver_elapsed_seconds",
    "receiver_user_cpu_seconds",
    "receiver_system_cpu_seconds",
    "total_cpu_seconds",
)


def _integer(row: dict[str, str], name: str, line_number: int) -> int:
    try:
        value = int(row[name])
    except (KeyError, ValueError) as error:
        raise ValueError(f"line {line_number}: invalid integer {name}") from error

    if value < 0:
        raise ValueError(f"line {line_number}: negative {name}")

    return value


def _floating(row: dict[str, str], name: str, line_number: int) -> float:
    try:
        value = float(row[name])
    except (KeyError, ValueError) as error:
        raise ValueError(f"line {line_number}: invalid number {name}") from error

    if not isfinite(value):
        raise ValueError(f"line {line_number}: non-finite {name}")

    if value < 0.0:
        raise ValueError(f"line {line_number}: negative {name}")

    return value


def _validate_row(row: dict[str, str], line_number: int) -> None:
    if row["scheme"] not in SCHEMES:
        raise ValueError(f"line {line_number}: invalid scheme")

    if row["error_pattern"] not in PATTERNS:
        raise ValueError(f"line {line_number}: invalid error_pattern")

    integers = {name: _integer(row, name, line_number) for name in INTEGER_FIELDS}
    floating = {name: _floating(row, name, line_number) for name in FLOAT_FIELDS}

    try:
        seed = int(row["seed"], 0)
    except ValueError as error:
        raise ValueError(f"line {line_number}: invalid seed") from error

    if seed < 0 or seed > 0xFFFF_FFFF:
        raise ValueError(f"line {line_number}: seed outside uint32 range")

    if integers["repeat"] < 1:
        raise ValueError(f"line {line_number}: repeat must be positive")

    sent = integers["frames_sent"]
    received = integers["frames_received"]
    injected = integers["frames_injected"]
    clean = integers["frames_clean"]
    accepted = integers["frames_accepted"]
    rejected = integers["frames_rejected"]
    undetected = integers["undetected_frames"]

    if sent != received:
        raise ValueError(f"line {line_number}: sent/received frame mismatch")

    if sent != injected + clean:
        raise ValueError(f"line {line_number}: invalid sender frame categories")

    if received != accepted + rejected:
        raise ValueError(f"line {line_number}: invalid receiver frame categories")

    if undetected != injected - rejected:
        raise ValueError(f"line {line_number}: invalid undetected count")

    if accepted != clean + undetected:
        raise ValueError(f"line {line_number}: invalid accepted-frame breakdown")

    expected_detection = rejected / injected if injected else 0.0
    expected_undetected = undetected / injected if injected else 0.0

    if abs(floating["detection_rate"] - expected_detection) > 5.1e-10:
        raise ValueError(f"line {line_number}: incorrect detection_rate")

    if abs(floating["undetected_rate"] - expected_undetected) > 5.1e-10:
        raise ValueError(f"line {line_number}: incorrect undetected_rate")

    expected_cpu = sum(
        floating[name]
        for name in (
            "sender_user_cpu_seconds",
            "sender_system_cpu_seconds",
            "receiver_user_cpu_seconds",
            "receiver_system_cpu_seconds",
        )
    )

    if abs(floating["total_cpu_seconds"] - expected_cpu) > 5.0e-9:
        raise ValueError(f"line {line_number}: incorrect total_cpu_seconds")

    if row["error_pattern"] == "none":
        if integers["error_interval"] != 0 or injected or rejected or undetected:
            raise ValueError(f"line {line_number}: invalid no-error control")

        if integers["input_bytes"] != integers["output_bytes"]:
            raise ValueError(f"line {line_number}: lossy no-error control")
    elif integers["error_interval"] < 1:
        raise ValueError(f"line {line_number}: invalid error_interval")


def load_rows(path: Path) -> list[dict[str, str]]:
    """Load rows after validating schema, values, invariants, and grain."""
    rows: list[dict[str, str]] = []
    grains: set[tuple[str, str, int]] = set()

    with path.open(newline="", encoding="utf-8") as source:
        reader = csv.DictReader(source)

        if tuple(reader.fieldnames or ()) != CSV_FIELDS:
            raise ValueError(f"incompatible experiment CSV schema: {path}")

        for line_number, row in enumerate(reader, start=2):
            _validate_row(row, line_number)
            grain = (row["scheme"], row["error_pattern"], int(row["repeat"]))

            if grain in grains:
                raise ValueError(
                    f"line {line_number}: duplicate scheme/pattern/repeat {grain}"
                )

            grains.add(grain)
            rows.append(row)

    if not rows:
        raise ValueError("results CSV contains no data rows")

    return rows


def summarize_rows(rows: list[dict[str, str]]) -> list[dict[str, str | float | int]]:
    """Pool frame outcomes and average process metrics by scheme and pattern."""
    grouped: dict[tuple[str, str], list[dict[str, str]]] = defaultdict(list)

    for row in rows:
        grouped[(row["scheme"], row["error_pattern"])].append(row)

    summaries: list[dict[str, str | float | int]] = []

    for (scheme, pattern), group_rows in grouped.items():
        for field in ("input_file", "input_bytes", "error_interval"):
            values = {
                row[field] if field == "input_file" else int(row[field])
                for row in group_rows
            }
            if len(values) != 1:
                raise ValueError(f"inconsistent {field} within {scheme}/{pattern} repeats")
        times = [float(row["receiver_elapsed_seconds"]) for row in group_rows]
        frames_injected = sum(int(row["frames_injected"]) for row in group_rows)
        frames_rejected = sum(int(row["frames_rejected"]) for row in group_rows)
        undetected_frames = sum(int(row["undetected_frames"]) for row in group_rows)
        summaries.append(
            {
                "scheme": scheme,
                "error_pattern": pattern,
                "runs": len(group_rows),
                "trial_signature": json.dumps(sorted(
                    (row["input_file"], int(row["input_bytes"]), int(row["repeat"]),
                     int(row["seed"], 0), int(row["error_interval"]), int(row["frames_injected"]))
                    for row in group_rows
                )),
                "input_file": group_rows[0]["input_file"],
                "error_interval": int(group_rows[0]["error_interval"]),
                "frames_clean": sum(int(row["frames_clean"]) for row in group_rows),
                "frames_injected": frames_injected,
                "frames_rejected": frames_rejected,
                "undetected_frames": undetected_frames,
                "detection_rate": (
                    frames_rejected / frames_injected if frames_injected else 0.0
                ),
                "undetected_rate": (
                    undetected_frames / frames_injected if frames_injected else 0.0
                ),
                "elapsed_seconds": mean(times),
                "elapsed_stdev": stdev(times) if len(times) > 1 else 0.0,
                "sender_elapsed_seconds": mean(
                    float(row["sender_elapsed_seconds"]) for row in group_rows
                ),
                "sender_cpu_seconds": mean(
                    float(row["sender_user_cpu_seconds"])
                    + float(row["sender_system_cpu_seconds"])
                    for row in group_rows
                ),
                "receiver_cpu_seconds": mean(
                    float(row["receiver_user_cpu_seconds"])
                    + float(row["receiver_system_cpu_seconds"])
                    for row in group_rows
                ),
                "sender_max_rss_kb": mean(
                    float(row["sender_max_rss_kb"]) for row in group_rows
                ),
                "receiver_max_rss_kb": mean(
                    float(row["receiver_max_rss_kb"]) for row in group_rows
                ),
                "input_bytes": mean(float(row["input_bytes"]) for row in group_rows),
                "output_bytes": mean(float(row["output_bytes"]) for row in group_rows),
            }
        )

    return summaries


def load_summaries(path: Path) -> list[dict[str, str | float | int]]:
    """Load and summarize one validated experiment CSV."""
    return summarize_rows(load_rows(path))


def summary_for(
    summaries: list[dict[str, str | float | int]],
    scheme: str,
    pattern: str,
) -> dict[str, str | float | int] | None:
    """Return one scheme/pattern summary when it is present."""
    return next(
        (
            summary
            for summary in summaries
            if summary["scheme"] == scheme and summary["error_pattern"] == pattern
        ),
        None,
    )


def validate_complete_campaign(
    summaries: list[dict[str, str | float | int]],
) -> None:
    """Require all scheme/pattern groups and consistent campaign dimensions."""
    expected = {(scheme, pattern) for scheme in SCHEMES for pattern in PATTERNS}
    actual = {
        (str(summary["scheme"]), str(summary["error_pattern"]))
        for summary in summaries
    }

    if actual != expected:
        missing = sorted(expected - actual)
        extra = sorted(actual - expected)
        raise ValueError(f"incomplete campaign groups; missing={missing}, extra={extra}")

    injected = [
        summary for summary in summaries if summary["error_pattern"] != "none"
    ]
    controls = [
        summary for summary in summaries if summary["error_pattern"] == "none"
    ]

    if len({int(summary["runs"]) for summary in injected}) != 1:
        raise ValueError("inconsistent injected-pattern repeat counts")

    if len({int(summary["runs"]) for summary in controls}) != 1:
        raise ValueError("inconsistent control repeat counts")

    if len({int(summary["error_interval"]) for summary in injected}) != 1:
        raise ValueError("inconsistent injected-pattern error intervals")

    if any(int(summary["frames_injected"]) == 0 for summary in injected):
        raise ValueError("complete campaign contains an injected group with zero trials")

    if len({float(summary["input_bytes"]) for summary in injected}) != 1:
        raise ValueError("inconsistent injected-pattern input sizes")

    if len({float(summary["input_bytes"]) for summary in controls}) != 1:
        raise ValueError("inconsistent control input sizes")

    if len({str(summary["input_file"]) for summary in injected}) != 1:
        raise ValueError("inconsistent injected-pattern input files")

    if len({str(summary["input_file"]) for summary in controls}) != 1:
        raise ValueError("inconsistent control input files")


def required_comparisons(
    summaries: list[dict[str, str | float | int]],
) -> list[dict[str, str | int]]:
    """Bound required-case intersections from paired aggregate marginals."""
    definitions = (
        ("Both detect", "single", "crc16"),
        ("Checksum only", "two", "crc10"),
        ("CRC only", "burst", "crc16"),
    )
    comparisons: list[dict[str, str | int]] = []

    for case, pattern, crc_scheme in definitions:
        checksum = summary_for(summaries, "checksum", pattern)
        crc = summary_for(summaries, crc_scheme, pattern)

        if checksum is None or crc is None:
            raise ValueError(f"missing summaries for required case {case}")

        trials = int(checksum["frames_injected"])
        crc_trials = int(crc["frames_injected"])

        if (
            trials != crc_trials
            or int(checksum["runs"]) != int(crc["runs"])
            or int(checksum["error_interval"]) != int(crc["error_interval"])
            or checksum.get("trial_signature") is None
            or checksum["trial_signature"] != crc.get("trial_signature")
        ):
            raise ValueError(f"unpaired aggregate trials for required case {case}")

        checksum_detected = int(checksum["frames_rejected"])
        crc_detected = int(crc["frames_rejected"])

        if case == "Both detect":
            lower = max(0, checksum_detected + crc_detected - trials)
            upper = min(checksum_detected, crc_detected)
        elif case == "Checksum only":
            lower = max(0, checksum_detected - crc_detected)
            upper = min(checksum_detected, trials - crc_detected)
        else:
            lower = max(0, crc_detected - checksum_detected)
            upper = min(crc_detected, trials - checksum_detected)

        comparisons.append(
            {
                "case": case,
                "pattern": pattern,
                "crc_scheme": crc_scheme,
                "trials": trials,
                "checksum_detected": checksum_detected,
                "crc_detected": crc_detected,
                "case_count_min": lower,
                "case_count_max": upper,
            }
        )

    return comparisons


if __name__ == "__main__":
    import argparse

    parser = argparse.ArgumentParser(description="Validate a complete experiment campaign.")
    parser.add_argument("results", type=Path)
    arguments = parser.parse_args()
    validate_complete_campaign(load_summaries(arguments.results))
    print(f"Validated complete campaign: {arguments.results}")
