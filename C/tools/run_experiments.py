#!/usr/bin/env python3
"""Run reproducible sender/receiver experiments for every selected scheme."""

from __future__ import annotations

import argparse
import socket
import subprocess
import tempfile
from pathlib import Path


ALL_SCHEMES = ("checksum", "crc8", "crc10", "crc16", "crc32")
ALL_PATTERNS = ("none", "single", "two", "odd", "burst")


def available_port() -> int:
    """Ask the operating system for a currently unused loopback TCP port."""
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as probe:
        probe.bind(("127.0.0.1", 0))
        return int(probe.getsockname()[1])


def run_one(
    root: Path,
    input_path: Path,
    results_path: Path,
    scheme: str,
    pattern: str,
    seed: int,
    output_path: Path,
) -> None:
    """Run one receiver and sender pair and validate lossless control runs."""
    port = available_port()
    receiver_command = [
        str(root / "build" / "receiver"),
        str(output_path),
        scheme,
        str(port),
        str(results_path),
        pattern,
    ]
    sender_command = [
        str(root / "build" / "sender"),
        str(input_path),
        scheme,
        pattern,
        "127.0.0.1",
        str(port),
        hex(seed),
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
        timeout=15,
        check=False,
    )
    receiver_stdout, receiver_stderr = receiver.communicate(timeout=15)

    if sender.returncode != 0 or receiver.returncode != 0:
        raise RuntimeError(
            f"experiment failed for {scheme}/{pattern}\n"
            f"sender: {sender.stderr}\nreceiver: {receiver_stderr}"
        )

    if pattern == "none" and output_path.read_bytes() != input_path.read_bytes():
        raise RuntimeError(f"lossless control mismatch for {scheme}")

    print(
        f"{scheme:8} {pattern:6} seed=0x{seed:08X} "
        f"{sender.stdout.strip()} {receiver_stdout.strip()}"
    )


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input", type=Path, help="binary input file")
    parser.add_argument(
        "--results",
        type=Path,
        default=None,
        help="CSV destination (default: C/results/experiments.csv)",
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
        "--append",
        action="store_true",
        help="append instead of replacing the results CSV",
    )
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    root = Path(__file__).resolve().parents[1]
    input_path = args.input.resolve()
    results_path = (
        args.results.resolve()
        if args.results is not None
        else root / "results" / "experiments.csv"
    )

    if args.repeats < 1:
        raise SystemExit("--repeats must be positive")

    if not input_path.is_file():
        raise SystemExit(f"input file does not exist: {input_path}")

    subprocess.run(["make", "all"], cwd=root, check=True)
    results_path.parent.mkdir(parents=True, exist_ok=True)

    if not args.append:
        results_path.write_text("", encoding="utf-8")

    with tempfile.TemporaryDirectory(prefix="error-detection-") as temp_dir:
        output_path = Path(temp_dir) / "received.bin"

        for repeat in range(args.repeats):
            for scheme in args.schemes:
                for pattern in args.patterns:
                    run_one(
                        root,
                        input_path,
                        results_path,
                        scheme,
                        pattern,
                        (args.seed + repeat) & 0xFFFF_FFFF,
                        output_path,
                    )


if __name__ == "__main__":
    main()
