# Development

This page is for contributors and maintainers who change Quant Risk Platform.
For user-facing installation and examples, start with [USAGE.md](USAGE.md).

## Documentation Convention

The repository keeps the entry points split by audience:

- [README.md](README.md): user-facing overview, capabilities, and first examples;
- [USAGE.md](USAGE.md): practical installation, demos, Python, CLI, data, and outputs;
- [DEVELOPMENT.md](DEVELOPMENT.md): development, testing, coverage, style, and review workflow;
- [QUICKSTART.md](QUICKSTART.md): compact build and smoke-test command reference;
- [docs/](docs/README.md): architecture, market-data, product, model, risk, and implementation handbook;
- [scripts/README.md](scripts/README.md): script-by-script automation reference.

Keep the root README focused on what the library does and how to use it. Put development mechanics here unless they
are needed for a first successful user run.

## Development Setup

Use the checked-in scripts for the normal local workflow.

### Windows PowerShell

```powershell
copy msvc.env.cmd.example .env.cmd
.\scripts\install.ps1
.\scripts\build.ps1
.\scripts\test.ps1
```

The PowerShell scripts import `.env.cmd` automatically unless `-SkipEnv` is passed.

### Linux Or macOS

```bash
cp gcc.env.sh.example .env.sh
./scripts/install.sh
./scripts/build.sh
./scripts/test.sh
```

Use `clang.env.sh.example` instead of `gcc.env.sh.example` when validating Clang. The Bash scripts source `.env.sh`
automatically unless `-SkipEnv` is passed.

## Build Presets

The supported CMake presets are:

- `dev`: default development preset, with C++ tests, CLI, and Python bindings;
- `dev-shared`: development preset with `qrp_core` built as a shared library;
- `debug`: native C++ debug preset without Python bindings;
- `debug-shared`: native C++ debug preset with shared `qrp_core`;
- `release`: optimized production-style preset with Python bindings;
- `release-shared`: optimized preset with shared `qrp_core`;
- `coverage`: GCC/Clang C++ coverage preset.

Python bindings on Windows Debug require a matching debug CPython and are not enabled by default.

## Local Quality Gates

Before opening or updating a pull request, run the focused gates for the change. For most code changes:

```powershell
python scripts\format.py --check
.\scripts\test.ps1
```

For coverage-sensitive changes:

```powershell
.\scripts\test.ps1 -Coverage
```

For benchmark-sensitive changes:

```powershell
.\scripts\test.ps1 -Performance
```

Linux and macOS equivalents:

```bash
python scripts/format.py --check
./scripts/test.sh
./scripts/test.sh -Coverage
./scripts/test.sh -Performance
```

The test scripts expose matching public options where applicable, including `-BuildDir`, `-Config`, `-Coverage`,
`-Performance`, `-PerformanceIterations`, `-CppCoverageMinLine`, `-PythonCoverageMinLine`, `-SkipCppCoverage`,
`-SkipPythonCoverage`, `-SkipBuild`, and `-SkipEnv`. They pin `QRP_PYTHON_PATH` to the selected build output before
running Python binding tests, so stale shell-level extension paths do not mask the active build. See
[scripts/README.md](scripts/README.md) before adding or changing script parameters.

## Coverage

The repository coverage gate is 95% line coverage for both C++ and Python.

Coverage artifacts are refreshed under `coverage/`:

- `coverage/coverage-badge.svg`;
- `coverage/coverage_summary.json`;
- `coverage/coverage_summary.md`.

Windows coverage uses the Visual Studio coverage collector when available. GCC and Clang coverage use the CMake
coverage target through gcovr. MSVC coverage honors `QRP_MSVC_COVERAGE_GTEST_FILTER` when the variable is set, which is
useful for narrowing instrumented GoogleTest runs during local debugging.

## Style

Follow [STYLE.md](STYLE.md) for C++, Python, Markdown, commit messages, ordering, and documentation conventions.

Formatter commands:

```powershell
python scripts\format.py --fix
python scripts\format.py --check
```

Use concise comments that explain intent, assumptions, invariants, or non-obvious business rules. Avoid comments that
repeat the code.

## Documentation Maintenance

Keep public documentation phase-agnostic unless the file is explicitly about implementation sequencing. The phased plan
belongs under `docs/implementation/`; user docs should describe current capabilities and boundaries.

When updating Markdown math, use dollar-delimited LaTeX and avoid unsupported GitHub macros such as
`\operatorname{...}`. Prefer `\mathrm{Var}(X)`, `\mathrm{Cov}(X,Y)`, or prose around the formula.

When implementation scope changes:

- update [README.md](README.md) and [USAGE.md](USAGE.md) for user-visible capabilities;
- update relevant architecture, market-data, asset-class, model, or risk docs under `docs/`;
- update [docs/implementation/PHASED_BUILD_PLAN.md](docs/implementation/PHASED_BUILD_PLAN.md) when phase status changes;
- keep each phased-plan section's `Current implementation checkpoint:` block current;
- update examples and golden outputs when public demo behavior changes.

## Branch And Commit Workflow

Create feature branches from `main`. Rebase onto `main` before opening or refreshing a pull request when the branch is
behind and you want a linear history.

Use Conventional Commits:

```text
docs(readme): split usage and development docs
feat(risk): add var contribution reporting
fix(test): avoid dangling contribution references
```

Keep the subject imperative, searchable, and under 72 characters. Use the body to explain why the change was needed.

## Pull Request Checklist

Before review, check that:

- formatting passes;
- relevant C++ and Python tests pass;
- coverage stays at or above 95% when the change affects executable code;
- scripts remain functionally aligned between PowerShell and Bash where both variants exist;
- README and user docs still describe how to run the current library;
- maintainer docs describe any new workflow or gate;
- generated artifacts are intentional.

## Local Outputs

Local generated state should normally stay out of commits:

- `build/`: CMake build trees;
- `var/`: SQLite database and logs;
- `reports/`: demo dashboards and generated reports;
- machine-local `.env.cmd` and `.env.sh` files.

Committed generated coverage summaries under `coverage/` are intentional when coverage workflows refresh them.
