"""Tests for experiment-runner output behavior."""

from __future__ import annotations

import sys
import tempfile
import unittest
from pathlib import Path


TOOLS_DIRECTORY = Path(__file__).resolve().parents[1] / "tools"
sys.path.insert(0, str(TOOLS_DIRECTORY))

from run_experiments import open_results  # noqa: E402


class RunExperimentsTest(unittest.TestCase):
    def test_results_csv_uses_lf_line_endings(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "results.csv"
            output, writer = open_results(path, append=False)
            writer.writerow({"scheme": "checksum"})
            output.close()

            self.assertNotIn(b"\r\n", path.read_bytes())


if __name__ == "__main__":
    unittest.main()
