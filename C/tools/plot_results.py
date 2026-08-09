#!/usr/bin/env python3
"""Create interpretation-focused SVG summaries from experiment CSV results."""

from __future__ import annotations

import argparse
import csv
import html
from collections import defaultdict
from pathlib import Path
from statistics import mean, stdev


SCHEMES = ("checksum", "crc8", "crc10", "crc16", "crc32")
PATTERNS = ("none", "single", "two", "odd", "burst")


def load_summaries(path: Path) -> list[dict[str, str | float | int]]:
    """Group repeated runs and calculate means and timing variability."""
    grouped: dict[tuple[str, str], list[dict[str, str]]] = defaultdict(list)

    with path.open(newline="", encoding="utf-8") as source:
        for row in csv.DictReader(source):
            grouped[(row["scheme"], row["error_pattern"])].append(row)

    summaries: list[dict[str, str | float | int]] = []

    for (scheme, pattern), rows in grouped.items():
        times = [float(row["elapsed_seconds"]) for row in rows]
        summaries.append(
            {
                "scheme": scheme,
                "error_pattern": pattern,
                "runs": len(rows),
                "detection_rate": mean(float(row["detection_rate"]) for row in rows),
                "elapsed_seconds": mean(times),
                "elapsed_stdev": stdev(times) if len(times) > 1 else 0.0,
                "max_rss_kb": mean(float(row["max_rss_kb"]) for row in rows),
            }
        )

    if not summaries:
        raise ValueError("results CSV contains no data rows")

    return summaries


def summary_for(
    summaries: list[dict[str, str | float | int]],
    scheme: str,
    pattern: str,
) -> dict[str, str | float | int] | None:
    """Return one scheme/pattern summary when it is present."""
    for summary in summaries:
        if summary["scheme"] == scheme and summary["error_pattern"] == pattern:
            return summary

    return None


def svg_document(title: str, width: int, height: int) -> list[str]:
    """Return the shared SVG header and title elements."""
    return [
        f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}" '
        f'viewBox="0 0 {width} {height}">',
        '<rect width="100%" height="100%" fill="white"/>',
        f'<text x="{width / 2}" y="34" text-anchor="middle" '
        f'font-family="sans-serif" font-size="24">{html.escape(title)}</text>',
    ]


def write_undetected_error_matrix(
    summaries: list[dict[str, str | float | int]], output_path: Path
) -> None:
    """Write a matrix that exposes rare undetected errors instead of hiding them."""
    patterns = PATTERNS[1:]
    width = 1000
    height = 470
    left = 190
    top = 120
    cell_width = 185
    cell_height = 48
    values = []

    for scheme in SCHEMES:
        for pattern in patterns:
            summary = summary_for(summaries, scheme, pattern)
            if summary is not None:
                values.append(100.0 * (1.0 - float(summary["detection_rate"])))

    maximum = max(values, default=0.0)
    elements = svg_document("Mean undetected-frame rate", width, height)
    elements.append(
        '<text x="500" y="62" text-anchor="middle" font-family="sans-serif" '
        'font-size="14">Injected-error runs only. Exact percentages are shown in every cell.</text>'
    )

    for index, pattern in enumerate(patterns):
        x = left + index * cell_width + cell_width / 2
        elements.append(
            f'<text x="{x}" y="103" text-anchor="middle" font-family="sans-serif" '
            f'font-size="14">{html.escape(pattern)} error</text>'
        )

    for row_index, scheme in enumerate(SCHEMES):
        y = top + row_index * cell_height
        elements.append(
            f'<text x="{left - 12}" y="{y + 31}" text-anchor="end" '
            f'font-family="sans-serif" font-size="15">{html.escape(scheme)}</text>'
        )

        for column_index, pattern in enumerate(patterns):
            summary = summary_for(summaries, scheme, pattern)
            value = 0.0 if summary is None else 100.0 * (
                1.0 - float(summary["detection_rate"])
            )
            intensity = 0.0 if maximum == 0.0 else value / maximum
            red = int(245 - 35 * intensity)
            green = int(245 - 150 * intensity)
            blue = int(245 - 150 * intensity)
            x = left + column_index * cell_width
            elements.extend(
                [
                    f'<rect x="{x}" y="{y}" width="{cell_width - 4}" '
                    f'height="{cell_height - 4}" fill="rgb({red},{green},{blue})" '
                    'stroke="#B8B8B8"/>',
                    f'<text x="{x + (cell_width - 4) / 2}" y="{y + 30}" '
                    'text-anchor="middle" font-family="sans-serif" font-size="15">'
                    f'{value:.2f}%</text>',
                ]
            )

    elements.extend(
        [
            '<text x="500" y="411" text-anchor="middle" font-family="sans-serif" '
            'font-size="13">Lower is better. A zero means no undetected frame was observed, '
            'not that the theoretical probability is zero.</text>',
            "</svg>",
        ]
    )
    output_path.write_text("\n".join(elements) + "\n", encoding="utf-8")


