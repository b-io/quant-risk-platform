#!/bin/bash
# Format or check C++ and Python source files.

set -euo pipefail

SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
python "$SCRIPT_DIR/format.py" "$@"
