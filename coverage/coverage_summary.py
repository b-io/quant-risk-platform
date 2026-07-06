"""Generate committed coverage summary artifacts from C++ and Python metrics."""

from __future__ import annotations

import argparse
import json
from dataclasses import asdict, dataclass
from datetime import datetime, timezone
from pathlib import Path


@dataclass(frozen=True)
class LanguageCoverage:
    """Line coverage for one language section."""

    language: str
    line_coverage_percent: float
    lines_covered: int
    lines_valid: int
    status: str


@dataclass(frozen=True)
class CoverageSummary:
    """Combined coverage summary for repository badges and CI artifacts."""

    combined_line_coverage_percent: float
    generated_at_utc: str
    languages: list[LanguageCoverage]
    lines_covered: int
    lines_valid: int
    status: str


def badge_color(percent: float) -> str:
    """Returns a conservative badge color for the combined line score."""

    if percent >= 85.0:
        return "#2da44e"
    if percent >= 70.0:
        return "#6f9e26"
    if percent >= 50.0:
        return "#bf8700"
    return "#cf222e"


def read_language_metric(path: Path, language: str) -> LanguageCoverage:
    """Reads one generated coverage metric file."""

    payload = json.loads(path.read_text(encoding="utf-8"))
    return LanguageCoverage(
        language=language,
        line_coverage_percent=float(payload["line_coverage_percent"]),
        lines_covered=int(payload["lines_covered"]),
        lines_valid=int(payload["lines_valid"]),
        status=str(payload["status"]),
    )


def summarize(cpp_metric: Path, python_metric: Path) -> CoverageSummary:
    """Builds a weighted line-coverage summary across C++ and Python."""

    languages = [
        read_language_metric(cpp_metric, "C++"),
        read_language_metric(python_metric, "Python"),
    ]
    lines_covered = sum(item.lines_covered for item in languages)
    lines_valid = sum(item.lines_valid for item in languages)
    combined_percent = 0.0 if lines_valid == 0 else (lines_covered / lines_valid) * 100.0
    status = "pass" if all(item.status == "pass" for item in languages) else "fail"
    return CoverageSummary(
        combined_line_coverage_percent=round(combined_percent, 2),
        generated_at_utc=datetime.now(timezone.utc).replace(microsecond=0).isoformat(),
        languages=languages,
        lines_covered=lines_covered,
        lines_valid=lines_valid,
        status=status,
    )


def write_badge(summary: CoverageSummary, path: Path) -> None:
    """Writes a compact SVG coverage badge."""

    label = "coverage"
    value = f"{summary.combined_line_coverage_percent:.2f}%"
    color = badge_color(summary.combined_line_coverage_percent)
    path.write_text(
        "\n".join(
            [
                '<svg xmlns="http://www.w3.org/2000/svg" width="112" height="20" role="img" aria-label="coverage">',
                "  <title>coverage</title>",
                '  <linearGradient id="s" x2="0" y2="100%">',
                '    <stop offset="0" stop-color="#bbb" stop-opacity=".1"/>',
                '    <stop offset="1" stop-opacity=".1"/>',
                "  </linearGradient>",
                '  <clipPath id="r"><rect width="112" height="20" rx="3" fill="#fff"/></clipPath>',
                '  <g clip-path="url(#r)">',
                '    <rect width="61" height="20" fill="#555"/>',
                f'    <rect x="61" width="51" height="20" fill="{color}"/>',
                '    <rect width="112" height="20" fill="url(#s)"/>',
                "  </g>",
                '  <g fill="#fff" text-anchor="middle" '
                'font-family="Verdana,Geneva,DejaVu Sans,sans-serif" font-size="11">',
                f'    <text x="31" y="15" fill="#010101" fill-opacity=".3">{label}</text>',
                f'    <text x="31" y="14">{label}</text>',
                f'    <text x="86" y="15" fill="#010101" fill-opacity=".3">{value}</text>',
                f'    <text x="86" y="14">{value}</text>',
                "  </g>",
                "</svg>",
                "",
            ]
        ),
        encoding="utf-8",
    )


def write_markdown(summary: CoverageSummary, path: Path) -> None:
    """Writes a human-readable coverage summary."""

    lines = [
        "# Coverage Summary",
        "",
        f"- Status: `{summary.status}`",
        f"- Combined line coverage: `{summary.combined_line_coverage_percent:.2f}%` "
        f"({summary.lines_covered}/{summary.lines_valid})",
        f"- Generated UTC: `{summary.generated_at_utc}`",
        "",
        "| Language | Line coverage | Covered / Valid | Status |",
        "| --- | ---: | ---: | --- |",
    ]
    for item in summary.languages:
        lines.append(
            f"| {item.language} | {item.line_coverage_percent:.2f}% | "
            f"{item.lines_covered}/{item.lines_valid} | `{item.status}` |"
        )
    lines.append("")
    path.write_text("\n".join(lines), encoding="utf-8")


def parse_args() -> argparse.Namespace:
    """Parses command-line arguments."""

    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cpp-json", required=True, type=Path, help="Path to C++ coverage_metric.json.")
    parser.add_argument("--output-dir", required=True, type=Path, help="Directory for generated summary artifacts.")
    parser.add_argument("--python-json", required=True, type=Path, help="Path to Python coverage_metric.json.")
    return parser.parse_args()


def main() -> int:
    """Runs the coverage summary generator."""

    args = parse_args()
    args.output_dir.mkdir(parents=True, exist_ok=True)
    summary = summarize(args.cpp_json, args.python_json)

    (args.output_dir / "coverage_summary.json").write_text(
        json.dumps(asdict(summary), indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
    )
    write_badge(summary, args.output_dir / "coverage-badge.svg")
    write_markdown(summary, args.output_dir / "coverage_summary.md")

    print(
        "Combined coverage: "
        f"{summary.combined_line_coverage_percent:.2f}% line "
        f"({summary.status}, {summary.lines_covered}/{summary.lines_valid})"
    )
    return 0 if summary.status == "pass" else 1


if __name__ == "__main__":
    raise SystemExit(main())
