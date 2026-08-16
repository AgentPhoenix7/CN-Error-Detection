#!/usr/bin/env python3
"""Generate the Markdown evaluation report from validated experiment results."""

from __future__ import annotations

import argparse
from pathlib import Path

from experiment_results import (
    PATTERNS,
    SCHEMES,
    load_summaries,
    required_comparisons,
    summary_for,
    validate_complete_campaign,
)


PLACEHOLDERS = (
    "{{CAMPAIGN_SUMMARY}}",
    "{{OUTCOME_TABLE}}",
    "{{PATTERN_TABLE}}",
    "{{REQUIRED_CASES}}",
    "{{TIMING_TABLE}}",
    "{{RESOURCE_TABLE}}",
    "{{OBSERVED_MISSES}}",
)


def scheme_label(scheme: str) -> str:
    """Return the report label for one command-line scheme name."""
    labels = {
        "checksum": "Checksum",
        "crc8": "CRC-8",
        "crc10": "CRC-10",
        "crc16": "CRC-16",
        "crc32": "CRC-32",
    }
    return labels[scheme]


def pattern_label(pattern: str) -> str:
    """Return the report label for one injected error pattern."""
    labels = {
        "single": "single-bit",
        "two": "two-bit",
        "odd": "odd three-bit",
        "burst": "16-bit burst",
    }
    return labels[pattern]


def campaign_summary(
    summaries: list[dict[str, str | float | int]],
) -> str:
    """Render campaign size, input, repeat, and selection details."""
    injected = next(
        summary for summary in summaries if summary["error_pattern"] != "none"
    )
    control = next(
        summary for summary in summaries if summary["error_pattern"] == "none"
    )
    total_runs = sum(int(summary["runs"]) for summary in summaries)
    return (
        f"The validated campaign contains **{total_runs} runs**. Each scheme and "
        f"injected pattern uses {int(injected['runs'])} repeats of the "
        f"{float(injected['input_bytes']) / 1_048_576.0:.2f} MiB detection input. "
        f"Each scheme also uses {int(control['runs'])} clean repeats of the "
        f"{float(control['input_bytes']) / 1_048_576.0:.2f} MiB performance input. "
        f"For injected runs, each frame is selected reproducibly with probability "
        f"1/{int(injected['error_interval'])}; the recorded clean and injected "
        "counts are observed outcomes, not assumed quotas."
    )


def outcome_table(
    summaries: list[dict[str, str | float | int]],
) -> str:
    """Render outcomes aggregated across the four injected patterns."""
    lines = [
        "| Scheme | Clean | Injected | Detected | Undetected | Detection rate |",
        "| --- | ---: | ---: | ---: | ---: | ---: |",
    ]
    for scheme in SCHEMES:
        rows = [
            summary
            for summary in summaries
            if summary["scheme"] == scheme and summary["error_pattern"] != "none"
        ]
        clean = sum(int(row["frames_clean"]) for row in rows)
        injected = sum(int(row["frames_injected"]) for row in rows)
        detected = sum(int(row["frames_rejected"]) for row in rows)
        undetected = sum(int(row["undetected_frames"]) for row in rows)
        rate = 100.0 * detected / injected
        lines.append(
            f"| {scheme_label(scheme)} | {clean:,} | {injected:,} | "
            f"{detected:,} | {undetected:,} | {rate:.4f}% |"
        )
    return "\n".join(lines)


def pattern_table(
    summaries: list[dict[str, str | float | int]],
) -> str:
    """Render undetected counts and rates for every scheme/pattern group."""
    injected_patterns = PATTERNS[1:]
    lines = [
        "| Scheme | "
        + " | ".join(pattern_label(pattern) for pattern in injected_patterns)
        + " |",
        "| --- | " + " | ".join("---:" for _ in injected_patterns) + " |",
    ]
    for scheme in SCHEMES:
        cells = []
        for pattern in injected_patterns:
            summary = summary_for(summaries, scheme, pattern)
            if summary is None:
                raise ValueError(f"missing summary for {scheme}/{pattern}")
            missed = int(summary["undetected_frames"])
            injected = int(summary["frames_injected"])
            rate = 100.0 * missed / injected
            cells.append(f"{missed:,} / {injected:,} ({rate:.4f}%)")
        lines.append(f"| {scheme_label(scheme)} | " + " | ".join(cells) + " |")
    return "\n".join(lines)


def required_cases_table(
    summaries: list[dict[str, str | float | int]],
) -> str:
    """Render the three assignment-required aggregate comparison cases."""
    lines = [
        "| Required case | Pattern | CRC | Trials | Checksum detected | "
        "CRC detected | Case count |",
        "| --- | --- | --- | ---: | ---: | ---: | ---: |",
    ]
    for comparison in required_comparisons(summaries):
        lines.append(
            f"| {comparison['case']} | {comparison['pattern']} | "
            f"{scheme_label(str(comparison['crc_scheme']))} | "
            f"{int(comparison['trials']):,} | "
            f"{int(comparison['checksum_detected']):,} | "
            f"{int(comparison['crc_detected']):,} | "
            f"{int(comparison['case_count']):,} |"
        )
    return "\n".join(lines)


