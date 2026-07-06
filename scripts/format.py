"""Format or check C++ and Python source files for the project."""

from __future__ import annotations

import argparse
import shutil
import subprocess
import sys
from pathlib import Path

PROJECT_ROOT = Path(__file__).resolve().parents[1]
RUFF_CONFIG = PROJECT_ROOT / "ruff.toml"

CPP_EXTENSIONS = {".c", ".cc", ".cpp", ".cxx", ".h", ".hh", ".hpp", ".hxx"}
PYTHON_EXTENSIONS = {".py"}

DEFAULT_CPP_ROOTS = (PROJECT_ROOT / "cpp", PROJECT_ROOT / "tests")
DEFAULT_PYTHON_ROOTS = (
    PROJECT_ROOT / "coverage",
    PROJECT_ROOT / "python",
    PROJECT_ROOT / "scripts",
    PROJECT_ROOT / "tests",
)

EXCLUDED_PARTS = {
    ".git",
    ".pytest_cache",
    ".ruff_cache",
    ".venv",
    ".vs",
    "build",
    "quant_risk_platform.egg-info",
    "reports",
    "temp",
    "var",
}


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument("--check", action="store_true", help="Check formatting without changing files.")
    mode.add_argument("--fix", action="store_true", help="Format files in place.")
    parser.add_argument(
        "--scope",
        choices=("all", "cpp", "python"),
        default="all",
        help="Language scope to format or check.",
    )
    parser.add_argument("paths", nargs="*", help="Optional files or directories to limit the run.")
    return parser.parse_args()


def is_excluded(path: Path) -> bool:
    try:
        relative = path.resolve().relative_to(PROJECT_ROOT)
    except ValueError:
        return True
    return any(part in EXCLUDED_PARTS for part in relative.parts)


def collect_files(paths: list[str], roots: tuple[Path, ...], extensions: set[str]) -> list[Path]:
    candidates = [Path(path) for path in paths] if paths else list(roots)
    files: list[Path] = []
    for candidate in candidates:
        path = candidate if candidate.is_absolute() else PROJECT_ROOT / candidate
        if not path.exists() or is_excluded(path):
            continue
        if path.is_file():
            if path.suffix.lower() in extensions:
                files.append(path)
            continue
        files.extend(
            child
            for child in path.rglob("*")
            if child.is_file() and child.suffix.lower() in extensions and not is_excluded(child)
        )
    return sorted(set(files), key=lambda item: item.as_posix().lower())


def require_tool(name: str) -> str:
    path = shutil.which(name)
    if path:
        return path
    raise RuntimeError(
        f"Required formatter '{name}' was not found on PATH. "
        "Install it locally or run the CI setup commands from .github/workflows/tests.yml."
    )


def run_command(command: list[str]) -> int:
    print("+ " + " ".join(command))
    return subprocess.run(command, cwd=PROJECT_ROOT, check=False).returncode


def run_batched(base_command: list[str], files: list[Path], *, batch_size: int = 80) -> int:
    exit_code = 0
    for index in range(0, len(files), batch_size):
        batch = [str(path.relative_to(PROJECT_ROOT)) for path in files[index : index + batch_size]]
        exit_code = run_command([*base_command, *batch]) or exit_code
    return exit_code


def run_cpp(files: list[Path], check: bool) -> int:
    if not files:
        print("No C++ files matched.")
        return 0

    clang_format = require_tool("clang-format")
    if check:
        return run_batched([clang_format, "--dry-run", "--Werror"], files)
    return run_batched([clang_format, "-i"], files)


def run_python(files: list[Path], check: bool) -> int:
    if not files:
        print("No Python files matched.")
        return 0

    ruff = require_tool("ruff")
    paths = [str(path.relative_to(PROJECT_ROOT)) for path in files]
    check_command = [ruff, "check", "--config", str(RUFF_CONFIG)]
    format_command = [ruff, "format", "--config", str(RUFF_CONFIG)]
    if check:
        lint_exit = run_command([*check_command, *paths])
        format_exit = run_command([*format_command, "--check", *paths])
        return lint_exit or format_exit

    lint_exit = run_command([*check_command, "--fix", *paths])
    format_exit = run_command([*format_command, *paths])
    return lint_exit or format_exit


def main() -> int:
    args = parse_args()
    check = not args.fix

    cpp_files = collect_files(args.paths, DEFAULT_CPP_ROOTS, CPP_EXTENSIONS)
    python_files = collect_files(args.paths, DEFAULT_PYTHON_ROOTS, PYTHON_EXTENSIONS)

    exit_code = 0
    try:
        if args.scope in ("all", "cpp"):
            exit_code = run_cpp(cpp_files, check) or exit_code
        if args.scope in ("all", "python"):
            exit_code = run_python(python_files, check) or exit_code
    except RuntimeError as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 2
    return exit_code


if __name__ == "__main__":
    sys.exit(main())
