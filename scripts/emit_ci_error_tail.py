"""Emit a GitHub Actions error annotation with the tail of a log file."""

from __future__ import annotations

import argparse
from pathlib import Path


def escape_annotation(value: str) -> str:
    """Escape text for the GitHub Actions workflow command format."""
    return value.replace("%", "%25").replace("\r", "%0D").replace("\n", "%0A")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("log_path", type=Path, help="Log file whose tail should be emitted.")
    parser.add_argument("title", help="GitHub Actions annotation title.")
    parser.add_argument("--lines", type=int, default=80, help="Number of trailing log lines to include.")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    lines = args.log_path.read_text(encoding="utf-8", errors="replace").splitlines()[-args.lines :]
    message = escape_annotation("\n".join(lines))
    title = escape_annotation(args.title)
    print(f"::error title={title}::{message}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