def timing_table(
    summaries: list[dict[str, str | float | int]],
) -> str:
    """Render clean-control sender and receiver elapsed-time means."""
    lines = [
        "| Scheme | Sender mean (ms) | Receiver mean (ms) | Receiver SD (ms) |",
        "| --- | ---: | ---: | ---: |",
    ]
    for scheme in SCHEMES:
        summary = summary_for(summaries, scheme, "none")
        if summary is None:
            raise ValueError(f"missing clean timing summary for {scheme}")
        lines.append(
            f"| {scheme_label(scheme)} | "
            f"{1000.0 * float(summary['sender_elapsed_seconds']):.3f} | "
            f"{1000.0 * float(summary['elapsed_seconds']):.3f} | "
            f"{1000.0 * float(summary['elapsed_stdev']):.3f} |"
        )
    return "\n".join(lines)


def resource_table(
    summaries: list[dict[str, str | float | int]],
) -> str:
    """Render clean-control CPU and per-process peak RSS means."""
    lines = [
        "| Scheme | Sender CPU (ms) | Receiver CPU (ms) | Sender peak RSS "
        "(KiB) | Receiver peak RSS (KiB) |",
        "| --- | ---: | ---: | ---: | ---: |",
    ]
    for scheme in SCHEMES:
        summary = summary_for(summaries, scheme, "none")
        if summary is None:
            raise ValueError(f"missing clean resource summary for {scheme}")
        lines.append(
            f"| {scheme_label(scheme)} | "
            f"{1000.0 * float(summary['sender_cpu_seconds']):.3f} | "
            f"{1000.0 * float(summary['receiver_cpu_seconds']):.3f} | "
            f"{float(summary['sender_max_rss_kb']):.0f} | "
            f"{float(summary['receiver_max_rss_kb']):.0f} |"
        )
    return "\n".join(lines)


def observed_misses(
    summaries: list[dict[str, str | float | int]],
) -> str:
    """Render a sentence listing every nonzero empirical miss group."""
    entries = []
    for scheme in SCHEMES:
        for pattern in PATTERNS[1:]:
            summary = summary_for(summaries, scheme, pattern)
            if summary is None:
                raise ValueError(f"missing miss summary for {scheme}/{pattern}")
            missed = int(summary["undetected_frames"])
            if missed:
                entries.append(
                    f"{scheme_label(scheme)} {pattern_label(pattern)}: "
                    f"{missed:,} of {int(summary['frames_injected']):,}"
                )
    if not entries:
        return "No undetected injected frame was observed in this campaign."
    return "Observed undetected frames were " + "; ".join(entries) + "."


def render_report(
    template_text: str,
    summaries: list[dict[str, str | float | int]],
    source_name: str,
) -> str:
    """Validate and render one deterministic Markdown report."""
    validate_complete_campaign(summaries)
    replacements = {
        "{{CAMPAIGN_SUMMARY}}": campaign_summary(summaries),
        "{{OUTCOME_TABLE}}": outcome_table(summaries),
        "{{PATTERN_TABLE}}": pattern_table(summaries),
        "{{REQUIRED_CASES}}": required_cases_table(summaries),
        "{{TIMING_TABLE}}": timing_table(summaries),
        "{{RESOURCE_TABLE}}": resource_table(summaries),
        "{{OBSERVED_MISSES}}": observed_misses(summaries),
    }

    rendered = template_text
    for placeholder in PLACEHOLDERS:
        count = rendered.count(placeholder)
        if count != 1:
            raise ValueError(
                f"placeholder {placeholder} must appear exactly once; found {count}"
            )
        rendered = rendered.replace(placeholder, replacements[placeholder])

    if "{{" in rendered or "}}" in rendered:
        raise ValueError("report template contains an unknown placeholder")

    notice = (
        "<!--\n"
        "Generated file; do not edit directly.\n"
        "Generator: C/tools/generate_report.py\n"
        "Template: C/report/report_template.md\n"
        f"Source CSV: `{source_name}`\n"
        "Regenerate: make -C C report\n"
        "-->\n\n"
    )
    return notice + rendered


def generate_report(
    csv_path: Path,
    template_path: Path,
    output_path: Path,
) -> None:
    """Load validated inputs and write one generated report."""
    summaries = load_summaries(csv_path)
    rendered = render_report(
        template_path.read_text(encoding="utf-8"),
        summaries,
        str(csv_path),
    )
    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text(rendered, encoding="utf-8")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("csv_path", type=Path)
    parser.add_argument("template_path", type=Path)
    parser.add_argument("output_path", type=Path)
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    generate_report(args.csv_path, args.template_path, args.output_path)


if __name__ == "__main__":
    main()
