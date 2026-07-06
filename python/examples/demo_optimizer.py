"""Demonstrates the optional CVXPY portfolio optimizer worker."""

import json
import math
import sys
from pathlib import Path

PROJECT_ROOT = Path(__file__).resolve().parents[2]
PYTHON_ROOT = PROJECT_ROOT / "python"
REPORT_PATH = PROJECT_ROOT / "reports" / "demo_optimizer_result.json"

ASSETS = [
    {
        "id": "TTF_OPTIONALITY",
        "asset_class": "commodity",
        "expected_return": 0.090,
        "current_weight": 0.04,
        "ub": 0.10,
    },
    {
        "id": "WTI_ENERGY",
        "asset_class": "commodity",
        "expected_return": 0.075,
        "current_weight": 0.06,
        "ub": 0.12,
    },
    {
        "id": "IG_CREDIT",
        "asset_class": "credit",
        "expected_return": 0.065,
        "current_weight": 0.18,
        "ub": 0.30,
    },
    {
        "id": "AAPL_EQUITY",
        "asset_class": "equity",
        "expected_return": 0.105,
        "current_weight": 0.16,
        "ub": 0.25,
    },
    {
        "id": "SPX_FUTURE",
        "asset_class": "equity",
        "expected_return": 0.085,
        "current_weight": 0.18,
        "ub": 0.30,
    },
    {
        "id": "EURUSD_CARRY",
        "asset_class": "fx",
        "expected_return": 0.045,
        "current_weight": 0.08,
        "ub": 0.15,
    },
    {
        "id": "USD_CORE_BONDS",
        "asset_class": "rates",
        "expected_return": 0.052,
        "current_weight": 0.20,
        "ub": 0.35,
    },
    {
        "id": "USD_LIQUIDITY",
        "asset_class": "rates",
        "expected_return": 0.042,
        "current_weight": 0.10,
        "ub": 0.25,
    },
]

FACTOR_VOLS = [0.04, 0.06, 0.16, 0.08, 0.22]
LOADINGS = {
    "TTF_OPTIONALITY": [0.00, 0.05, 0.15, 0.05, 1.15],
    "WTI_ENERGY": [0.00, 0.10, 0.25, 0.05, 1.00],
    "IG_CREDIT": [0.45, 0.85, 0.20, 0.00, 0.00],
    "AAPL_EQUITY": [0.05, 0.10, 1.10, 0.00, 0.05],
    "SPX_FUTURE": [0.05, 0.05, 1.00, 0.00, 0.00],
    "EURUSD_CARRY": [0.15, 0.00, 0.10, 0.95, 0.00],
    "USD_CORE_BONDS": [1.10, 0.10, -0.10, 0.00, 0.00],
    "USD_LIQUIDITY": [0.25, 0.00, 0.00, 0.00, 0.00],
}
SPECIFIC_VOLS = {
    "TTF_OPTIONALITY": 0.140,
    "WTI_ENERGY": 0.120,
    "IG_CREDIT": 0.025,
    "AAPL_EQUITY": 0.100,
    "SPX_FUTURE": 0.060,
    "EURUSD_CARRY": 0.035,
    "USD_CORE_BONDS": 0.015,
    "USD_LIQUIDITY": 0.005,
}


def import_worker():
    if str(PYTHON_ROOT) not in sys.path:
        sys.path.insert(0, str(PYTHON_ROOT))
    try:
        from qrp.optimization import cvxpy_worker
    except ImportError as exc:
        print(f"Skipped: CVXPY worker dependencies are not available ({exc}).")
        print("Install the optional optimization dependencies with:")
        print("  uv sync --project python --extra optimization")
        return None
    return cvxpy_worker


def sort_text(value):
    return str(value or "").casefold()


def asset_sort_key(asset):
    return (sort_text(asset["asset_class"]), sort_text(asset["id"]))


def sorted_assets():
    return sorted(ASSETS, key=asset_sort_key)


def asset_ids():
    return [asset["id"] for asset in sorted_assets()]


def mapping(field):
    return {asset["id"]: asset[field] for asset in sorted_assets()}


def class_coefficients(*asset_classes):
    selected = set(asset_classes)
    return {asset["id"]: 1.0 for asset in sorted_assets() if asset["asset_class"] in selected}


def covariance_matrix():
    ids = asset_ids()
    factor_variances = [vol * vol for vol in FACTOR_VOLS]
    covariance = []
    for left_id in ids:
        row = []
        left_loadings = LOADINGS[left_id]
        for right_id in ids:
            right_loadings = LOADINGS[right_id]
            value = sum(
                left * right * factor_variance
                for left, right, factor_variance in zip(left_loadings, right_loadings, factor_variances)
            )
            if left_id == right_id:
                value += SPECIFIC_VOLS[left_id] ** 2
            row.append(value)
        covariance.append(row)
    return covariance