def write_control_timing_chart(
    summaries: list[dict[str, str | float | int]], output_path: Path
) -> None:
    """Write a control-only timing chart with standard-deviation error bars."""
    rows = [
        summary_for(summaries, scheme, "none")
        for scheme in SCHEMES
    ]
    rows = [row for row in rows if row is not None]
    width = 900
    height = 560
    left = 100
    right = 35
    top = 100
    bottom = 110
    chart_width = width - left - right
    chart_height = height - top - bottom
    maximum = max(
        (float(row["elapsed_seconds"]) + float(row["elapsed_stdev"])) * 1000.0
        for row in rows
    )
    maximum = maximum if maximum > 0.0 else 1.0
    elements = svg_document("Receiver time for no-error controls", width, height)
    elements.append(
        '<text x="450" y="62" text-anchor="middle" font-family="sans-serif" '
        'font-size="14">Mean receiver elapsed time in milliseconds; whiskers show one standard deviation.</text>'
    )
    elements.extend(
        [
            f'<line x1="{left}" y1="{top}" x2="{left}" y2="{top + chart_height}" stroke="#222"/>',
            f'<line x1="{left}" y1="{top + chart_height}" x2="{left + chart_width}" '
            f'y2="{top + chart_height}" stroke="#222"/>',
        ]
    )

    for tick in range(6):
        value = maximum * tick / 5.0
        y = top + chart_height - chart_height * tick / 5.0
        elements.extend(
            [
                f'<line x1="{left}" y1="{y:.1f}" x2="{left + chart_width}" '
                f'y2="{y:.1f}" stroke="#DDD"/>',
                f'<text x="{left - 8}" y="{y + 5:.1f}" text-anchor="end" '
                f'font-family="sans-serif" font-size="12">{value:.3f}</text>',
            ]
        )

    bar_space = chart_width / len(rows)
    bar_width = bar_space * 0.48
    for index, row in enumerate(rows):
        value = float(row["elapsed_seconds"]) * 1000.0
        deviation = float(row["elapsed_stdev"]) * 1000.0
        x = left + index * bar_space + (bar_space - bar_width) / 2
        y = top + chart_height - chart_height * value / maximum
        error_top = top + chart_height - chart_height * (value + deviation) / maximum
        error_bottom = top + chart_height - chart_height * max(value - deviation, 0.0) / maximum
        center = x + bar_width / 2
        elements.extend(
            [
                f'<rect x="{x:.1f}" y="{y:.1f}" width="{bar_width:.1f}" '
                f'height="{top + chart_height - y:.1f}" fill="#4C78A8"/>',
                f'<line x1="{center:.1f}" y1="{error_top:.1f}" x2="{center:.1f}" '
                f'y2="{error_bottom:.1f}" stroke="#222" stroke-width="2"/>',
                f'<line x1="{center - 7:.1f}" y1="{error_top:.1f}" x2="{center + 7:.1f}" '
                f'y2="{error_top:.1f}" stroke="#222" stroke-width="2"/>',
                f'<line x1="{center - 7:.1f}" y1="{error_bottom:.1f}" x2="{center + 7:.1f}" '
                f'y2="{error_bottom:.1f}" stroke="#222" stroke-width="2"/>',
                f'<text x="{center:.1f}" y="{top + chart_height + 28}" text-anchor="middle" '
                f'font-family="sans-serif" font-size="14">{html.escape(str(row["scheme"]))}</text>',
                f'<text x="{center:.1f}" y="{y - 8:.1f}" text-anchor="middle" '
                f'font-family="sans-serif" font-size="12">{value:.3f} ms</text>',
            ]
        )

    elements.extend(
        [
            f'<text x="24" y="{top + chart_height / 2}" text-anchor="middle" '
            f'transform="rotate(-90 24 {top + chart_height / 2})" '
            'font-family="sans-serif" font-size="15">Milliseconds</text>',
            '<text x="450" y="522" text-anchor="middle" font-family="sans-serif" '
            'font-size="13">This is application receiver time for a loopback transfer, not network latency.</text>',
            "</svg>",
        ]
    )
    output_path.write_text("\n".join(elements) + "\n", encoding="utf-8")


