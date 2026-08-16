#!/usr/bin/env python3
"""Create interpretation-focused SVG summaries from experiment CSV results."""

from __future__ import annotations

import argparse
import html
import math
from pathlib import Path

from experiment_results import PATTERNS, SCHEMES, load_summaries, summary_for


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
                values.append(100.0 * float(summary["undetected_rate"]))

    maximum = max(values, default=0.0)
    injected_summary = next(
        (
            summary
            for summary in summaries
            if summary["error_pattern"] != "none"
        ),
        None,
    )
    context = "Injected-error frames only."

    if injected_summary is not None:
        context = (
            f'{float(injected_summary["input_bytes"]) / 1_048_576.0:.2f} MiB input; '
            f'one in every {int(injected_summary["error_interval"])} frames selected; '
            f'{int(injected_summary["runs"])} repeats per pattern.'
        )

    elements = svg_document("Mean undetected-frame rate", width, height)
    elements.append(
        '<text x="500" y="62" text-anchor="middle" font-family="sans-serif" '
        f'font-size="14">{html.escape(context)}</text>'
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
            value = (
                0.0
                if summary is None
                else 100.0 * float(summary["undetected_rate"])
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


def write_frame_outcomes_chart(
    summaries: list[dict[str, str | float | int]], output_path: Path
) -> None:
    """Write full composition plus a focused view of rare undetected errors."""
    width = 1200
    height = 600
    left = 90
    top = 165
    chart_width = 580
    chart_height = 315
    colors = {
        "clean": "#4C78A8",
        "detected": "#F2A541",
        "undetected": "#D65F8D",
    }
    injected_summary = next(
        (
            summary
            for summary in summaries
            if summary["error_pattern"] != "none"
        ),
        None,
    )
    context = "Injected-pattern runs; all error patterns and repeats combined."

    if injected_summary is not None:
        context = (
            f'{float(injected_summary["input_bytes"]) / 1_048_576.0:.2f} MiB input; '
            f'one in every {int(injected_summary["error_interval"])} frames selected; '
            f'{int(injected_summary["runs"])} repeats per pattern.'
        )

    outcomes: list[tuple[str, int, int, int]] = []

    for scheme in SCHEMES:
        scheme_rows = [
            summary
            for summary in summaries
            if summary["scheme"] == scheme and summary["error_pattern"] != "none"
        ]
        outcomes.append(
            (
                scheme,
                sum(int(row["frames_clean"]) for row in scheme_rows),
                sum(int(row["frames_rejected"]) for row in scheme_rows),
                sum(int(row["undetected_frames"]) for row in scheme_rows),
            )
        )

    elements = svg_document("Frame outcomes in mixed-error trials", width, height)
    elements.append(
        '<text x="600" y="62" text-anchor="middle" font-family="sans-serif" '
        f'font-size="14">{html.escape(context)}</text>'
    )
    elements.extend(
        [
            '<text x="380" y="99" text-anchor="middle" font-family="sans-serif" '
            'font-size="17" font-weight="bold">All transmitted frames</text>',
            '<text x="955" y="99" text-anchor="middle" font-family="sans-serif" '
            'font-size="17" font-weight="bold">Undetected among injected frames</text>',
            '<text x="955" y="123" text-anchor="middle" font-family="sans-serif" '
            'font-size="13">Focused scale; exact counts and rates shown</text>',
            '<rect x="165" y="117" width="14" height="14" fill="#4C78A8"/>',
            '<text x="186" y="129" font-family="sans-serif" font-size="13">Clean</text>',
            '<rect x="285" y="117" width="14" height="14" fill="#F2A541"/>',
            '<text x="306" y="129" font-family="sans-serif" font-size="13">Error detected</text>',
            '<rect x="455" y="117" width="14" height="14" fill="#D65F8D"/>',
            '<text x="476" y="129" font-family="sans-serif" font-size="13">Error undetected</text>',
            '<line x1="710" y1="88" x2="710" y2="520" stroke="#D0D0D0"/>',
            f'<line x1="{left}" y1="{top}" x2="{left}" y2="{top + chart_height}" stroke="#222"/>',
            f'<line x1="{left}" y1="{top + chart_height}" x2="{left + chart_width}" '
            f'y2="{top + chart_height}" stroke="#222"/>',
        ]
    )

    for tick in range(6):
        value = tick * 20
        y = top + chart_height - chart_height * value / 100.0
        elements.extend(
            [
                f'<line x1="{left}" y1="{y:.1f}" x2="{left + chart_width}" '
                f'y2="{y:.1f}" stroke="#DDD"/>',
                f'<text x="{left - 8}" y="{y + 5:.1f}" text-anchor="end" '
                f'font-family="sans-serif" font-size="12">{value}%</text>',
            ]
        )

    bar_space = chart_width / len(outcomes)
    bar_width = bar_space * 0.54

    for index, (scheme, clean, detected, undetected) in enumerate(outcomes):
        total = clean + detected + undetected
        portions = (
            ("clean", clean),
            ("detected", detected),
            ("undetected", undetected),
        )
        x = left + index * bar_space + (bar_space - bar_width) / 2
        current_y = top + chart_height

        for category, count in portions:
            percentage = 0.0 if total == 0 else 100.0 * count / total
            segment_height = chart_height * percentage / 100.0
            current_y -= segment_height
            elements.append(
                f'<rect x="{x:.1f}" y="{current_y:.1f}" width="{bar_width:.1f}" '
                f'height="{segment_height:.1f}" fill="{colors[category]}" stroke="white"/>'
            )

            if segment_height >= 18.0:
                elements.append(
                    f'<text x="{x + bar_width / 2:.1f}" '
                    f'y="{current_y + segment_height / 2 + 5:.1f}" '
                    'text-anchor="middle" font-family="sans-serif" font-size="12" '
                    f'fill="white">{percentage:.1f}%</text>'
                )

        elements.append(
            f'<text x="{x + bar_width / 2:.1f}" y="{top + chart_height + 28}" '
            'text-anchor="middle" font-family="sans-serif" font-size="14">'
            f'{html.escape(scheme)}</text>'
        )
        elements.append(
            f'<text x="{x + bar_width / 2:.1f}" y="{top - 10}" '
            'text-anchor="middle" font-family="sans-serif" font-size="12" '
            f'fill="#9E365F">{undetected:,} missed</text>'
        )

    focused_rates = [
        100.0 * undetected / (detected + undetected)
        if detected + undetected > 0
        else 0.0
        for _, _, detected, undetected in outcomes
    ]
    focused_maximum = max(focused_rates, default=0.0)

    if focused_maximum == 0.0:
        focused_maximum = 0.1
    else:
        rounded_maximum = math.ceil(focused_maximum * 10.0) / 10.0
        focused_maximum = (
            rounded_maximum + 0.1
            if rounded_maximum <= focused_maximum
            else rounded_maximum
        )

    focused_left = 805
    focused_width = 315
    focused_top = 165
    focused_spacing = 58

    for tick in range(3):
        value = focused_maximum * tick / 2.0
        x = focused_left + focused_width * tick / 2.0
        elements.extend(
            [
                f'<line x1="{x:.1f}" y1="{focused_top - 10}" x2="{x:.1f}" '
                'y2="470" stroke="#E0E0E0"/>',
                f'<text x="{x:.1f}" y="491" text-anchor="middle" '
                f'font-family="sans-serif" font-size="12">{value:.2f}%</text>',
            ]
        )

    elements.append(
        f'<line x1="{focused_left}" y1="470" '
        f'x2="{focused_left + focused_width}" y2="470" stroke="#222"/>'
    )

    for index, ((scheme, _, detected, undetected), rate) in enumerate(
        zip(outcomes, focused_rates, strict=True)
    ):
        injected = detected + undetected
        y = focused_top + index * focused_spacing
        bar_width_focused = focused_width * rate / focused_maximum
        elements.extend(
            [
                f'<text x="{focused_left - 12}" y="{y + 16}" text-anchor="end" '
                f'font-family="sans-serif" font-size="14">{html.escape(scheme)}</text>',
                f'<rect x="{focused_left}" y="{y}" width="{bar_width_focused:.1f}" '
                'height="19" fill="#D65F8D" stroke="#9E365F"/>',
                f'<text x="{focused_left + focused_width}" y="{y + 40}" '
                'text-anchor="end" font-family="monospace" font-size="12" '
                f'fill="#4A2635">{undetected:,} / {injected:,} ({rate:.3f}%)</text>',
            ]
        )

        if rate == 0.0:
            elements.append(
                f'<circle cx="{focused_left}" cy="{y + 9.5}" r="4" '
                'fill="white" stroke="#9E365F" stroke-width="2"/>'
            )

    elements.extend(
        [
            '<text x="600" y="555" text-anchor="middle" font-family="sans-serif" '
            'font-size="13">Left: share of all transmitted frames. Right: undetected errors divided by injected frames; focused scale starts at zero.</text>',
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
    input_mib = (
        float(rows[0]["input_bytes"]) / 1_048_576.0
        if rows
        else 0.0
    )
    run_count = int(rows[0]["runs"]) if rows else 0
    elements = svg_document("Receiver time for no-error controls", width, height)
    elements.append(
        '<text x="450" y="62" text-anchor="middle" font-family="sans-serif" '
        f'font-size="14">{input_mib:.2f} MiB input; mean of {run_count} runs; whiskers show one standard deviation.</text>'
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
    """Write sender and receiver CPU and peak-memory control summaries."""
    rows = []
    for scheme in SCHEMES:
        summary = summary_for(summaries, scheme, "none")
        if summary is not None:
            rows.append(summary)

    width = 980
    height = 430
    input_mib = (
        float(rows[0]["input_bytes"]) / 1_048_576.0
        if rows
        else 0.0
    )
    run_count = int(rows[0]["runs"]) if rows else 0
    elements = svg_document("Process resource usage for no-error controls", width, height)
    elements.append(
        '<text x="490" y="65" text-anchor="middle" font-family="sans-serif" '
        f'font-size="14">{input_mib:.2f} MiB input; mean of {run_count} runs; CPU is user plus system time.</text>'
    )
    elements.extend(
        [
            '<rect x="70" y="100" width="840" height="42" fill="#E8EEF7" stroke="#888"/>',
            '<text x="135" y="127" text-anchor="middle" font-family="sans-serif" font-size="14">Scheme</text>',
            '<text x="285" y="127" text-anchor="middle" font-family="sans-serif" font-size="14">Sender CPU (ms)</text>',
            '<text x="455" y="127" text-anchor="middle" font-family="sans-serif" font-size="14">Receiver CPU (ms)</text>',
            '<text x="640" y="127" text-anchor="middle" font-family="sans-serif" font-size="14">Sender peak RSS (KiB)</text>',
            '<text x="825" y="127" text-anchor="middle" font-family="sans-serif" font-size="14">Receiver peak RSS (KiB)</text>',
        ]
    )
    for index, row in enumerate(rows):
        y = 142 + index * 42
        elements.extend(
            [
                f'<rect x="70" y="{y}" width="840" height="42" fill="white" stroke="#B8B8B8"/>',
                f'<text x="135" y="{y + 27}" text-anchor="middle" font-family="sans-serif" '
                f'font-size="14">{html.escape(str(row["scheme"]))}</text>',
                f'<text x="285" y="{y + 27}" text-anchor="middle" font-family="monospace" '
                f'font-size="14">{1000.0 * float(row["sender_cpu_seconds"]):.3f}</text>',
                f'<text x="455" y="{y + 27}" text-anchor="middle" font-family="monospace" '
                f'font-size="14">{1000.0 * float(row["receiver_cpu_seconds"]):.3f}</text>',
                f'<text x="640" y="{y + 27}" text-anchor="middle" font-family="monospace" '
                f'font-size="14">{float(row["sender_max_rss_kb"]):.0f}</text>',
                f'<text x="825" y="{y + 27}" text-anchor="middle" font-family="monospace" '
                f'font-size="14">{float(row["receiver_max_rss_kb"]):.0f}</text>',
            ]
        )
    elements.append(
        '<text x="490" y="389" text-anchor="middle" font-family="sans-serif" '
        'font-size="13">RSS values are per-process maxima and must not be added as a measured simultaneous peak.</text>'
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
    write_frame_outcomes_chart(summaries, args.output_directory / "frame_outcomes.svg")
    write_control_timing_chart(summaries, args.output_directory / "execution_time.svg")
    write_resource_summary(summaries, args.output_directory / "resource_usage.svg")


if __name__ == "__main__":
    main()
