"""Tests for experiment-runner output behavior."""

from __future__ import annotations

import sys
import subprocess
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch


TOOLS_DIRECTORY = Path(__file__).resolve().parents[1] / "tools"
sys.path.insert(0, str(TOOLS_DIRECTORY))

from run_experiments import open_results, run_one  # noqa: E402
import run_experiments as runner  # noqa: E402
from experiment_results import load_rows  # noqa: E402
from test_experiment_results import experiment_row  # noqa: E402


class RunExperimentsTest(unittest.TestCase):
    # Creates isolated paths and a fixed input for each runner test. Registers
    # temporary-directory cleanup, stores the C project root and CSV path, and
    # selects the small text fixture.
    def setUp(self) -> None:
        directory = tempfile.TemporaryDirectory()
        self.addCleanup(directory.cleanup)
        self.directory = Path(directory.name)
        self.root = TOOLS_DIRECTORY.parent
        self.results = self.directory / "results.csv"
        self.input = self.root / "test_data/small.txt"

    # Builds a short experiment command for runner tests. Returns arguments for
    # one checksum/no-error trial with isolated results and receiver paths,
    # appending overrides supplied by the test.
    def arguments(self, *extra: str) -> list[str]:
        return [str(TOOLS_DIRECTORY / "run_experiments.py"), str(self.input),
                "--schemes", "checksum", "--patterns", "none", "--repeats", "1",
                "--results", str(self.results), "--output-directory", str(self.directory / "received"),
                *extra]

    # Seeds a results file and returns its original bytes for rollback checks.
    # Builds a valid synthetic row using the real fixture's size, applies
    # requested field changes, and writes the production header and LF line
    # endings.
    def write_existing(self, *, pattern: str = "none", **changes: str) -> bytes:
        row = experiment_row("checksum", pattern)
        row.update(input_file="test_data/small.txt", input_bytes=str(self.input.stat().st_size))
        if pattern == "none":
            row["output_bytes"] = row["input_bytes"]
        row.update(changes)
        import csv
        with self.results.open("w", newline="") as stream:
            writer = csv.DictWriter(stream, fieldnames=runner.CSV_FIELDS, lineterminator="\n")
            writer.writeheader()
            writer.writerow(row)
        return self.results.read_bytes()

    # Checks rollback when a replacement batch fails. Seeds a CSV, makes
    # run_one() raise during main(), and compares the destination bytes with
    # their original snapshot.
    def test_failed_replacement_preserves_existing_csv(self) -> None:
        before = self.write_existing()
        with patch.object(sys, "argv", self.arguments()), patch.object(runner, "run_one", side_effect=RuntimeError("transfer aborted")):
            with self.assertRaisesRegex(RuntimeError, "transfer aborted"):
                runner.main()
        self.assertEqual(self.results.read_bytes(), before)

    # Checks cleanup after a failed first batch. Injects a transfer failure
    # before any destination exists and requires that no results CSV is left
    # behind.
    def test_failed_first_run_does_not_leave_csv(self) -> None:
        with patch.object(sys, "argv", self.arguments()), patch.object(runner, "run_one", side_effect=RuntimeError("transfer aborted")):
            with self.assertRaisesRegex(RuntimeError, "transfer aborted"):
                runner.main()
        self.assertFalse(self.results.exists())

    # Checks rollback after partial progress within an appended batch. Supplies
    # one successful extra row followed by a transfer exception and requires
    # the existing CSV to remain byte-for-byte unchanged.
    def test_failed_append_preserves_existing_csv(self) -> None:
        before = self.write_existing()
        row = experiment_row("checksum", "none", repeat=2)
        row.update(input_file="test_data/small.txt", input_bytes=str(self.input.stat().st_size),
                   output_bytes=str(self.input.stat().st_size), seed="0x12345679")
        with patch.object(sys, "argv", self.arguments("--append", "--repeats", "2")), patch.object(runner, "run_one", side_effect=[row, RuntimeError("transfer aborted")]):
            with self.assertRaisesRegex(RuntimeError, "transfer aborted"):
                runner.main()
        self.assertEqual(self.results.read_bytes(), before)

    # Checks append identities through real subprocess runs. Runs the same one-
    # trial command twice with append on the second run and checks repeats
    # one/two and successive base-seed values.
    def test_append_continues_repeat_and_seed(self) -> None:
        for extra in ([], ["--append"]):
            result = subprocess.run([sys.executable, *self.arguments(*extra)], capture_output=True, text=True, timeout=10)
            self.assertEqual(result.returncode, 0, result.stderr)
        rows = load_rows(self.results)
        self.assertEqual([int(row["repeat"]) for row in rows], [1, 2])
        self.assertEqual([row["seed"] for row in rows], ["0x12345678", "0x12345679"])

    # Checks append preflight against changes in existing group metadata. Seeds
    # rows with a different input path, byte count, or denominator, invokes the
    # CLI, and requires a nonzero exit with the original file intact.
    def test_append_rejects_incompatible_metadata(self) -> None:
        for changes in ({"input_file": "other.bin"}, {"input_bytes": "999"}, {"error_interval": "3"}):
            with self.subTest(changes=changes):
                before = self.write_existing(pattern="single", **changes)
                result = subprocess.run([sys.executable, *self.arguments("--append", "--patterns", "single")], capture_output=True, text=True, timeout=10)
                self.assertNotEqual(result.returncode, 0)
                self.assertEqual(self.results.read_bytes(), before)

    # Checks uint32_t denominator validation before any transfer or result
    # write. Supplies 2^32, installs a transfer failure sentinel, and requires
    # a SystemExit range error with the previous CSV unchanged.
    def test_invalid_interval_is_rejected_before_results_open(self) -> None:
        before = self.write_existing()
        with patch.object(sys, "argv", self.arguments("--error-interval", "4294967296")), patch.object(runner, "run_one", side_effect=RuntimeError("reached transfer")):
            with self.assertRaisesRegex(SystemExit, "uint32"):
                runner.main()
        self.assertEqual(self.results.read_bytes(), before)

    # Checks that a receiver cannot overwrite the durable CSV directly. Sets
    # the CSV path equal to the generated receiver filename, runs the CLI, and
    # requires rejection with the existing bytes preserved.
    def test_rejects_results_receiver_output_collision(self) -> None:
        self.results = self.directory / "small_checksum_none_received.txt"
        before = self.write_existing()
        result = subprocess.run(
            [sys.executable, *self.arguments("--output-directory", str(self.directory))],
            capture_output=True, text=True, timeout=10,
        )
        self.assertNotEqual(result.returncode, 0)
        self.assertEqual(self.results.read_bytes(), before)

    # Checks receiver cleanup when the sender fails before connecting. Starts
    # real subprocesses with a missing input, records the receiver through a
    # Popen wrapper, bounds waits, and asserts that the child has exited before
    # final cleanup.
    def test_sender_failure_stops_receiver(self) -> None:
        receivers = []
        real_popen = subprocess.Popen

        # Records real receiver children during the lifecycle regression.
        # Delegates to the original Popen, retains receiver process objects,
        # and replaces only their communicate method with a bounded-wait
        # wrapper.
        def record_process(*args, **kwargs):
            process = real_popen(*args, **kwargs)
            if Path(args[0][0]).name == "receiver":
                receivers.append(process)
                communicate = process.communicate
                # Bound the old implementation's failure while exercising real processes.
                # Keeps the lifecycle regression's receiver wait short. Clamps
                # the requested communicate timeout to one second and delegates
                # all other arguments to the real process method.
                def bounded_communicate(*arguments, **options):
                    options["timeout"] = min(options.get("timeout", 1), 1)
                    return communicate(*arguments, **options)
                process.communicate = bounded_communicate
            return process

        try:
            with patch.object(subprocess, "Popen", side_effect=record_process):
                try:
                    run_one(self.root, self.directory / "missing.bin", "checksum", "none", 1, 1, 2, self.directory / "received.bin")
                except (RuntimeError, subprocess.TimeoutExpired):
                    pass
            self.assertEqual(len(receivers), 1)
            self.assertIsNotNone(receivers[0].poll(), "receiver survived a failed sender")
        finally:
            for process in receivers:
                if process.poll() is None:
                    process.kill()
                    process.communicate(timeout=1)

    # Checks that silent startup cannot block the runner indefinitely. Starts a
    # sleeping Python child with piped output, expects a short
    # wait_for_receiver() timeout, and always terminates and reaps the child.
    def test_receiver_readiness_has_a_timeout(self) -> None:
        with subprocess.Popen([sys.executable, "-c", "import time; time.sleep(10)"], stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True) as process:
            try:
                with self.assertRaises(subprocess.TimeoutExpired):
                    runner.wait_for_receiver(process, timeout=0.05)
            finally:
                process.terminate()
                process.communicate(timeout=1)

    # Checks that both inference inputs belong to one result transaction. Runs
    # the Make campaign with a valid detection fixture and missing control
    # input, then requires failure and byte-for-byte preservation of the
    # previous CSV.
    def test_failed_control_phase_preserves_previous_campaign(self) -> None:
        before = self.write_existing()
        result = subprocess.run(
            ["make", "experiments_inference", "DETECTION_INPUT=test_data/multi_frame.bin",
             "DETECTION_REPEATS=1", "PERFORMANCE_REPEATS=1",
             f"PERFORMANCE_INPUT={self.directory / 'missing.bin'}",
             f"INFERENCE_RESULTS={self.results}", f"INFERENCE_OUTPUT_DIRECTORY={self.directory / 'campaign-output'}"],
            cwd=self.root, capture_output=True, text=True, timeout=30,
        )
        self.assertNotEqual(result.returncode, 0)
        self.assertEqual(self.results.read_bytes(), before)

    # Checks that C process RSS does not inherit the Python parent's peak. Runs
    # a control trial before and after allocating 32 MiB in the parent and
    # requires positive sender/receiver peaks without a comparable increase.
    @unittest.skipUnless(sys.platform == "linux", "Linux process memory regression")
    def test_peak_rss_excludes_parent_allocation(self) -> None:
        root = TOOLS_DIRECTORY.parent
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / "received.bin"
            baseline = run_one(root, root / "test_data/small.txt", "checksum", "none", 1, 1, 2, output)
            allocation = bytearray(32 * 1024 * 1024)
            inflated = run_one(root, root / "test_data/small.txt", "checksum", "none", 1, 1, 2, output)
            self.assertEqual(allocation[0], 0)
            for process in ("sender", "receiver"):
                field = f"{process}_max_rss_kb"
                self.assertGreater(inflated[field], 0)
                self.assertLess(inflated[field], baseline[field] + 8192)

    # Checks compiler-generated header dependency tracking. Asks Make for dry
    # runs treating selected headers as changed and requires every expected
    # consuming source to appear in the rebuild commands.
    def test_changed_headers_rebuild_consumers(self) -> None:
        root = TOOLS_DIRECTORY.parent
        for header, sources in (
            ("config.h", ("sender.c", "receiver.c", "frame.c")),
            ("crc.h", ("crc.c", "frame.c")),
            ("error_injection.h", ("sender.c", "error_injection.c")),
        ):
            with self.subTest(header=header):
                result = subprocess.run(
                    ["make", "-n", "-W", f"include/{header}", "build/sender", "build/receiver"],
                    cwd=root, capture_output=True, text=True, check=True,
                )
                for source in sources:
                    self.assertIn(f"src/{source}", result.stdout)

    # Checks portable line endings in transactional CSV output. Writes a
    # complete valid row through open_results(), commits it, and checks that
    # the file contains no CRLF sequences.
    def test_results_csv_uses_lf_line_endings(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "results.csv"
            with open_results(path, append=False) as (_, writer):
                writer.writerow(experiment_row("checksum", "none"))

            self.assertNotIn(b"\r\n", path.read_bytes())


if __name__ == "__main__":
    unittest.main()
