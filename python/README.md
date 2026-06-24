### Python Interface Directory

This directory contains the Python-based API, examples, and notebooks for the Quant Risk Platform.

#### Directory Structure

- `examples/`: Python scripts demonstrating and smoke-testing valuation, factor scenarios, stress, risk, P&L explain, Monte Carlo, and optional optimization-worker flows.
- `notebooks/`: Jupyter Notebooks providing interactive walkthroughs of the platform's features, including valuation, risk analysis, and VaR.

#### Key Components

The platform's core logic is implemented in C++ and exposed to Python using Pybind11. Python-side dependencies are managed via `requirements.txt` and `pyproject.toml` in the project root.

#### Getting Started

1. Install the Python dependencies and the `qrp` module:
   ```powershell
   .\scripts\install.ps1 -Preset Release-Python
   ```
2. Run the end-to-end demo/smoke test:
   ```powershell
   python python\examples\demo_platform.py
   ```
   The optional CVXPY worker section is skipped automatically if its optional dependencies are not installed in the Python environment used to run the script.
   Prefer installing optional dependencies with `uv` so they stay isolated from your system Python:
   ```powershell
   powershell -ExecutionPolicy ByPass -c {$env:UV_INSTALL_DIR = "D:\BIN"; irm https://astral.sh/uv/install.ps1 | iex}
   uv sync --extra dashboard --extra optimization
   uv run python python\examples\demo_platform.py
   ```
   To generate and open the interactive Plotly risk dashboard, including the finance theme and light/dark toggle:
   ```powershell
   uv run python python\examples\demo_platform.py --dashboard
   ```
3. Or launch a notebook to explore the API interactively.
