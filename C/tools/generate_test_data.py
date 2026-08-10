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


def write_lcg_file(path: Path, length: int, seed: int) -> None:
    """Write a large deterministic fixture without keeping it all in memory."""
    chunk_size = 65_536
    state = seed
    remaining = length

    with path.open("wb") as output:
        while remaining > 0:
            chunk_length = min(chunk_size, remaining)
            chunk = bytearray()

            for _ in range(chunk_length):
                state = (state * 1_664_525 + 1_013_904_223) & 0xFFFF_FFFF
                chunk.append(state >> 24)

            output.write(chunk)
            remaining -= chunk_length


def write_text_file(path: Path, length: int, label: str) -> None:
    """Write an exact-size, deterministic, human-readable experiment input."""
    paragraph = (
        f"{label}: checksum and CRC experiments transfer this text from the "
        "sender to the receiver. Some frames remain clean, while selected "
        "frames receive reproducible bit errors after their FCS is computed.\n"
    ).encode("ascii")
    remaining = length

    with path.open("wb") as output:
        while remaining > 0:
            chunk = paragraph[:remaining]
            output.write(chunk)
            remaining -= len(chunk)


def main() -> None:
    root = Path(__file__).resolve().parents[1]
    test_data = root / "test_data"
    input_files = root / "input_files"
    output_files = root / "output_files"
    test_data.mkdir(exist_ok=True)
    input_files.mkdir(exist_ok=True)
    output_files.mkdir(exist_ok=True)
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
    write_lcg_file(
        test_data / "detection_1m.bin",
        1_048_576,
        0x1357_9BDF,
    )
    write_lcg_file(
        test_data / "performance_8m.bin",
        8_388_608,
        0x2468_ACE0,
    )
    write_text_file(
        input_files / "detection_1m.txt",
        1_048_576,
        "One-mebibyte detection input",
    )
    write_text_file(
        input_files / "performance_8m.txt",
        8_388_608,
        "Eight-mebibyte performance input",
    )


if __name__ == "__main__":
    main()
