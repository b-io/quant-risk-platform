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
.\scripts\test.ps1 -BuildDir build\Release-Python -Config Release
```
```bash
# Bash
./scripts/test.sh build/Release-Python Release
```

Tests use the GoogleTest (gtest) framework. Individual tests or filters can be specified when running the test executables manually from the build directory.
