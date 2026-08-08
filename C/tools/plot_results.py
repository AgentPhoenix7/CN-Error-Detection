#!/usr/bin/env python3
"""Create dependency-free SVG charts from experiment CSV results."""

from __future__ import annotations

import argparse
import csv
import html
from collections import defaultdict
from pathlib import Path
from statistics import mean


SCHEME_COLORS = {
    "checksum": "#3366CC",
    "crc8": "#DC3912",
    "crc10": "#FF9900",
    "crc16": "#109618",
    "crc32": "#990099",
}


def load_averages(path: Path) -> list[dict[str, str | float]]:
    """Group repeated CSV rows and average each quantitative metric."""
    grouped: dict[tuple[str, str], list[dict[str, str]]] = defaultdict(list)

    with path.open(newline="", encoding="utf-8") as source:
        for row in csv.DictReader(source):
            grouped[(row["scheme"], row["error_pattern"])].append(row)

    output: list[dict[str, str | float]] = []

    for (scheme, pattern), rows in sorted(grouped.items()):
        output.append(
            {
                "scheme": scheme,
                "error_pattern": pattern,
                "detection_rate": mean(float(row["detection_rate"]) for row in rows),
                "elapsed_seconds": mean(float(row["elapsed_seconds"]) for row in rows),
                "max_rss_kb": mean(float(row["max_rss_kb"]) for row in rows),
            }
        )

    if not output:
        raise ValueError("results CSV contains no data rows")

    return output


def write_bar_chart(
    rows: list[dict[str, str | float]],
    metric: str,
    title: str,
    y_label: str,
    output_path: Path,
) -> None:
    """Write one compact SVG bar chart."""
    width = 1200
    height = 620
    left = 90
    right = 30
    top = 70
    bottom = 150
    chart_width = width - left - right
    chart_height = height - top - bottom
    values = [float(row[metric]) for row in rows]
    maximum = max(values)

    if maximum <= 0.0:
        maximum = 1.0

    gap = 8.0
    bar_width = max(4.0, chart_width / len(rows) - gap)
    elements = [
        f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}" '
        f'viewBox="0 0 {width} {height}">',
        '<rect width="100%" height="100%" fill="white"/>',
        f'<text x="{width / 2}" y="34" text-anchor="middle" '
        f'font-family="sans-serif" font-size="24">{html.escape(title)}</text>',
        f'<line x1="{left}" y1="{top}" x2="{left}" y2="{top + chart_height}" '
        'stroke="#222"/>',
        f'<line x1="{left}" y1="{top + chart_height}" x2="{left + chart_width}" '
        f'y2="{top + chart_height}" stroke="#222"/>',
        f'<text x="22" y="{top + chart_height / 2}" text-anchor="middle" '
        f'transform="rotate(-90 22 {top + chart_height / 2})" '
        f'font-family="sans-serif" font-size="15">{html.escape(y_label)}</text>',
    ]

    for tick in range(6):
        value = maximum * tick / 5.0
        y = top + chart_height - chart_height * tick / 5.0
        elements.extend(
            [
                f'<line x1="{left}" y1="{y:.1f}" x2="{left + chart_width}" '
                f'y2="{y:.1f}" stroke="#DDD"/>',
                f'<text x="{left - 8}" y="{y + 5:.1f}" text-anchor="end" '
                f'font-family="sans-serif" font-size="12">{value:.4g}</text>',
            ]
        )

    for index, row in enumerate(rows):
        value = float(row[metric])
        x = left + index * (chart_width / len(rows)) + gap / 2.0
        bar_height = chart_height * value / maximum
        y = top + chart_height - bar_height
        scheme = str(row["scheme"])
        label = f'{scheme}/{row["error_pattern"]}'
        elements.extend(
            [
                f'<rect x="{x:.1f}" y="{y:.1f}" width="{bar_width:.1f}" '
                f'height="{bar_height:.1f}" fill="{SCHEME_COLORS.get(scheme, "#666")}"/>',
                f'<text x="{x + bar_width / 2:.1f}" y="{top + chart_height + 14}" '
                f'text-anchor="end" transform="rotate(-55 {x + bar_width / 2:.1f} '
                f'{top + chart_height + 14})" font-family="sans-serif" font-size="11">'
                f'{html.escape(label)}</text>',
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
    rows = load_averages(args.csv_path)
    args.output_directory.mkdir(parents=True, exist_ok=True)
    charts = (
        ("detection_rate", "Mean detection rate", "Rejected / received", "detection_rate.svg"),
        ("elapsed_seconds", "Mean receiver execution time", "Seconds", "execution_time.svg"),
        ("max_rss_kb", "Mean receiver maximum resident set", "KiB", "memory_usage.svg"),
    )

    for metric, title, y_label, filename in charts:
        write_bar_chart(
            rows,
            metric,
            title,
            y_label,
            args.output_directory / filename,
        )


if __name__ == "__main__":
    main()
