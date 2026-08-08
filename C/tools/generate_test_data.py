#!/usr/bin/env python3
"""Generate deterministic binary fixtures used by integration experiments."""

from pathlib import Path


def lcg_bytes(length: int, seed: int) -> bytes:
    """Return deterministic bytes using the assignment's fixed LCG."""
    output = bytearray()
    state = seed

    for _ in range(length):
        state = (state * 1_664_525 + 1_013_904_223) & 0xFFFF_FFFF
        output.append(state >> 24)

    return bytes(output)


def main() -> None:
    root = Path(__file__).resolve().parents[1]
    test_data = root / "test_data"
    test_data.mkdir(exist_ok=True)
    (test_data / "empty.bin").write_bytes(b"")
    (test_data / "small.txt").write_text(
        "Error detection over TCP preserves binary payloads.\n",
        encoding="utf-8",
    )
    (test_data / "exact_frame.bin").write_bytes(
        lcg_bytes(1_499, 0x0123_4567)
    )
    (test_data / "multi_frame.bin").write_bytes(
        lcg_bytes(4_096, 0x89AB_CDEF)
    )


if __name__ == "__main__":
    main()
