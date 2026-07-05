"""Solves portfolio optimization requests sent by the C++ CVXPY adapter."""

import json
import sys
import time

import cvxpy as cp
import numpy as np


def _require_finite(value, label):
    try:
        numeric_value = float(value)
    except (TypeError, ValueError) as exc:
        raise ValueError(f"{label} must be numeric") from exc

    if not np.isfinite(numeric_value):
        raise ValueError(f"{label} must be finite")
    return numeric_value


def _vector_from_mapping(values, asset_id_to_idx, n_assets, label, require_non_empty=False):
    if require_non_empty and not values:
        raise ValueError(f"{label} must not be empty")

    vector = np.zeros(n_assets)
    for asset_id, value in values.items():
        if asset_id not in asset_id_to_idx:
            raise ValueError(f"{label} references unknown asset '{asset_id}'")
        vector[asset_id_to_idx[asset_id]] = _require_finite(value, f"{label}[{asset_id}]")
    return vector


def _assert_psd(matrix, label):
    matrix = np.asarray(matrix, dtype=float)
    if matrix.ndim != 2 or matrix.shape[0] != matrix.shape[1]:
        raise ValueError(f"{label} must be a square matrix")
    if not np.all(np.isfinite(matrix)):
        raise ValueError(f"{label} contains non-finite values")
    if not np.allclose(matrix, matrix.T, atol=1e-10, rtol=0):
        raise ValueError(f"{label} must be symmetric")

    min_eigenvalue = np.linalg.eigvalsh(matrix).min()
    if min_eigenvalue < -1e-10:
        raise ValueError(f"{label} must be positive semidefinite")
    return matrix


def _build_risk_expression(risk_model_data, asset_ids, asset_id_to_idx):
    if not risk_model_data:
        return None

    n_assets = len(asset_ids)
    risk_type = risk_model_data["type"]
    risk_asset_ids = risk_model_data["asset_ids"]
    risk_asset_id_to_idx = {asset_id: i for i, asset_id in enumerate(risk_asset_ids)}

    missing_assets = [asset_id for asset_id in asset_ids if asset_id not in risk_asset_id_to_idx]
    if missing_assets:
        raise ValueError(f"Risk model is missing assets: {', '.join(missing_assets)}")

    if risk_type == "FullCovariance":
        sigma = _assert_psd(risk_model_data["covariance"], "Full covariance matrix")
        permutation = np.zeros((n_assets, len(risk_asset_ids)))
        for i, asset_id in enumerate(asset_ids):
            permutation[i, risk_asset_id_to_idx[asset_id]] = 1.0

        sigma_aligned = _assert_psd(permutation @ sigma @ permutation.T, "Aligned covariance matrix")
        sigma_psd = cp.psd_wrap(sigma_aligned)
        return lambda weights: 0.5 * cp.quad_form(weights, sigma_psd)

    if risk_type == "FactorRisk":
        exposures = np.asarray(risk_model_data["exposures"], dtype=float)
        factor_covariance = _assert_psd(risk_model_data["factor_covariance"], "Factor covariance matrix")
        specific_risk = risk_model_data.get("specific_risk", {})
        n_factors = factor_covariance.shape[0]

        if exposures.shape != (len(risk_asset_ids), n_factors):
            raise ValueError("Factor exposures must be asset-by-factor")
        if not np.all(np.isfinite(exposures)):
            raise ValueError("Factor exposures contain non-finite values")

        aligned_exposures = np.zeros((n_assets, n_factors))
        for i, asset_id in enumerate(asset_ids):
            aligned_exposures[i, :] = exposures[risk_asset_id_to_idx[asset_id], :]

        specific_variance = np.zeros(n_assets)
        for asset_id, value in specific_risk.items():
            if asset_id not in risk_asset_id_to_idx:
                raise ValueError(f"Specific risk references unknown asset '{asset_id}'")
            variance = _require_finite(value, f"specific_risk[{asset_id}]")
            if variance < 0:
                raise ValueError(f"specific_risk[{asset_id}] must be non-negative")
            if asset_id in asset_id_to_idx:
                specific_variance[asset_id_to_idx[asset_id]] = variance

        factor_covariance_psd = cp.psd_wrap(factor_covariance)

        def factor_risk(weights):
            factor_exposure = aligned_exposures.T @ weights
            systematic_risk = cp.quad_form(factor_exposure, factor_covariance_psd)
            idiosyncratic_risk = cp.sum(cp.multiply(specific_variance, cp.square(weights)))
            return 0.5 * (systematic_risk + idiosyncratic_risk)

        return factor_risk

    raise ValueError(f"Unsupported risk model type '{risk_type}'")


