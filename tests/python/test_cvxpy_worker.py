"""Tests CVXPY worker validation and request translation without solver packages."""

from __future__ import annotations

import builtins
import importlib
import importlib.util
import io
import json
import math
import runpy
import sys
import types
import unittest
from pathlib import Path
from unittest import mock


def install_optional_dependency_stubs() -> None:
    """Installs import stubs when NumPy or CVXPY are unavailable."""

    if importlib.util.find_spec("numpy") is None:
        sys.modules["numpy"] = types.ModuleType("numpy")

    if importlib.util.find_spec("cvxpy") is None:
        sys.modules["cvxpy"] = types.ModuleType("cvxpy")


install_optional_dependency_stubs()
PROJECT_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(PROJECT_ROOT / "python"))
worker = importlib.import_module("qrp.optimization.cvxpy_worker")


class FakeExpression:
    """Small expression object that supports CVXPY-like operator chaining."""

    def __add__(self, other):  # noqa: ANN001
        return self

    def __radd__(self, other):  # noqa: ANN001
        return self

    def __sub__(self, other):  # noqa: ANN001
        return self

    def __rsub__(self, other):  # noqa: ANN001
        return self

    def __mul__(self, other):  # noqa: ANN001
        return self

    def __rmul__(self, other):  # noqa: ANN001
        return self

    def __matmul__(self, other):  # noqa: ANN001
        return self

    def __rmatmul__(self, other):  # noqa: ANN001
        return self

    def __ge__(self, other):  # noqa: ANN001
        return ("ge", other)

    def __le__(self, other):  # noqa: ANN001
        return ("le", other)

    def __eq__(self, other):  # noqa: ANN001
        return ("eq", other)


class FakeVector:
    """Dense vector test double for NumPy arrays and CVXPY variables."""

    def __init__(self, values_or_size):
        if isinstance(values_or_size, int):
            self.values = [0.0 for _ in range(values_or_size)]
        else:
            self.values = [float(value) for value in values_or_size]
        self.value = [0.25 for _ in self.values]

    def __iter__(self):
        return iter(self.values)

    def __getitem__(self, index):
        return FakeExpression() if isinstance(index, int) else self.values[index]

    def __setitem__(self, index, value):
        self.values[index] = float(value)

    def __sub__(self, other):  # noqa: ANN001
        return FakeExpression()

    def __matmul__(self, other):  # noqa: ANN001
        return FakeExpression()

    def __rmatmul__(self, other):  # noqa: ANN001
        return FakeExpression()


class FakeMatrix:
    """Matrix test double carrying shape and PSD diagnostics."""

    def __init__(self, rows, *, finite=True, symmetric=True, min_eigenvalue=0.1, shape=None):
        self.rows = rows
        self.finite = finite
        self.symmetric = symmetric
        self.min_eigenvalue = min_eigenvalue
        self.ndim = 2
        self.shape = shape or (len(rows), len(rows[0]) if rows else 0)

    @property
    def T(self):
        return self

    def __getitem__(self, index):
        if isinstance(index, tuple):
            row, column = index
            if isinstance(column, slice):
                return self.rows[row]
            return self.rows[row][column]
        return self.rows[index]

    def __setitem__(self, index, value):
        row, column = index
        if isinstance(column, slice):
            self.rows[row] = list(value)
        else:
            self.rows[row][column] = value

    def __matmul__(self, other):  # noqa: ANN001
        if isinstance(other, FakeVector):
            return FakeExpression()
        return FakeMatrix(
            self.rows,
            finite=self.finite,
            symmetric=self.symmetric,
            min_eigenvalue=self.min_eigenvalue,
        )


class FakeEigenvalues(list):
    """Eigenvalue container exposing NumPy's min method."""

    def min(self):
        return builtins.min(self)