def write_resource_summary(
    summaries: list[dict[str, str | float | int]], output_path: Path
) -> None:
    """Write a table that states whether peak RSS actually differentiates schemes."""
    rows = []
    for scheme in SCHEMES:
        values = [
            float(summary["max_rss_kb"])
            for summary in summaries
            if summary["scheme"] == scheme
        ]
        if values:
            rows.append((scheme, mean(values)))

    distinct_values = {round(value, 3) for _, value in rows}
    width = 720
    height = 410
    elements = svg_document("Peak memory summary", width, height)
    conclusion = (
        "No observed difference: peak RSS is not a discriminating metric here."
        if len(distinct_values) <= 1
        else "Peak RSS differs across schemes in this experiment."
    )
    elements.append(
        f'<text x="360" y="65" text-anchor="middle" font-family="sans-serif" '
        f'font-size="14">{html.escape(conclusion)}</text>'
    )
    elements.extend(
        [
            '<rect x="180" y="100" width="360" height="42" fill="#E8EEF7" stroke="#888"/>',
            '<text x="250" y="127" text-anchor="middle" font-family="sans-serif" font-size="15">Scheme</text>',
            '<text x="450" y="127" text-anchor="middle" font-family="sans-serif" font-size="15">Mean peak RSS (KiB)</text>',
        ]
    )
    for index, (scheme, value) in enumerate(rows):
        y = 142 + index * 42
        elements.extend(
            [
                f'<rect x="180" y="{y}" width="360" height="42" fill="white" stroke="#B8B8B8"/>',
                f'<text x="250" y="{y + 27}" text-anchor="middle" font-family="sans-serif" '
                f'font-size="15">{html.escape(scheme)}</text>',
                f'<text x="450" y="{y + 27}" text-anchor="middle" font-family="sans-serif" '
                f'font-size="15">{value:.0f}</text>',
            ]
        )
    elements.append("</svg>")
    output_path.write_text("\n".join(elements) + "\n", encoding="utf-8")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("csv_path", type=Path)
    parser.add_argument("output_directory", type=Path)
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    summaries = load_summaries(args.csv_path)
    args.output_directory.mkdir(parents=True, exist_ok=True)
    write_undetected_error_matrix(summaries, args.output_directory / "undetected_rate.svg")
    write_control_timing_chart(summaries, args.output_directory / "execution_time.svg")
    write_resource_summary(summaries, args.output_directory / "memory_summary.svg")


if __name__ == "__main__":
    main()
