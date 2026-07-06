### Tests Directory

This directory contains the project's test suite, organized by test type.

#### Folder Structure

- `unit/`: Small, focused tests for individual components (e.g., math, curve building, trade construction).
- `integration/`: Tests that verify the interaction between multiple components (e.g., full valuation flows).
- `regression/`: Tests that ensure previously fixed bugs do not reoccur.

#### Running Tests

To build and run all tests, use the provided `test.ps1` or `test.sh` scripts:
```powershell
# PowerShell
.\scripts\test.ps1 -BuildDir build\dev -Config RelWithDebInfo
```
```bash
# Bash
./scripts/test.sh -BuildDir build/dev -Config RelWithDebInfo
```

Tests use the GoogleTest (gtest) framework. Individual tests or filters can be specified when running the test executables manually from the build directory.

#### Coverage

Coverage is opt-in and intended for GCC/Clang toolchains, for example Linux, WSL, MinGW, or Clang-based CLion profiles.
Install `gcovr`, configure with coverage enabled, then build the `coverage` target:

```bash
cmake --preset coverage
cmake --build build/coverage --target coverage
```

The coverage target runs CTest, writes HTML to `build/coverage/coverage/cpp/index.html`, writes XML to
`build/coverage/coverage/cpp/coverage.xml`, writes `build/coverage/coverage/cpp/coverage_metric.json` for dashboards
or CI ingestion, and writes `build/coverage/coverage/cpp/coverage_metric.md` for quick review. The generated metric is
marked as passing or failing against `QRP_COVERAGE_MIN_LINE`; the default threshold is `95`, and CI enforces the gate
from the combined coverage summary after all language metrics have been generated.
