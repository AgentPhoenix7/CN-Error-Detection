#!/usr/bin/env python3
"""Run reproducible mixed clean/error sender-receiver experiments."""

from __future__ import annotations

import argparse
import csv
import filecmp
import socket
import subprocess
from pathlib import Path


ALL_SCHEMES = ("checksum", "crc8", "crc10", "crc16", "crc32")
ALL_PATTERNS = ("none", "single", "two", "odd", "burst")
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


def available_port() -> int:
    """Ask the operating system for a currently unused loopback TCP port."""
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as probe:
        probe.bind(("127.0.0.1", 0))
        return int(probe.getsockname()[1])


def parse_metrics(
    output: str,
    prefix: str,
    integer_fields: tuple[str, ...],
    float_fields: tuple[str, ...],
) -> dict[str, int | float]:
    """Parse one machine-readable metrics line emitted by a C process."""
    metrics_line = next(
        (line for line in output.splitlines() if line.startswith(prefix + ",")),
        None,
    )

    if metrics_line is None:
        raise RuntimeError(f"missing {prefix} line in process output")

    values = metrics_line.split(",")[1:]
    expected_count = len(integer_fields) + len(float_fields)

    if len(values) != expected_count:
        raise RuntimeError(
            f"invalid {prefix} field count: expected {expected_count}, "
            f"received {len(values)}"
        )

    metrics: dict[str, int | float] = {}

    for name, value in zip(
        integer_fields,
        values[: len(integer_fields)],
        strict=True,
    ):
        metrics[name] = int(value)

    for name, value in zip(
        float_fields,
        values[len(integer_fields) :],
        strict=True,
    ):
        metrics[name] = float(value)

    return metrics


def portable_path(path: Path, root: Path) -> str:
    """Prefer a repository-relative path in durable CSV output."""
    try:
        return str(path.relative_to(root))
    except ValueError:
        return str(path)


def run_one(
    root: Path,
    input_path: Path,
    scheme: str,
    pattern: str,
    repeat: int,
    seed: int,
    error_interval: int,
    output_path: Path,
) -> dict[str, str | int | float]:
    """Run one process pair, validate counts, and return one CSV row."""
    port = available_port()
    receiver_command = [
        str(root / "build" / "receiver"),
        str(output_path),
        scheme,
        str(port),
    ]
    sender_command = [
        str(root / "build" / "sender"),
        str(input_path),
        scheme,
        pattern,
        "127.0.0.1",
        str(port),
        hex(seed),
        str(error_interval),
    ]

    receiver = subprocess.Popen(
        receiver_command,
        cwd=root,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
    )

    assert receiver.stdout is not None
    ready_line = receiver.stdout.readline()

    if not ready_line.startswith("Listening on port"):
        _, stderr = receiver.communicate(timeout=5)
        raise RuntimeError(
            f"receiver failed to start for {scheme}/{pattern}: {stderr}"
        )

    sender = subprocess.run(
        sender_command,
        cwd=root,
        capture_output=True,
        text=True,
        timeout=60,
        check=False,
    )
    receiver_stdout, receiver_stderr = receiver.communicate(timeout=60)

    if sender.returncode != 0 or receiver.returncode != 0:
        raise RuntimeError(
            f"experiment failed for {scheme}/{pattern}\n"
            f"sender: {sender.stderr}\nreceiver: {receiver_stderr}"
        )

    sender_metrics = parse_metrics(
        sender.stdout,
        "SENDER_METRICS",
        ("frames_sent", "frames_injected", "frames_clean"),
        (
            "sender_elapsed_seconds",
            "sender_user_cpu_seconds",
            "sender_system_cpu_seconds",
            "sender_max_rss_kb",
        ),
    )
    receiver_metrics = parse_metrics(
        receiver_stdout,
        "RECEIVER_METRICS",
        (
            "frames_received",
            "frames_accepted",
            "frames_rejected",
            "output_bytes",
        ),
        (
            "receiver_elapsed_seconds",
            "receiver_user_cpu_seconds",
            "receiver_system_cpu_seconds",
            "receiver_max_rss_kb",
        ),
    )

    frames_sent = int(sender_metrics["frames_sent"])
    frames_received = int(receiver_metrics["frames_received"])
    frames_injected = int(sender_metrics["frames_injected"])
    frames_clean = int(sender_metrics["frames_clean"])
    frames_accepted = int(receiver_metrics["frames_accepted"])
    frames_rejected = int(receiver_metrics["frames_rejected"])
    undetected_frames = frames_injected - frames_rejected

    if frames_sent != frames_received:
        raise RuntimeError(
            f"frame-count mismatch for {scheme}/{pattern}: "
            f"sent {frames_sent}, received {frames_received}"
        )

    if frames_sent != frames_injected + frames_clean:
        raise RuntimeError(f"invalid sender frame categories for {scheme}/{pattern}")

    if frames_received != frames_accepted + frames_rejected:
        raise RuntimeError(f"invalid receiver frame outcomes for {scheme}/{pattern}")

    if undetected_frames < 0 or frames_accepted != frames_clean + undetected_frames:
        raise RuntimeError(f"inconsistent error outcomes for {scheme}/{pattern}")

    if pattern == "none" and not filecmp.cmp(input_path, output_path, shallow=False):
        raise RuntimeError(f"lossless control mismatch for {scheme}")

    input_bytes = input_path.stat().st_size
    output_bytes = output_path.stat().st_size
    detection_rate = (
        frames_rejected / frames_injected if frames_injected > 0 else 0.0
    )
    undetected_rate = (
        undetected_frames / frames_injected if frames_injected > 0 else 0.0
    )
    total_cpu_seconds = sum(
        float(sender_metrics[name])
        for name in ("sender_user_cpu_seconds", "sender_system_cpu_seconds")
    ) + sum(
        float(receiver_metrics[name])
        for name in ("receiver_user_cpu_seconds", "receiver_system_cpu_seconds")
    )

    print(
        f"{scheme:8} {pattern:6} seed=0x{seed:08X} "
        f"clean={frames_clean} injected={frames_injected} "
        f"rejected={frames_rejected} undetected={undetected_frames}"
    )

    return {
        "input_file": portable_path(input_path, root),
        "output_file": portable_path(output_path, root),
        "input_bytes": input_bytes,
        "output_bytes": output_bytes,
        "scheme": scheme,
        "error_pattern": pattern,
        "repeat": repeat,
        "seed": f"0x{seed:08X}",
        "error_interval": error_interval if pattern != "none" else 0,
        "frames_sent": frames_sent,
        "frames_received": frames_received,
        "frames_injected": frames_injected,
        "frames_clean": frames_clean,
        "frames_accepted": frames_accepted,
        "frames_rejected": frames_rejected,
        "undetected_frames": undetected_frames,
        "detection_rate": f"{detection_rate:.9f}",
        "undetected_rate": f"{undetected_rate:.9f}",
        "sender_elapsed_seconds": f"{float(sender_metrics['sender_elapsed_seconds']):.9f}",
        "sender_user_cpu_seconds": f"{float(sender_metrics['sender_user_cpu_seconds']):.9f}",
        "sender_system_cpu_seconds": f"{float(sender_metrics['sender_system_cpu_seconds']):.9f}",
        "sender_max_rss_kb": int(sender_metrics["sender_max_rss_kb"]),
        "receiver_elapsed_seconds": f"{float(receiver_metrics['receiver_elapsed_seconds']):.9f}",
        "receiver_user_cpu_seconds": f"{float(receiver_metrics['receiver_user_cpu_seconds']):.9f}",
        "receiver_system_cpu_seconds": f"{float(receiver_metrics['receiver_system_cpu_seconds']):.9f}",
        "receiver_max_rss_kb": int(receiver_metrics["receiver_max_rss_kb"]),
        "total_cpu_seconds": f"{total_cpu_seconds:.9f}",
    }


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input", type=Path, help="input file sent by the sender")
    parser.add_argument(
        "--results",
        type=Path,
        default=None,
        help="CSV destination (default: C/results/experiments.csv)",
    )
    parser.add_argument(
        "--output-directory",
        type=Path,
        default=None,
        help="received-file directory (default: C/output_files)",
    )
    parser.add_argument(
        "--schemes",
        nargs="+",
        choices=ALL_SCHEMES,
        default=list(ALL_SCHEMES),
    )
    parser.add_argument(
        "--patterns",
        nargs="+",
        choices=ALL_PATTERNS,
        default=list(ALL_PATTERNS),
    )
    parser.add_argument("--repeats", type=int, default=3)
    parser.add_argument("--seed", type=lambda value: int(value, 0), default=0x12345678)
    parser.add_argument(
        "--error-interval",
        type=int,
        default=2,
        help=(
            "select each frame reproducibly with probability 1/N for error "
            "injection (default: 2)"
        ),
    )
    parser.add_argument(
        "--append",
        action="store_true",
        help="append instead of replacing the results CSV",
    )
    return parser.parse_args()