def build_request():
    ids = asset_ids()
    return {
        "name": "Demo multi-asset mean-variance allocation",
        "variables": [{"id": asset["id"], "lb": 0.0, "ub": asset["ub"], "integer": False} for asset in sorted_assets()],
        "objectives": [
            {
                "type": "MeanVariance",
                "expected_returns": mapping("expected_return"),
                "risk_aversion": 3.0,
            }
        ],
        "constraints": [
            {
                "type": "LinearEquality",
                "coefficients": {asset_id: 1.0 for asset_id in ids},
                "target": 1.0,
            },
            {
                "type": "LinearInequality",
                "coefficients": class_coefficients("rates", "credit"),
                "lb": 0.30,
                "ub": 0.62,
            },
            {
                "type": "LinearInequality",
                "coefficients": class_coefficients("equity"),
                "ub": 0.40,
            },
            {
                "type": "LinearInequality",
                "coefficients": class_coefficients("fx"),
                "ub": 0.15,
            },
            {
                "type": "LinearInequality",
                "coefficients": class_coefficients("fx", "commodity"),
                "lb": 0.08,
                "ub": 0.26,
            },
            {
                "type": "LinearInequality",
                "coefficients": class_coefficients("commodity"),
                "ub": 0.18,
            },
            {
                "type": "Turnover",
                "current_weights": mapping("current_weight"),
                "max_turnover": 0.35,
            },
        ],
        "risk_model": {
            "type": "FullCovariance",
            "asset_ids": ids,
            "covariance": covariance_matrix(),
        },
        "config": {
            "solver": "OSQP",
            "tolerance": 1e-8,
            "max_iterations": 10000,
            "verbose": False,
        },
    }


def portfolio_metrics(weights, covariance):
    expected_returns = mapping("expected_return")
    expected_return = sum(expected_returns[asset_id] * weight for asset_id, weight in weights.items())
    ordered_weights = [weights[asset_id] for asset_id in asset_ids()]
    variance = 0.0
    for i, left_weight in enumerate(ordered_weights):
        for j, right_weight in enumerate(ordered_weights):
            variance += left_weight * covariance[i][j] * right_weight
    turnover = sum(abs(weights[asset["id"]] - asset["current_weight"]) for asset in sorted_assets())
    return {
        "expected_return": expected_return,
        "volatility": math.sqrt(max(variance, 0.0)),
        "turnover": turnover,
    }


def clean_weights(weights):
    return {asset_id: 0.0 if abs(float(weight)) < 1e-8 else float(weight) for asset_id, weight in weights.items()}


def pct(value):
    return f"{100.0 * value:6.2f}%"


def print_result(result, covariance):
    weights = clean_weights(result["optimal_values"])
    metrics = portfolio_metrics(weights, covariance)
    current_metrics = portfolio_metrics(mapping("current_weight"), covariance)
    ids = asset_ids()

    print("\nQuant Risk Platform Optimizer Demo")
    print("=" * 72)
    print(f"Solver status:     {result['status']}")
    print(f"Objective value:   {result['objective_value']:.8f}")
    print(f"Solve time:        {result['solve_time_ms']:.2f} ms")
    print()
    print(f"{'Asset':<18} {'Class':<10} {'Current':>9} {'Optimized':>11} {'Change':>9} {'Exp Ret':>9} {'Vol':>9}")
    print("-" * 82)
    for asset in sorted_assets():
        asset_id = asset["id"]
        current_weight = asset["current_weight"]
        optimized_weight = weights[asset_id]
        single_asset_vol = math.sqrt(covariance[ids.index(asset_id)][ids.index(asset_id)])
        print(
            f"{asset_id:<18} {asset['asset_class']:<10} {pct(current_weight):>9} "
            f"{pct(optimized_weight):>11} {pct(optimized_weight - current_weight):>9} "
            f"{pct(asset['expected_return']):>9} {pct(single_asset_vol):>9}"
        )
    print("-" * 82)
    print(
        f"{'Portfolio':<29} "
        f"{pct(current_metrics['expected_return']):>9} -> {pct(metrics['expected_return']):>9} "
        f"return, {pct(current_metrics['volatility']):>9} -> {pct(metrics['volatility']):>9} vol"
    )
    print(f"Turnover used:     {pct(metrics['turnover'])}")


def main():
    worker = import_worker()
    if worker is None:
        return 0

    request = build_request()
    result = worker.solve_optimization(request)
    if result.get("status") not in {"optimal", "optimal_inaccurate"}:
        print(json.dumps(result, indent=2, sort_keys=True))
        return 1
    result["optimal_values"] = clean_weights(result["optimal_values"])

    covariance = request["risk_model"]["covariance"]
    print_result(result, covariance)

    REPORT_PATH.parent.mkdir(parents=True, exist_ok=True)
    REPORT_PATH.write_text(
        json.dumps(
            {
                "current_metrics": portfolio_metrics(mapping("current_weight"), covariance),
                "optimized_metrics": portfolio_metrics(result["optimal_values"], covariance),
                "request": request,
                "result": result,
            },
            indent=2,
            sort_keys=True,
        ),
        encoding="utf-8",
    )
    print(f"\nResult JSON written to: {REPORT_PATH}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