class FakeNumpy:
    """Subset of NumPy used by cvxpy_worker.py."""

    class linalg:
        @staticmethod
        def eigvalsh(matrix):
            return FakeEigenvalues([matrix.min_eigenvalue])

    @staticmethod
    def all(values):
        return builtins.all(values)

    @staticmethod
    def allclose(left, right, atol=0.0, rtol=0.0):  # noqa: ANN001, ARG004
        return getattr(left, "symmetric", True)

    @staticmethod
    def asarray(value, dtype=float):  # noqa: ANN001, ARG004
        if isinstance(value, FakeMatrix):
            return value
        return FakeMatrix([[float(cell) for cell in row] for row in value])

    @staticmethod
    def isfinite(value):  # noqa: ANN001
        if isinstance(value, FakeMatrix):
            return [value.finite for _ in range(value.shape[0] * value.shape[1])]
        if isinstance(value, FakeVector):
            return [math.isfinite(item) for item in value.values]
        return math.isfinite(float(value))

    @staticmethod
    def zeros(size):
        if isinstance(size, tuple):
            rows, columns = size
            return FakeMatrix([[0.0 for _ in range(columns)] for _ in range(rows)])
        return FakeVector(size)


class FakeCvxpy:
    """Subset of CVXPY used by cvxpy_worker.py."""

    OPTIMAL = "optimal"
    OPTIMAL_INACCURATE = "optimal_inaccurate"
    CLARABEL = "CLARABEL"
    OSQP = "OSQP"
    SCS = "SCS"

    class Problem:
        def __init__(self, objective, constraints):  # noqa: ANN001
            self.objective = objective
            self.constraints = constraints
            self.status = FakeCvxpy.OPTIMAL
            self.value = 1.23

        def solve(self, solver, verbose=False, **options):  # noqa: ANN001, ARG002
            self.solver = solver
            self.verbose = verbose
            self.options = options

    @staticmethod
    def Maximize(expression):  # noqa: ANN001
        return expression

    @staticmethod
    def Variable(size, name=None):  # noqa: ARG004
        return FakeVector(size)

    @staticmethod
    def multiply(left, right):  # noqa: ANN001
        return FakeExpression()

    @staticmethod
    def norm(expression, order):  # noqa: ANN001, ARG004
        return FakeExpression()

    @staticmethod
    def psd_wrap(matrix):  # noqa: ANN001
        return matrix

    @staticmethod
    def quad_form(expression, matrix):  # noqa: ANN001
        return FakeExpression()

    @staticmethod
    def square(expression):  # noqa: ANN001
        return FakeExpression()

    @staticmethod
    def sum(expression):  # noqa: ANN001
        return FakeExpression()


worker.np = FakeNumpy
worker.cp = FakeCvxpy
sys.modules["numpy"] = FakeNumpy
sys.modules["cvxpy"] = FakeCvxpy


def base_request() -> dict:
    """Returns a minimal valid long-only return-maximization request."""

    return {
        "variables": [{"id": "A", "lb": 0.0, "ub": 1.0}],
        "objectives": [{"type": "MaximizeReturn", "expected_returns": {"A": 0.05}}],
        "constraints": [],
        "config": {"solver": "OSQP", "tolerance": 1.0e-6, "max_iterations": 100},
    }