def _solver_options(config, solver_name):
    solver_name = solver_name.upper()
    tolerance = config.get("tolerance")
    max_iterations = config.get("max_iterations")
    time_limit = config.get("time_limit")
    options = {}

    if solver_name == "OSQP":
        if tolerance is not None:
            options["eps_abs"] = float(tolerance)
            options["eps_rel"] = float(tolerance)
        if max_iterations is not None:
            options["max_iter"] = int(max_iterations)
        if time_limit is not None:
            options["time_limit"] = float(time_limit)
    elif solver_name == "SCS":
        if tolerance is not None:
            options["eps"] = float(tolerance)
        if max_iterations is not None:
            options["max_iters"] = int(max_iterations)
        if time_limit is not None:
            options["time_limit_secs"] = float(time_limit)
    elif solver_name == "CLARABEL":
        if tolerance is not None:
            options["tol_feas"] = float(tolerance)
            options["tol_gap_abs"] = float(tolerance)
            options["tol_gap_rel"] = float(tolerance)
        if max_iterations is not None:
            options["max_iter"] = int(max_iterations)
        if time_limit is not None:
            options["time_limit"] = float(time_limit)

    return options


def _objective_needs_risk(objective):
    objective_type = objective["type"]
    if objective_type == "MeanVariance":
        return _require_finite(objective.get("risk_aversion", 1.0), "MeanVariance.risk_aversion") > 0
    return objective_type in {"MinimumVariance", "TrackingError"}


