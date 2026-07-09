"""Smoke tests for Python-facing LSMC diagnostics."""

from __future__ import annotations

import importlib
import importlib.machinery
import math
import os
import sys
import unittest
from pathlib import Path

PROJECT_ROOT = Path(__file__).resolve().parents[2]


def add_extension_search_path() -> None:
    """Adds the local CMake extension output folder when it exists."""

    build_path = os.environ.get("QRP_PYTHON_PATH")
    if build_path:
        sys.path.insert(0, build_path)
        return

    candidates = [
        PROJECT_ROOT / "build" / "dev" / "python" / "RelWithDebInfo",
        PROJECT_ROOT / "build" / "dev" / "python" / "Release",
        PROJECT_ROOT / "build" / "python" / "RelWithDebInfo",
        PROJECT_ROOT / "build" / "python" / "Release",
    ]
    for path in candidates:
        extension_files = list(path.glob("quant_risk_platform*.pyd")) + list(path.glob("quant_risk_platform*.so"))
        if any(
            extension_file.name.endswith(suffix)
            for extension_file in extension_files
            for suffix in importlib.machinery.EXTENSION_SUFFIXES
        ):
            sys.path.insert(0, str(path))
            return


class LsmcBindingsTest(unittest.TestCase):
    """Verifies that the C++ LSMC helper exposes compact diagnostics to Python."""

    @classmethod
    def setUpClass(cls) -> None:
        add_extension_search_path()
        try:
            cls.qrp = importlib.import_module("quant_risk_platform")
        except ModuleNotFoundError as exc:
            raise unittest.SkipTest("quant_risk_platform extension is not built") from exc

    def test_american_option_helper_returns_diagnostics(self) -> None:
        request = self.qrp.AmericanOptionLsmcRequest()
        request.spot = 100.0
        request.strike = 100.0
        request.risk_free_rate = 0.05
        request.volatility = 0.20
        request.maturity = 1.0
        request.exercise_steps = 12
        request.basis_degree = 2
        request.config.num_paths = 1024
        request.config.seed = 7
        request.config.discount_rate = request.risk_free_rate

        result = self.qrp.price_american_option_lsmc(request)

        self.assertTrue(math.isfinite(result.value))
        self.assertGreater(result.standard_error, 0.0)
        self.assertEqual(result.basis_function_names, ["1", "spot", "spot^2"])
        self.assertEqual(len(result.exercise_times), request.exercise_steps + 1)
        self.assertEqual(result.exercise_times[0], 0.0)
        self.assertAlmostEqual(result.exercise_times[-1], request.maturity)
        self.assertEqual(len(result.path_values), request.config.num_paths)
        self.assertEqual(len(result.sorted_path_values), request.config.num_paths)
        self.assertEqual(result.sorted_path_values, sorted(result.path_values))
        self.assertTrue(math.isfinite(result.var_95))
        self.assertTrue(math.isfinite(result.expected_shortfall_95))
        self.assertEqual(result.config_tags["seed"], str(request.config.seed))
        self.assertEqual(result.config_tags["num_paths"], str(request.config.num_paths))
        self.assertEqual(result.config_tags["option_type"], "put")
        self.assertGreater(len(result.regression_diagnostics), 0)

        first_step = result.regression_diagnostics[0]
        self.assertGreater(first_step.sample_count, 0)
        self.assertEqual(first_step.basis_function_count, len(result.basis_function_names))
        self.assertGreaterEqual(first_step.time_index, 1)


if __name__ == "__main__":
    unittest.main()
