#!/usr/bin/env python3
"""Validate the deterministic three-frame mixed end-to-end result."""

from __future__ import annotations

import csv
import sys
from pathlib import Path


SCHEMES = {"checksum", "crc8", "crc10", "crc16", "crc32"}


# Checks the seeded three-frame mixed-transfer smoke results. Loads one CSV,
# requires every scheme exactly once, checks the pinned
# sent/received/injected/clean counts, and verifies all outcome-accounting
# identities.
def main() -> None:
    if len(sys.argv) != 2:
        raise SystemExit("usage: validate_mixed_csv.py CSV_PATH")

    csv_path = Path(sys.argv[1])
    with csv_path.open(newline="", encoding="utf-8") as source:
        rows = list(csv.DictReader(source))

    if len(rows) != len(SCHEMES):
        raise SystemExit(f"expected {len(SCHEMES)} rows, received {len(rows)}")

    if {row["scheme"] for row in rows} != SCHEMES:
        raise SystemExit("mixed result does not contain every required scheme")

    for row in rows:
        scheme = row["scheme"]
        sent = int(row["frames_sent"])
        received = int(row["frames_received"])
        injected = int(row["frames_injected"])
        clean = int(row["frames_clean"])
        accepted = int(row["frames_accepted"])
        rejected = int(row["frames_rejected"])
        undetected = int(row["undetected_frames"])

        if (sent, received, injected, clean) != (3, 3, 1, 2):
            raise SystemExit(
                f"{scheme}: expected sent/received/injected/clean 3/3/1/2, "
                f"received {sent}/{received}/{injected}/{clean}"
            )

        if sent != injected + clean:
            raise SystemExit(f"{scheme}: invalid sender categories")

        if received != accepted + rejected:
            raise SystemExit(f"{scheme}: invalid receiver categories")

        if undetected != injected - rejected:
            raise SystemExit(f"{scheme}: invalid undetected count")

        if accepted != clean + undetected:
            raise SystemExit(f"{scheme}: invalid accepted-frame breakdown")

    print("PASS: seeded mixed-frame CSV invariants")


if __name__ == "__main__":
    main()
