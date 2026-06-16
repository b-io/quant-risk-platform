import json
import sys
import os
import time
import numpy as np
import cvxpy as cp


def solve_optimization(request):
    try:
        # 1. Parse request
        variables_data = request.get("variables", [])
        objectives_data = request.get("objectives", [])
        constraints_data = request.get("constraints", [])
        risk_model_data = request.get("risk_model", {})
        config = request.get("config", {})

        # Map asset IDs to cvxpy variables
        asset_ids = [v["id"] for v in variables_data]
        n_assets = len(asset_ids)
        asset_id_to_idx = {aid: i for i, aid in enumerate(asset_ids)}

        w = cp.Variable(n_assets, name="weights")

        # 2. Build Constraints
        cvx_constraints = []

        # Variable bounds
        for v in variables_data:
            idx = asset_id_to_idx[v["id"]]
            if "lb" in v and v["lb"] > -1e10:
                cvx_constraints.append(w[idx] >= v["lb"])
            if "ub" in v and v["ub"] < 1e10:
                cvx_constraints.append(w[idx] <= v["ub"])

        for c in constraints_data:
            ctype = c["type"]
            if ctype == "LinearEquality":
                coeffs = np.zeros(n_assets)
                for aid, val in c["coefficients"].items():
                    if aid in asset_id_to_idx:
                        coeffs[asset_id_to_idx[aid]] = val
                cvx_constraints.append(coeffs @ w == c["target"])

            elif ctype == "LinearInequality":
                coeffs = np.zeros(n_assets)
                for aid, val in c["coefficients"].items():
                    if aid in asset_id_to_idx:
                        coeffs[asset_id_to_idx[aid]] = val
                if "lb" in c:
                    cvx_constraints.append(coeffs @ w >= c["lb"])
                if "ub" in c:
                    cvx_constraints.append(coeffs @ w <= c["ub"])

            elif ctype == "Turnover":
                w_curr = np.zeros(n_assets)
                for aid, val in c["current_weights"].items():
                    if aid in asset_id_to_idx:
                        w_curr[asset_id_to_idx[aid]] = val
                cvx_constraints.append(cp.norm(w - w_curr, 1) <= c["max_turnover"])

        # 3. Build Objective
        obj_expr = 0

        # Risk part (Variance)
        if risk_model_data:
            rtype = risk_model_data["type"]
            if rtype == "FullCovariance":
                sigma = np.array(risk_model_data["covariance"])
                # We need to ensure we use the same ordering as w
                rm_asset_ids = risk_model_data["asset_ids"]
                rm_id_to_idx = {aid: i for i, aid in enumerate(rm_asset_ids)}

                # Permute sigma if needed
                P = np.zeros((n_assets, len(rm_asset_ids)))
                for i, aid in enumerate(asset_ids):
                    if aid in rm_id_to_idx:
                        P[i, rm_id_to_idx[aid]] = 1.0

                sigma_aligned = P @ sigma @ P.T
                risk_expr = 0.5 * cp.quad_form(w, sigma_aligned)

            elif rtype == "FactorRisk":
                # Sigma = BFB^T + D
                B = np.array(risk_model_data["exposures"])  # N x K
                F = np.array(risk_model_data["factor_covariance"])  # K x K
                D_map = risk_model_data["specific_risk"]  # Map asset_id -> var

                D_diag = np.zeros(n_assets)
                for aid, val in D_map.items():
                    if aid in asset_id_to_idx:
                        D_diag[asset_id_to_idx[aid]] = val

                # Aligned B
                rm_asset_ids = risk_model_data["asset_ids"]
                rm_id_to_idx = {aid: i for i, aid in enumerate(rm_asset_ids)}
                B_aligned = np.zeros((n_assets, B.shape[1]))
                for i, aid in enumerate(asset_ids):
                    if aid in rm_id_to_idx:
                        B_aligned[i, :] = B[rm_id_to_idx[aid], :]

                # risk = 0.5 * (w^T B F B^T w + w^T D w)
                # Let y = B^T w (factor exposures)
                y = B_aligned.T @ w
                risk_expr = 0.5 * (cp.quad_form(y, F) + cp.sum(cp.multiply(D_diag, cp.square(w))))
            else:
                risk_expr = 0
        else:
            risk_expr = 0

        for o in objectives_data:
            otype = o["type"]
            if otype == "MeanVariance":
                mu = np.zeros(n_assets)
                for aid, val in o["expected_returns"].items():
                    if aid in asset_id_to_idx:
                        mu[asset_id_to_idx[aid]] = val
                gamma = o.get("risk_aversion", 1.0)
                obj_expr += mu @ w - gamma * risk_expr

            elif otype == "MinimumVariance":
                obj_expr -= risk_expr  # minimizing variance means maximizing -variance

            elif otype == "MaximizeReturn":
                mu = np.zeros(n_assets)
                for aid, val in o["expected_returns"].items():
                    if aid in asset_id_to_idx:
                        mu[asset_id_to_idx[aid]] = val
                obj_expr += mu @ w

        # 4. Solve
        prob = cp.Problem(cp.Maximize(obj_expr), cvx_constraints)

        solver = config.get("solver", "OSQP")
        verbose = config.get("verbose", False)

        start_time = time.time()
        prob.solve(solver=getattr(cp, solver, cp.OSQP), verbose=verbose)
        end_time = time.time()

        # 5. Format Result
        result = {"status": prob.status, "objective_value": prob.value, "solve_time_ms": (end_time - start_time) * 1000}

        if prob.status == "optimal":
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
