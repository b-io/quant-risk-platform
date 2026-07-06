"""Generate a Python line-coverage metric using only the standard library."""

from __future__ import annotations

import argparse
import ast
import json
import sys
import trace
import unittest
from dataclasses import asdict, dataclass
from datetime import datetime, timezone
from pathlib import Path


@dataclass(frozen=True)
class PythonCoverageMetric:
    """Serializable Python test-coverage metric emitted by the test scripts."""

    metric_name: str
    generated_at_utc: str
    line_coverage_percent: float
    lines_covered: int
    lines_valid: int
    threshold_percent: float
    status: str
    test_status: str
    tests_run: int
    source_roots: list[str]
    tests_root: str


def executable_lines(path: Path) -> set[int]:
    """Returns statement line numbers that can be observed by the trace module."""

    tree = ast.parse(path.read_text(encoding="utf-8"), filename=str(path))
    lines: set[int] = set()
    for node in ast.walk(tree):
        if not isinstance(node, ast.stmt):
            continue
        if isinstance(node, ast.Expr) and isinstance(getattr(node, "value", None), ast.Constant):
            if isinstance(node.value.value, str):
                continue
        lines.add(node.lineno)
    return lines


def source_files(source_roots: list[Path]) -> list[Path]:
    """Finds Python source files under the configured source roots."""

    files: list[Path] = []
    for root in source_roots:
        if root.is_file() and root.suffix == ".py":
            files.append(root)
            continue
        if not root.exists():
            continue
        files.extend(path for path in root.rglob("*.py") if "__pycache__" not in path.parts)
    return sorted({path.resolve() for path in files})


def run_unittest_suite(tests_root: Path, pattern: str) -> unittest.result.TestResult | None:
    """Discovers and runs Python unit tests from the requested directory."""

    if not tests_root.exists():
        return None
    suite = unittest.defaultTestLoader.discover(str(tests_root), pattern=pattern)
    if suite.countTestCases() == 0:
        return None
    runner = unittest.TextTestRunner(stream=sys.stdout, verbosity=2)
    return runner.run(suite)


def parse_args() -> argparse.Namespace:
    """Parses command-line options for the Python coverage metric runner."""

    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--fail-under-threshold",
        action="store_true",
        help="Return a non-zero exit code when line coverage is below threshold.",
    )
    parser.add_argument("--json", required=True, type=Path, help="Path for the generated JSON metric.")
    parser.add_argument("--markdown", type=Path, help="Optional path for a Markdown summary.")
    parser.add_argument("--pattern", default="test_*.py", help="unittest discovery pattern.")
    parser.add_argument("--project-root", required=True, type=Path, help="Repository root to add to sys.path.")
    parser.add_argument(
        "--source",
        action="append",
        required=True,
        type=Path,
        help="Python source root to include in the coverage score. Repeatable.",
    )
    parser.add_argument("--tests", required=True, type=Path, help="Python unittest discovery root.")
    parser.add_argument("--threshold", default=0.0, type=float, help="Minimum required line coverage percentage.")
    return parser.parse_args()


def write_json(metric: PythonCoverageMetric, output_path: Path) -> None:
    """Writes the coverage metric as formatted JSON."""

    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text(json.dumps(asdict(metric), indent=2, sort_keys=True) + "\n", encoding="utf-8")


def write_markdown(metric: PythonCoverageMetric, output_path: Path) -> None:
    """Writes the coverage metric as a compact Markdown summary."""

    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text(
        "\n".join(
            [
                "# Python Test Coverage Metric",
                "",
                f"- Status: `{metric.status}`",
                f"- Test status: `{metric.test_status}`",
                f"- Tests run: `{metric.tests_run}`",
                f"- Line coverage: `{metric.line_coverage_percent:.2f}%` ({metric.lines_covered}/{metric.lines_valid})",
                f"- Threshold: `{metric.threshold_percent:.2f}%`",
                f"- Generated UTC: `{metric.generated_at_utc}`",
                f"- Source roots: `{', '.join(metric.source_roots)}`",
                f"- Tests root: `{metric.tests_root}`",
                "",
            ]
        ),
        encoding="utf-8",
    )


def main() -> int:
    """Runs Python tests under trace and emits a stable line-coverage score."""

    args = parse_args()
    project_root = args.project_root.resolve()
    source_roots = [path.resolve() for path in args.source]
    tests_root = args.tests.resolve()

    sys.path.insert(0, str(project_root))
    sys.path.insert(0, str(project_root / "python"))

    files = source_files(source_roots)
    executable_by_file = {path: executable_lines(path) for path in files}
    total_lines = sum(len(lines) for lines in executable_by_file.values())

    tracer = trace.Trace(count=True, trace=False, ignoredirs=[sys.prefix, sys.exec_prefix])
    test_result = tracer.runfunc(run_unittest_suite, tests_root, args.pattern)
    counts = tracer.results().counts

    covered_by_file: dict[Path, set[int]] = {path: set() for path in executable_by_file}
    for (filename, line_number), count in counts.items():
        if count <= 0:
            continue
        path = Path(filename).resolve()
        if path in covered_by_file:
            covered_by_file[path].add(line_number)

    covered_lines = sum(len(executable_by_file[path] & covered_by_file[path]) for path in executable_by_file)
    coverage_percent = 0.0 if total_lines == 0 else (covered_lines / total_lines) * 100.0

    if test_result is None:
        test_status = "no_tests"
    else:
        test_status = "pass" if test_result.wasSuccessful() else "fail"

    if total_lines == 0:
        status = "no_source"
    elif test_status == "fail":
        status = "fail"
    elif test_status == "no_tests":
        status = "no_tests"
    else:
        status = "pass" if coverage_percent >= args.threshold else "fail"

    metric = PythonCoverageMetric(
        metric_name="python_test_line_coverage",
        generated_at_utc=datetime.now(timezone.utc).replace(microsecond=0).isoformat(),
        line_coverage_percent=round(coverage_percent, 2),
        lines_covered=covered_lines,
        lines_valid=total_lines,
        threshold_percent=args.threshold,
        status=status,
        test_status=test_status,
        tests_run=0 if test_result is None else test_result.testsRun,
        source_roots=[str(path) for path in source_roots],
        tests_root=str(tests_root),
    )

    write_json(metric, args.json)
    if args.markdown is not None:
        write_markdown(metric, args.markdown)

    print(
        "Python coverage metric: "
        f"{metric.line_coverage_percent:.2f}% line coverage "
        f"({metric.status}, tests {metric.tests_run}, threshold {metric.threshold_percent:.2f}%)"
    )
    if test_status == "fail":
        return 1
    if args.fail_under_threshold and metric.status == "fail":
        return 2
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