def open_results(path: Path, append: bool) -> tuple[object, csv.DictWriter]:
    """Open a results CSV, rejecting incompatible append schemas."""
    if append and path.exists() and path.stat().st_size > 0:
        with path.open(newline="", encoding="utf-8") as source:
            existing_fields = tuple(next(csv.reader(source)))

        if existing_fields != CSV_FIELDS:
            raise SystemExit(
                f"cannot append to CSV with an incompatible schema: {path}"
            )

    mode = "a" if append else "w"
    output = path.open(mode, newline="", encoding="utf-8")
    writer = csv.DictWriter(output, fieldnames=CSV_FIELDS, lineterminator="\n")

    if not append or path.stat().st_size == 0:
        writer.writeheader()

    return output, writer


def main() -> None:
    args = parse_args()
    root = Path(__file__).resolve().parents[1]
    input_path = args.input.resolve()
    results_path = (
        args.results.resolve()
        if args.results is not None
        else root / "results" / "experiments.csv"
    )
    output_directory = (
        args.output_directory.resolve()
        if args.output_directory is not None
        else root / "output_files"
    )

    if args.repeats < 1:
        raise SystemExit("--repeats must be positive")

    if args.error_interval < 1:
        raise SystemExit("--error-interval must be positive")

    if not input_path.is_file():
        raise SystemExit(f"input file does not exist: {input_path}")

    subprocess.run(["make", "all"], cwd=root, check=True)
    results_path.parent.mkdir(parents=True, exist_ok=True)
    output_directory.mkdir(parents=True, exist_ok=True)
    results_file, writer = open_results(results_path, args.append)

    try:
        for repeat in range(args.repeats):
            for scheme in args.schemes:
                for pattern in args.patterns:
                    suffix = input_path.suffix if input_path.suffix else ".bin"
                    output_path = output_directory / (
                        f"{input_path.stem}_{scheme}_{pattern}_received{suffix}"
                    )
                    writer.writerow(
                        run_one(
                            root,
                            input_path,
                            scheme,
                            pattern,
                            repeat + 1,
                            (args.seed + repeat) & 0xFFFF_FFFF,
                            args.error_interval,
                            output_path,
                        )
                    )
                    results_file.flush()
    finally:
        results_file.close()


if __name__ == "__main__":
    main()
