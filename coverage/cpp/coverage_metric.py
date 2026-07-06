"""Generate a stable test-coverage metric from Cobertura XML."""

from __future__ import annotations

import argparse
import json
import xml.etree.ElementTree as ET
from dataclasses import asdict, dataclass
from datetime import datetime, timezone
from pathlib import Path


@dataclass(frozen=True)
class CoverageMetric:
    """Serializable test-coverage metric emitted by the coverage target."""

    metric_name: str
    generated_at_utc: str
    line_coverage_percent: float
    lines_covered: int
    lines_valid: int
    branch_coverage_percent: float | None
    branches_covered: int | None
    branches_valid: int | None
    threshold_percent: float
    status: str
    source_xml: str
    html_report: str | None


def _read_float_attribute(root: ET.Element, name: str, default: float = 0.0) -> float:
    """Reads a floating-point XML attribute with a default for absent values."""

    value = root.attrib.get(name)
    return default if value is None else float(value)


def _read_int_attribute(root: ET.Element, name: str) -> int:
    """Reads a required integer XML attribute."""

    value = root.attrib.get(name)
    if value is None:
        raise ValueError(f"Coverage XML is missing required attribute '{name}'")
    return int(value)


def _matches_prefix(path: Path, prefixes: list[Path]) -> bool:
    """Returns whether the path is inside any configured prefix."""

    if not prefixes:
        return True
    resolved = path.resolve()
    for prefix in prefixes:
        try:
            resolved.relative_to(prefix.resolve())
            return True
        except ValueError:
            continue
    return False


def _filtered_line_counts(
    root: ET.Element, include_prefixes: list[Path], exclude_prefixes: list[Path]
) -> tuple[int, int]:
    """Counts covered and valid Cobertura lines after source-file filtering."""

    valid_lines: set[tuple[str, int]] = set()
    covered_lines: set[tuple[str, int]] = set()
    for class_node in root.findall(".//class"):
        filename = class_node.attrib.get("filename")
        if not filename:
            continue
        path = Path(filename)
        if not _matches_prefix(path, include_prefixes):
            continue
        if exclude_prefixes and _matches_prefix(path, exclude_prefixes):
            continue

        key_path = str(path.resolve())
        for line_node in class_node.findall(".//line"):
            line_number = int(line_node.attrib["number"])
            line_key = (key_path, line_number)
            valid_lines.add(line_key)
            if int(line_node.attrib.get("hits", "0")) > 0:
                covered_lines.add(line_key)
    return len(covered_lines), len(valid_lines)


def parse_coverage_metric(
    xml_path: Path,
    threshold_percent: float,
    html_report: Path | None,
    include_prefixes: list[Path] | None = None,
    exclude_prefixes: list[Path] | None = None,
) -> CoverageMetric:
    """Parses a Cobertura report into the platform coverage metric."""

    root = ET.parse(xml_path).getroot()
    include_prefixes = include_prefixes or []
    exclude_prefixes = exclude_prefixes or []
    if include_prefixes or exclude_prefixes:
        lines_covered, lines_valid = _filtered_line_counts(root, include_prefixes, exclude_prefixes)
    else:
        lines_covered = _read_int_attribute(root, "lines-covered")
        lines_valid = _read_int_attribute(root, "lines-valid")
    if lines_valid <= 0:
        class_count = len(root.findall(".//class"))
        package_count = len(root.findall(".//package"))
        raise ValueError(
            "Coverage XML reports no valid lines "
            f"(packages={package_count}, classes={class_count}). "
            "For MSVC native coverage, ensure the tested binaries were built with PDB debug symbols."
        )

    line_rate = (
        lines_covered / lines_valid
        if include_prefixes or exclude_prefixes
        else _read_float_attribute(
            root,
            "line-rate",
            lines_covered / lines_valid,
        )
    )
    branches_covered = root.attrib.get("branches-covered")
    branches_valid = root.attrib.get("branches-valid")
    branch_rate = root.attrib.get("branch-rate")

    branch_coverage_percent: float | None = None
    branch_covered_value: int | None = None
    branch_valid_value: int | None = None
    if branches_covered is not None and branches_valid is not None:
        branch_covered_value = int(branches_covered)
        branch_valid_value = int(branches_valid)
        if branch_valid_value > 0:
            branch_coverage_percent = (
                float(branch_rate) * 100.0
                if branch_rate is not None
                else (branch_covered_value / branch_valid_value) * 100.0
            )

    line_coverage_percent = line_rate * 100.0
    return CoverageMetric(
        metric_name="cpp_test_line_coverage",
        generated_at_utc=datetime.now(timezone.utc).replace(microsecond=0).isoformat(),
        line_coverage_percent=round(line_coverage_percent, 2),
        lines_covered=lines_covered,
        lines_valid=lines_valid,
        branch_coverage_percent=None if branch_coverage_percent is None else round(branch_coverage_percent, 2),
        branches_covered=branch_covered_value,
        branches_valid=branch_valid_value,
        threshold_percent=threshold_percent,
        status="pass" if line_coverage_percent >= threshold_percent else "fail",
        source_xml=str(xml_path),
        html_report=None if html_report is None else str(html_report),
    )