def solve_optimization(request):
    try:
        # 1. Parse request
        variables_data = request.get("variables", [])
        objectives_data = request.get("objectives", [])
        constraints_data = request.get("constraints", [])
        risk_model_data = request.get("risk_model", {})
        config = request.get("config", {})

        if not variables_data:
            raise ValueError("Optimization request must contain at least one variable")
        if not objectives_data:
            raise ValueError("Optimization request must contain at least one objective")

        # Map asset IDs to cvxpy variables
        asset_ids = [v["id"] for v in variables_data]
        if any(not asset_id for asset_id in asset_ids):
            raise ValueError("Variable IDs must be non-empty")
        if len(set(asset_ids)) != len(asset_ids):
            raise ValueError("Variable IDs must be unique")

        n_assets = len(asset_ids)
        asset_id_to_idx = {aid: i for i, aid in enumerate(asset_ids)}

        w = cp.Variable(n_assets, name="weights")

        # 2. Build Constraints
        cvx_constraints = []

        # Variable bounds
        for v in variables_data:
            if v.get("integer", False):
                raise ValueError("Integer variables are not supported by the CVXPY adapter")
            idx = asset_id_to_idx[v["id"]]
            lower_bound = _require_finite(v.get("lb", -1e20), f"{v['id']}.lb")
            upper_bound = _require_finite(v.get("ub", 1e20), f"{v['id']}.ub")
            if lower_bound > upper_bound:
                raise ValueError(f"Variable '{v['id']}' has lower bound greater than upper bound")
            if lower_bound > -1e10:
                cvx_constraints.append(w[idx] >= lower_bound)
            if upper_bound < 1e10:
                cvx_constraints.append(w[idx] <= upper_bound)

        for c in constraints_data:
            ctype = c["type"]
            if ctype == "LinearEquality":
                coeffs = _vector_from_mapping(
                    c["coefficients"], asset_id_to_idx, n_assets, "LinearEquality.coefficients", require_non_empty=True)
                target = _require_finite(c["target"], "LinearEquality.target")
                cvx_constraints.append(coeffs @ w == target)

            elif ctype == "LinearInequality":
                coeffs = _vector_from_mapping(
                    c["coefficients"], asset_id_to_idx, n_assets, "LinearInequality.coefficients", require_non_empty=True)
                if "lb" not in c and "ub" not in c:
                    raise ValueError("LinearInequality must define at least one bound")
                lower_bound = _require_finite(c["lb"], "LinearInequality.lb") if "lb" in c else None
                upper_bound = _require_finite(c["ub"], "LinearInequality.ub") if "ub" in c else None
                if lower_bound is not None and upper_bound is not None and lower_bound > upper_bound:
                    raise ValueError("LinearInequality lower bound cannot exceed upper bound")
                if "lb" in c:
                    cvx_constraints.append(coeffs @ w >= lower_bound)
                if "ub" in c:
                    cvx_constraints.append(coeffs @ w <= upper_bound)

            elif ctype == "Turnover":
                w_curr = _vector_from_mapping(c["current_weights"], asset_id_to_idx, n_assets, "Turnover.current_weights")
                max_turnover = _require_finite(c["max_turnover"], "Turnover.max_turnover")
                if max_turnover < 0:
                    raise ValueError("Turnover.max_turnover must be non-negative")
                cvx_constraints.append(cp.norm(w - w_curr, 1) <= max_turnover)
            else:
                raise ValueError(f"Unsupported constraint type '{ctype}'")

        # 3. Build Objective
        obj_expr = 0
        needs_risk_expression = any(_objective_needs_risk(objective) for objective in objectives_data)
        risk_expression = _build_risk_expression(risk_model_data, asset_ids, asset_id_to_idx) if needs_risk_expression else None

        for o in objectives_data:
            otype = o["type"]
            if otype == "MeanVariance":
                mu = _vector_from_mapping(
                    o["expected_returns"], asset_id_to_idx, n_assets, "MeanVariance.expected_returns", require_non_empty=True)
                gamma = _require_finite(o.get("risk_aversion", 1.0), "MeanVariance.risk_aversion")
                if gamma < 0:
                    raise ValueError("MeanVariance.risk_aversion must be non-negative")
                if gamma > 0 and risk_expression is None:
                    raise ValueError("MeanVariance requires a risk model when risk_aversion is positive")
                obj_expr += mu @ w
                if gamma > 0:
                    obj_expr -= gamma * risk_expression(w)

            elif otype == "MinimumVariance":
                if risk_expression is None:
                    raise ValueError("MinimumVariance requires a risk model")
                obj_expr -= risk_expression(w)

            elif otype == "MaximizeReturn":
                mu = _vector_from_mapping(
                    o["expected_returns"], asset_id_to_idx, n_assets, "MaximizeReturn.expected_returns", require_non_empty=True)
                obj_expr += mu @ w

            elif otype == "TrackingError":
                if risk_expression is None:
                    raise ValueError("TrackingError requires a risk model")
                benchmark = _vector_from_mapping(
                    o["benchmark_weights"], asset_id_to_idx, n_assets, "TrackingError.benchmark_weights", require_non_empty=True)
                obj_expr -= risk_expression(w - benchmark)

            else:
                raise ValueError(f"Unsupported objective type '{otype}'")

        # 4. Solve
        prob = cp.Problem(cp.Maximize(obj_expr), cvx_constraints)

        solver = str(config.get("solver", "OSQP")).upper()
        verbose = config.get("verbose", False)
        solver_constant = getattr(cp, solver, None)
        if solver_constant is None:
            raise ValueError(f"CVXPY solver '{solver}' is not available")

        start_time = time.time()
        prob.solve(solver=solver_constant, verbose=verbose, **_solver_options(config, solver))
        end_time = time.time()

        # 5. Format Result
        result = {"status": prob.status, "objective_value": prob.value, "solve_time_ms": (end_time - start_time) * 1000}

        if prob.status in {cp.OPTIMAL, cp.OPTIMAL_INACCURATE} and w.value is not None:
            opt_vals = {}
            for i, aid in enumerate(asset_ids):
                opt_vals[aid] = float(w.value[i])
            result["optimal_values"] = opt_vals

        return result

    except Exception as e:
        return {"status": "error", "message": str(e)}


if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: cvxpy_worker.py <request_file> <response_file>")
        sys.exit(1)

    req_file = sys.argv[1]
    res_file = sys.argv[2]

    with open(req_file, "r") as f:
        request = json.load(f)

    result = solve_optimization(request)

    with open(res_file, "w") as f:
        json.dump(result, f, indent=4)