class CvxpyWorkerHelperTest(unittest.TestCase):
    """Covers deterministic request-validation helpers used by the worker."""

    def assert_worker_error(self, request: dict, message: str) -> None:
        result = worker.solve_optimization(request)

        self.assertEqual(result["status"], "error")
        self.assertIn(message, result["message"])

    def test_assert_psd_accepts_valid_matrix(self) -> None:
        matrix = FakeMatrix([[1.0, 0.1], [0.1, 1.0]])

        self.assertIs(worker._assert_psd(matrix, "risk"), matrix)

    def test_assert_psd_rejects_invalid_matrices(self) -> None:
        cases = [
            (FakeMatrix([[1.0, 2.0]], shape=(1, 2)), "square"),
            (FakeMatrix([[1.0]], finite=False), "non-finite"),
            (FakeMatrix([[1.0, 2.0], [0.0, 1.0]], symmetric=False), "symmetric"),
            (FakeMatrix([[1.0]], min_eigenvalue=-1.0), "positive semidefinite"),
        ]
        for matrix, message in cases:
            with self.subTest(message=message):
                with self.assertRaisesRegex(ValueError, message):
                    worker._assert_psd(matrix, "risk")

    def test_require_finite_accepts_numeric_values(self) -> None:
        self.assertEqual(worker._require_finite("1.25", "value"), 1.25)

    def test_require_finite_rejects_invalid_values(self) -> None:
        with self.assertRaisesRegex(ValueError, "must be numeric"):
            worker._require_finite("not-a-number", "value")
        with self.assertRaisesRegex(ValueError, "must be finite"):
            worker._require_finite(float("inf"), "value")

    def test_vector_from_mapping_builds_dense_vector(self) -> None:
        vector = worker._vector_from_mapping(
            {"B": 2.5},
            {"A": 0, "B": 1, "C": 2},
            3,
            "weights",
        )

        self.assertEqual(list(vector), [0.0, 2.5, 0.0])

    def test_vector_from_mapping_validates_required_entries(self) -> None:
        with self.assertRaisesRegex(ValueError, "must not be empty"):
            worker._vector_from_mapping({}, {"A": 0}, 1, "weights", require_non_empty=True)
        with self.assertRaisesRegex(ValueError, "unknown asset"):
            worker._vector_from_mapping({"B": 1.0}, {"A": 0}, 1, "weights")

    def test_solver_options_translate_common_solver_controls(self) -> None:
        config = {"tolerance": 1.0e-6, "max_iterations": 250, "time_limit": 5.0}

        self.assertEqual(
            worker._solver_options(config, "OSQP"),
            {"eps_abs": 1.0e-6, "eps_rel": 1.0e-6, "max_iter": 250, "time_limit": 5.0},
        )
        self.assertEqual(
            worker._solver_options(config, "SCS"),
            {"eps": 1.0e-6, "max_iters": 250, "time_limit_secs": 5.0},
        )
        self.assertEqual(
            worker._solver_options(config, "CLARABEL"),
            {
                "tol_feas": 1.0e-6,
                "tol_gap_abs": 1.0e-6,
                "tol_gap_rel": 1.0e-6,
                "max_iter": 250,
                "time_limit": 5.0,
            },
        )
        self.assertEqual(worker._solver_options({}, "OTHER"), {})

    def test_objective_needs_risk_only_when_required(self) -> None:
        self.assertFalse(worker._objective_needs_risk({"type": "MaximizeReturn"}))
        self.assertFalse(worker._objective_needs_risk({"type": "MeanVariance", "risk_aversion": 0.0}))
        self.assertTrue(worker._objective_needs_risk({"type": "MeanVariance", "risk_aversion": 1.0}))
        self.assertTrue(worker._objective_needs_risk({"type": "MinimumVariance"}))
        self.assertTrue(worker._objective_needs_risk({"type": "TrackingError"}))

    def test_build_risk_expression_supports_full_covariance(self) -> None:
        risk_expression = worker._build_risk_expression(
            {
                "type": "FullCovariance",
                "asset_ids": ["B", "A"],
                "covariance": [[0.2, 0.0], [0.0, 0.1]],
            },
            ["A", "B"],
            {"A": 0, "B": 1},
        )

        self.assertIsNotNone(risk_expression)
        self.assertIsInstance(risk_expression(FakeVector(2)), FakeExpression)

    def test_build_risk_expression_validates_risk_models(self) -> None:
        self.assertIsNone(worker._build_risk_expression({}, ["A"], {"A": 0}))
        cases = [
            ({"type": "FullCovariance", "asset_ids": ["A"]}, "missing assets"),
            (
                {
                    "type": "FactorRisk",
                    "asset_ids": ["A"],
                    "exposures": [[1.0, 2.0]],
                    "factor_covariance": [[1.0]],
                },
                "asset-by-factor",
            ),
            (
                {
                    "type": "FactorRisk",
                    "asset_ids": ["A"],
                    "exposures": FakeMatrix([[1.0]], finite=False),
                    "factor_covariance": [[1.0]],
                },
                "non-finite",
            ),
            (
                {
                    "type": "FactorRisk",
                    "asset_ids": ["A"],
                    "exposures": [[1.0]],
                    "factor_covariance": [[1.0]],
                    "specific_risk": {"B": 0.1},
                },
                "unknown asset",
            ),
            (
                {
                    "type": "FactorRisk",
                    "asset_ids": ["A"],
                    "exposures": [[1.0]],
                    "factor_covariance": [[1.0]],
                    "specific_risk": {"A": -0.1},
                },
                "non-negative",
            ),
            ({"type": "Unsupported", "asset_ids": ["A"]}, "Unsupported risk model type"),
        ]
        for risk_model, message in cases:
            with self.subTest(message=message):
                with self.assertRaisesRegex(ValueError, message):
                    worker._build_risk_expression(
                        risk_model, ["A", "B"] if message == "missing assets" else ["A"], {"A": 0}
                    )

    def test_build_risk_expression_supports_factor_risk(self) -> None:
        risk_expression = worker._build_risk_expression(
            {
                "type": "FactorRisk",
                "asset_ids": ["A"],
                "exposures": [[1.0]],
                "factor_covariance": [[1.0]],
                "specific_risk": {"A": 0.05},
            },
            ["A"],
            {"A": 0},
        )

        self.assertIsNotNone(risk_expression)
        self.assertIsInstance(risk_expression(FakeVector(1)), FakeExpression)

    def test_solve_optimization_solves_simple_return_problem(self) -> None:
        result = worker.solve_optimization(base_request())

        self.assertEqual(result["status"], "optimal")
        self.assertEqual(result["objective_value"], 1.23)
        self.assertEqual(result["optimal_values"], {"A": 0.25})

    def test_solve_optimization_reports_request_shape_errors(self) -> None:
        cases = [
            ({"variables": [], "objectives": []}, "at least one variable"),
            ({"variables": [{"id": "A"}], "objectives": []}, "at least one objective"),
            ({**base_request(), "variables": [{"id": ""}]}, "non-empty"),
            ({**base_request(), "variables": [{"id": "A"}, {"id": "A"}]}, "unique"),
        ]
        for request, message in cases:
            with self.subTest(message=message):
                self.assert_worker_error(request, message)

    def test_solve_optimization_validates_variable_bounds(self) -> None:
        cases = [
            ([{"id": "A", "integer": True}], "Integer variables"),
            ([{"id": "A", "lb": 2.0, "ub": 1.0}], "lower bound greater"),
        ]
        for variables, message in cases:
            request = base_request()
            request["variables"] = variables
            with self.subTest(message=message):
                self.assert_worker_error(request, message)

    def test_solve_optimization_validates_constraints(self) -> None:
        cases = [
            (
                {"type": "LinearEquality", "coefficients": {"B": 1.0}, "target": 1.0},
                "unknown asset",
            ),
            ({"type": "LinearInequality", "coefficients": {"A": 1.0}}, "at least one bound"),
            (
                {"type": "LinearInequality", "coefficients": {"A": 1.0}, "lb": 2.0, "ub": 1.0},
                "cannot exceed",
            ),
            (
                {"type": "Turnover", "current_weights": {"A": 0.5}, "max_turnover": -0.1},
                "non-negative",
            ),
            ({"type": "Unsupported"}, "Unsupported constraint type"),
        ]
        for constraint, message in cases:
            request = base_request()
            request["constraints"] = [constraint]
            with self.subTest(message=message):
                self.assert_worker_error(request, message)

    def test_solve_optimization_validates_objectives(self) -> None:
        cases = [
            (
                {"type": "MeanVariance", "expected_returns": {"A": 0.05}, "risk_aversion": -1.0},
                "non-negative",
            ),
            (
                {"type": "MeanVariance", "expected_returns": {"A": 0.05}, "risk_aversion": 1.0},
                "requires a risk model",
            ),
            ({"type": "MinimumVariance"}, "requires a risk model"),
            ({"type": "MaximizeReturn", "expected_returns": {"B": 0.05}}, "unknown asset"),
            ({"type": "TrackingError", "benchmark_weights": {"A": 1.0}}, "requires a risk model"),
            ({"type": "Unsupported"}, "Unsupported objective type"),
        ]
        for objective, message in cases:
            request = base_request()
            request["objectives"] = [objective]
            with self.subTest(message=message):
                self.assert_worker_error(request, message)

    def test_solve_optimization_supports_risk_backed_mean_variance(self) -> None:
        request = base_request()
        request["objectives"] = [{"type": "MeanVariance", "expected_returns": {"A": 0.05}, "risk_aversion": 1.0}]
        request["risk_model"] = {
            "type": "FullCovariance",
            "asset_ids": ["A"],
            "covariance": [[0.1]],
        }

        result = worker.solve_optimization(request)

        self.assertEqual(result["status"], "optimal")

    def test_solve_optimization_supports_constraint_forms_and_risk_objectives(self) -> None:
        request = base_request()
        request["constraints"] = [
            {"type": "LinearEquality", "coefficients": {"A": 1.0}, "target": 1.0},
            {"type": "LinearInequality", "coefficients": {"A": 1.0}, "lb": 0.0, "ub": 1.0},
            {"type": "Turnover", "current_weights": {"A": 0.5}, "max_turnover": 0.75},
        ]
        request["objectives"] = [
            {"type": "MinimumVariance"},
            {"type": "TrackingError", "benchmark_weights": {"A": 1.0}},
        ]
        request["risk_model"] = {
            "type": "FullCovariance",
            "asset_ids": ["A"],
            "covariance": [[0.1]],
        }

        result = worker.solve_optimization(request)

        self.assertEqual(result["status"], "optimal")

    def test_solve_optimization_rejects_unknown_solver(self) -> None:
        request = base_request()
        request["config"]["solver"] = "missing_solver"

        self.assert_worker_error(request, "not available")

    def test_command_line_entrypoint_writes_result_file(self) -> None:
        temp_dir = PROJECT_ROOT / "temp"
        temp_dir.mkdir(exist_ok=True)
        request_path = temp_dir / "cvxpy_worker_request_test.json"
        response_path = temp_dir / "cvxpy_worker_response_test.json"
        request_path.write_text(json.dumps(base_request()), encoding="utf-8")

        with mock.patch.object(sys, "argv", ["cvxpy_worker.py", str(request_path), str(response_path)]):
            runpy.run_path(
                str(PROJECT_ROOT / "python" / "qrp" / "optimization" / "cvxpy_worker.py"),
                run_name="__main__",
            )

        response = json.loads(response_path.read_text(encoding="utf-8"))
        self.assertEqual(response["status"], "optimal")

    def test_command_line_entrypoint_reports_usage_without_paths(self) -> None:
        with (
            mock.patch.object(sys, "argv", ["cvxpy_worker.py"]),
            mock.patch("sys.stdout", new_callable=io.StringIO) as stdout,
        ):
            with self.assertRaises(SystemExit) as exit_context:
                runpy.run_path(
                    str(PROJECT_ROOT / "python" / "qrp" / "optimization" / "cvxpy_worker.py"),
                    run_name="__main__",
                )

        self.assertEqual(exit_context.exception.code, 1)
        self.assertIn("Usage: cvxpy_worker.py", stdout.getvalue())


if __name__ == "__main__":
    unittest.main()
