### Python Interface Directory

This directory contains the Python-based API, examples, and notebooks for the Quant Risk Platform.

#### Directory Structure

- `examples/`: Python scripts demonstrating and smoke-testing valuation, factor scenarios, `RevaluationSession`
  quote/scenario revaluation, dependency-graph impact diagnostics, stress, risk, P&L explain, LSMC exercise-policy
  valuation, Monte Carlo, dashboard export, and optional optimization-worker flows.
- `notebooks/`: Jupyter Notebooks providing interactive walkthroughs of the platform's features, including valuation, risk analysis, and VaR.

#### Key Components

The platform's core logic is implemented in C++ and exposed to Python using Pybind11. Python-side dependencies are managed by the `requirements.txt`, `pyproject.toml`, and `uv.lock` files in this directory.

#### Getting Started

1. Install the Python dependencies and the `qrp` module:
   ```powershell
   .\scripts\install.ps1 -Preset dev
   ```
2. Run the end-to-end demo/smoke test with the pinned Python 3.12 environment:
   ```powershell
   powershell -ExecutionPolicy ByPass -c {$env:UV_INSTALL_DIR = "<path-to-your-local-tools>"; irm https://astral.sh/uv/install.ps1 | iex}
   uv sync --project python --extra dashboard --extra optimization
   uv run --project python python python\examples\demo_platform.py
   ```
   The optional CVXPY worker section is skipped automatically if its optional dependencies are not installed in the
   Python environment used to run the script.
   To generate and open the interactive Plotly risk dashboard, including finance themes, light/dark mode, support
   coverage, selectable demo portfolios, stress/risk/Monte Carlo panels, and market-as-of/generated timestamps:
   ```powershell
   uv run --project python python python\examples\demo_platform.py --dashboard
   ```
   To run the standalone multi-asset optimizer demo through the optional CVXPY worker:
   ```powershell
   uv run --project python python python\examples\demo_optimizer.py
   ```
3. Or launch a notebook to explore the API interactively.