def write_json(metric: CoverageMetric, output_path: Path) -> None:
    """Writes the coverage metric as formatted JSON."""

    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text(json.dumps(asdict(metric), indent=2, sort_keys=True) + "\n", encoding="utf-8")


def write_markdown(metric: CoverageMetric, output_path: Path) -> None:
    """Writes the coverage metric as a compact Markdown summary."""

    output_path.parent.mkdir(parents=True, exist_ok=True)
    branch_value = (
        "n/a"
        if metric.branch_coverage_percent is None
        else f"{metric.branch_coverage_percent:.2f}% ({metric.branches_covered}/{metric.branches_valid})"
    )
    output_path.write_text(
        "\n".join(
            [
                "# Test Coverage Metric",
                "",
                f"- Status: `{metric.status}`",
                f"- Line coverage: `{metric.line_coverage_percent:.2f}%` ({metric.lines_covered}/{metric.lines_valid})",
                f"- Branch coverage: `{branch_value}`",
                f"- Threshold: `{metric.threshold_percent:.2f}%`",
                f"- Generated UTC: `{metric.generated_at_utc}`",
                f"- Source XML: `{metric.source_xml}`",
                f"- HTML report: `{metric.html_report or 'n/a'}`",
                "",
            ]
        ),
        encoding="utf-8",
    )


def parse_args() -> argparse.Namespace:
    """Parses command-line options for the coverage metric generator."""

    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--fail-under-threshold",
        action="store_true",
        help="Return a non-zero exit code after writing metrics when line coverage is below threshold.",
    )
    parser.add_argument("--xml", required=True, type=Path, help="Path to gcovr Cobertura XML.")
    parser.add_argument("--json", required=True, type=Path, help="Path for the generated JSON metric.")
    parser.add_argument("--markdown", type=Path, help="Optional path for a Markdown summary.")
    parser.add_argument("--threshold", required=True, type=float, help="Minimum required line coverage percentage.")
    parser.add_argument("--html-report", type=Path, help="Optional path to the generated HTML report.")
    parser.add_argument(
        "--include-source-prefix",
        action="append",
        default=[],
        type=Path,
        help="Only count Cobertura class files under this source prefix. Repeatable.",
    )
    parser.add_argument(
        "--exclude-source-prefix",
        action="append",
        default=[],
        type=Path,
        help="Exclude Cobertura class files under this source prefix. Repeatable.",
    )
    return parser.parse_args()


def main() -> int:
    """Runs the coverage metric generator."""

    args = parse_args()
    try:
        metric = parse_coverage_metric(
            args.xml,
            args.threshold,
            args.html_report,
            args.include_source_prefix,
            args.exclude_source_prefix,
        )
    except ValueError as exc:
        print(f"Coverage metric error: {exc}")
        return 1
    write_json(metric, args.json)
    if args.markdown is not None:
        write_markdown(metric, args.markdown)

    print(
        "Coverage metric: "
        f"{metric.line_coverage_percent:.2f}% line coverage "
        f"({metric.status}, threshold {metric.threshold_percent:.2f}%)"
    )
    if args.fail_under_threshold and metric.status == "fail":
        return 2
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
