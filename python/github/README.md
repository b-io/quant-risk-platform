# GitHub Helpers

This folder contains small Python helpers used by GitHub Actions workflows.

- `emit_ci_error_tail.py` reads the tail of a failed CI log, escapes it for the
  GitHub Actions workflow-command format, and emits a `::error` annotation so
  the relevant failure is visible directly in the Actions UI.

GitHub workflows call this helper directly with Python, for example:

```bash
python python/github/emit_ci_error_tail.py "$log_path" "Configure build failed"
```
