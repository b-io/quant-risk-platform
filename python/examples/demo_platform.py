import argparse
import importlib.machinery
import importlib.util
import json
import math
import os
import sys
import webbrowser
from collections import defaultdict
from pathlib import Path

from demo_dashboard import create_plotly_dashboard


script_path = Path(__file__).resolve()
project_root = script_path.parents[2]


def configure_module_path():
    build_path = os.environ.get("QRP_PYTHON_PATH")
    if not build_path:
        candidates = [
            project_root / "build" / "Release-Python" / "python" / "Release",
            project_root / "build" / "Release-Python" / "python",
            project_root / "build" / "Release-Python-Shared" / "python" / "Release",
            project_root / "build" / "Release-Python-Shared" / "python",
        ]
        incompatible_extensions = []
        for candidate in candidates:
            extension_files = list(candidate.glob("quant_risk_platform*.pyd")) + list(candidate.glob("quant_risk_platform*.so"))
            compatible_files = [
                path for path in extension_files
                if any(path.name.endswith(suffix) for suffix in importlib.machinery.EXTENSION_SUFFIXES)
            ]
            if compatible_files:
                build_path = str(candidate)
                break
            incompatible_extensions.extend(extension_files)
        if not build_path and incompatible_extensions:
            suffixes = ", ".join(importlib.machinery.EXTENSION_SUFFIXES)
            found = "\n  ".join(str(path) for path in incompatible_extensions)
            print("Warning: found compiled quant_risk_platform extensions, but none match this Python runtime.")
            print(f"Current Python: {sys.executable}")
            print(f"Compatible extension suffixes: {suffixes}")
            print(f"Found extensions:\n  {found}")

    if build_path and os.path.exists(build_path):
        sys.path.append(build_path)
        if sys.platform == "win32" and hasattr(os, "add_dll_directory"):
            os.add_dll_directory(build_path)
    else:
        print(f"Warning: QRP Python module path not found: {build_path}")


configure_module_path()

try:
    import quant_risk_platform as qrp
except ImportError as exc:
    print(f"Error: could not import 'quant_risk_platform'. Details: {exc}")
    sys.exit(1)


FACTOR_TYPE = {
    "BasisSpread": qrp.FactorType.BasisSpread,
    "CommodityForward": qrp.FactorType.CommodityForward,
    "CreditSpread": qrp.FactorType.CreditSpread,
    "Custom": qrp.FactorType.Custom,
    "EquitySpot": qrp.FactorType.EquitySpot,
    "FXSpot": qrp.FactorType.FXSpot,
    "HazardRate": qrp.FactorType.HazardRate,
    "RateForward": qrp.FactorType.RateForward,
    "RateZero": qrp.FactorType.RateZero,
    "Volatility": qrp.FactorType.Volatility,
}

CURRENCY = {
    "CHF": qrp.Currency.CHF,
    "EUR": qrp.Currency.EUR,
    "GBP": qrp.Currency.GBP,
    "JPY": qrp.Currency.JPY,
    "UNKNOWN": qrp.Currency.UNKNOWN,
    "USD": qrp.Currency.USD,
}

SHOCK_MEASURE = {
    "Absolute": qrp.ShockMeasure.Absolute,
    "BasisPoints": qrp.ShockMeasure.BasisPoints,
    "LogReturn": qrp.ShockMeasure.LogReturn,
    "Relative": qrp.ShockMeasure.Relative,
    "VolPoints": qrp.ShockMeasure.VolPoints,
}

REQUIRED_STRESS_SCENARIOS = {
    "CROSS_ASSET_REPRICING",
    "EQUITY_CRASH_VOL_SPIKE",
    "EQUITY_RALLY_VOL_CRUSH",
    "EUR_RATES_UP_50BP",
    "GLOBAL_RISK_OFF",
    "GLOBAL_RISK_ON",
    "USD_CURVE_BEAR_FLATTENER",
    "USD_CURVE_BEAR_STEEPENER",
    "USD_CURVE_BULL_STEEPENER",
    "USD_LIBOR_OIS_WIDENING",
    "USD_RATES_PARALLEL_DOWN_100BP",
    "USD_RATES_PARALLEL_UP_100BP",
    "USD_STRENGTH",
    "USD_WEAKNESS",
    "VOL_CRUSH",
    "VOL_SPIKE",
}

QUOTE_INSTRUMENT_CATEGORY = {
    # Commodity
    "CommodityForward": "Commodity",
    "CommodityFuture": "Commodity",
    "CommoditySpot": "Commodity",
    "CommodityVol": "Commodity",
    "ConvenienceYield": "Commodity",
    # Credit
    "Bond": "Credit",
    "BondPrice": "Credit",
    "BondSpread": "Credit",
    "CDS": "Credit",
    "CreditIndex": "Credit",
    "CreditSpread": "Credit",
    "HazardRate": "Credit",
    "RecoveryRate": "Credit",
    # Equity
    "BorrowRate": "Equity",
    "DividendYield": "Equity",
    "EquitySpot": "Equity",
    "EquityVol": "Equity",
    # FX
    "FXForward": "FX",
    "FXForwardPoint": "FX",
    "FXSpot": "FX",
    "FXVol": "FX",
    # Generic
    "Future": "Generic",
    # Rates
    "CapFloorVol": "Rates",
    "Deposit": "Rates",
    "FRA": "Rates",
    "InterestRateFuture": "Rates",
    "IRS": "Rates",
    "OIS": "Rates",
    "SwaptionVol": "Rates",
}


def section(title):
    print("\n" + "=" * 72)
    print(title)
    print("=" * 72)


def assert_close(label, actual, expected, tolerance=1e-10):
    if abs(actual - expected) > tolerance:
        raise AssertionError(f"{label}: expected {expected:.12f}, got {actual:.12f}")


def enum_name(value):
    return getattr(value, "name", str(value).rsplit(".", 1)[-1])


def load_golden_expectations(path):
    with open(path, "r", encoding="utf-8") as handle:
        return json.load(handle)


def load_product_family_expectations(path):
    fixtures = []
    for fixture_path in sorted(path.glob("*_golden.json")):
        with open(fixture_path, "r", encoding="utf-8") as handle:
            fixture = json.load(handle)
        fixture["fixture_path"] = str(fixture_path)
        fixtures.append(fixture)
    if not fixtures:
        raise ValueError(f"No product-family golden fixtures found in {path}")
    return fixtures


def quote_values(market):
    return {quote.id: quote.value for quote in market.quotes}


def print_market_data_coverage(market, factors, bindings):
    section("1. Market Data Coverage")
    diagnostics = qrp.collect_market_snapshot_diagnostics(market)
    blocking = [item for item in diagnostics if qrp.is_blocking_market_data_diagnostic(item)]
    if blocking:
        for item in blocking:
            print(qrp.format_market_data_diagnostic(item))
        raise AssertionError("Market snapshot has blocking diagnostics")

    quote_ids = {quote.id for quote in market.quotes}
    factor_quote_ids = {quote_id for factor in factors for quote_id in factor.quote_ids}
    binding_quote_ids = {binding.quote_id for binding in bindings}
    curve_quote_ids = {quote_id for curve in market.curves for quote_id in curve.quote_ids}

    print(f"Snapshot:          {market.snapshot_id}")
    print(f"Market date:       {market.valuation_date}")
    print(f"Quotes/curves:     {len(market.quotes)} quotes / {len(market.curves)} curves")
    print(f"Diagnostics:       {len(diagnostics)} non-blocking")

    grouped = defaultdict(lambda: defaultdict(list))
    for quote in market.quotes:
        instrument = enum_name(quote.instrument_type)
        category = QUOTE_INSTRUMENT_CATEGORY.get(instrument, "Generic")
        grouped[category][instrument].append(quote.id)

    print("\nQuotes by category")
    for category in sorted(grouped):
        print(f"  {category}")
        for instrument in sorted(grouped[category]):
            ids = ", ".join(sorted(grouped[category][instrument]))
            print(f"    {instrument:<20} {ids}")

    print("\nRates curve build report")
    for result in sorted(qrp.build_rates_market_report(market), key=lambda item: (enum_name(item.id.currency), item.id.family)):
        status = "built" if result.built else "skipped"
        curve_id = f"{enum_name(result.id.currency)}:{result.id.family}"
        print(f"  {curve_id:<16} {status:<7} {enum_name(result.purpose):<12} {result.status_message}")

    missing_binding_quotes = sorted(binding_quote_ids - quote_ids)
    missing_factor_quotes = sorted(factor_quote_ids - quote_ids)
    if missing_binding_quotes or missing_factor_quotes:
        missing = sorted(set(missing_binding_quotes) | set(missing_factor_quotes))
        raise AssertionError(f"Scenario factors reference missing market quotes: {', '.join(missing)}")

    exercised_quote_ids = curve_quote_ids | binding_quote_ids
    catalog_only_quote_ids = sorted(quote_ids - exercised_quote_ids)
    if catalog_only_quote_ids:
        print("\nCatalog-only quotes")
        for quote_id in catalog_only_quote_ids:
            print(f"  {quote_id}")


def build_factor(item):
    factor = qrp.FactorDefinition()
    factor.factor_id = item["factor_id"]
    if not qrp.is_canonical_factor_id(factor.factor_id):
        raise ValueError(f"Non-canonical factor_id: {factor.factor_id}")
    factor.factor_type = FACTOR_TYPE[item["factor_type"]]
    factor.shock_measure = SHOCK_MEASURE[item["shock_measure"]]
    factor.currency = CURRENCY[item.get("currency", "UNKNOWN")]
    factor.curve_id = item.get("curve_id", "")
    factor.tenor = item.get("tenor", "")
    factor.quote_ids = item.get("quote_ids", [])
    factor.description = item.get("description", "")
    return factor


def build_binding(item):
    binding = qrp.FactorBinding()
    binding.factor_id = item["factor_id"]
    binding.quote_id = item["quote_id"]
    binding.shock_measure = SHOCK_MEASURE[item["shock_measure"]]
    binding.weight = item.get("weight", 1.0)
    binding.transform = item.get("transform", "")
    binding.selector_json = item.get("selector_json", "")
    return binding


def build_scenario(name, item):
    scenario = qrp.ScenarioDefinition()
    scenario.name = name
    scenario.factor_shocks = dict(item["factor_shocks"])
    return scenario


def load_factor_scenario_set(path):
    with open(path, "r", encoding="utf-8") as handle:
        payload = json.load(handle)

    factors = [build_factor(item) for item in payload["factors"]]
    factor_ids = {factor.factor_id for factor in factors}
    bindings = [build_binding(item) for item in payload["bindings"]]
    scenarios = [build_scenario(name, item) for name, item in payload["scenarios"].items()]
    missing_scenarios = REQUIRED_STRESS_SCENARIOS - set(payload["scenarios"])
    if missing_scenarios:
        missing = ", ".join(sorted(missing_scenarios))
        raise ValueError(f"Missing required stress scenarios: {missing}")
    for binding in bindings:
        if binding.factor_id not in factor_ids:
            raise ValueError(f"Binding references undefined factor_id: {binding.factor_id}")
    for scenario in scenarios:
        for factor_id in scenario.factor_shocks:
            if factor_id not in factor_ids:
                raise ValueError(f"Scenario references undefined factor_id: {factor_id}")
    return payload, factors, bindings, scenarios


def print_portfolio_valuation(portfolio, market):
    section("2. Portfolio Valuation")
    valuation_results = qrp.price_portfolio(portfolio, market)
    total_npv = sum(result.npv for result in valuation_results)
    for result in valuation_results:
        status = result.tags.get("status", "unknown")
        product = result.tags.get("product_type", "unknown")
        print(f"{result.trade_id:<28} {result.npv:>15,.2f} {result.currency:<3} {status:<20} {product}")
    print("-" * 52)
    print(f"{'TOTAL':<28} {total_npv:>15,.2f} USD")
    if not valuation_results:
        raise AssertionError("Expected at least one valuation result")
    return valuation_results, total_npv


def run_factor_resolution_checks(market, factors, bindings, scenarios):
    section("3. Factor Shock Resolution")
    base = quote_values(market)
    scenarios_by_name = {scenario.name: scenario for scenario in scenarios}

    scenario = scenarios_by_name["GLOBAL_RISK_OFF"]
    shocked = dict(qrp.resolve_quote_values(scenario, factors, bindings, market))

    expected = {
        "USD_OIS_1Y": base["USD_OIS_1Y"] - 35.0 / 10000.0,
        "USD_OIS_2Y": base["USD_OIS_2Y"] - 35.0 / 10000.0,
        "USD_OIS_5Y": base["USD_OIS_5Y"] - 0.8 * 35.0 / 10000.0,
        "USD_OIS_10Y": base["USD_OIS_10Y"] - 0.6 * 35.0 / 10000.0,
        "EURUSD": base["EURUSD"] * math.exp(scenario.factor_shocks["RF:FX:EURUSD:SPOT"]),
        "AAPL": base["AAPL"] * (1.0 + scenario.factor_shocks["RF:EQ:AAPL:SPOT"]),
        "USD_CAP_VOL_1Y": base["USD_CAP_VOL_1Y"] + scenario.factor_shocks["RF:RATESVOL:USD:CAP:1Y:ATM"] / 100.0,
    }

    for quote_id, expected_value in expected.items():
        assert_close(quote_id, shocked[quote_id], expected_value)
        print(f"{quote_id:<18} {base[quote_id]:>10.6f} -> {shocked[quote_id]:>10.6f}")

    print("Factor resolution checks passed.")


def run_stress(portfolio, market, factors, bindings, scenarios):
    section("4. Historical Stress")
    stress_results = qrp.run_historical_stress(portfolio, market, scenarios, factors, bindings)
    if len(stress_results) != len(scenarios):
        raise AssertionError("Stress result count does not match scenario count")

    for result in stress_results:
        print(f"{result.scenario_name:<28} total P&L = {result.total_pnl:>15,.2f}")
        for trade_id, pnl in result.trade_pnls.items():
            if abs(pnl) > 1e-8:
                print(f"  {trade_id:<26} {pnl:>15,.2f}")
    return stress_results


def run_risk(portfolio, market, factors, bindings):
    section("5. Risk Sensitivities")
    risk_results = qrp.compute_risk(portfolio, market, factors, bindings)
    if not risk_results:
        raise AssertionError("Expected risk results")

    for result in risk_results:
        print(f"{result.trade_id:<28} PV01={result.pv01:>12,.2f} CS01={result.cs01:>12,.2f}")
        for node, value in result.bucketed_risk.items():
            if abs(value) > 1e-8:
                print(f"  {node:<24} {value:>12,.2f}")
    return risk_results


def run_pnl_explain(portfolio, market_path):
    section("6. P&L Explain")
    previous_market = qrp.load_market(str(market_path))
    current_market = qrp.load_market(str(market_path))
    current_market.valuation_date = "2026-03-25"

    adjusted_quotes = []
    for quote in current_market.quotes:
        if quote.id.startswith("USD_OIS") or quote.id.startswith("USD_LIBOR"):
            quote.value += 0.0005
        elif quote.id == "EURUSD":
            quote.value *= 1.01
        elif quote.id == "AAPL":
            quote.value *= 0.99
        adjusted_quotes.append(quote)
    current_market.quotes = adjusted_quotes

    pnl_results = qrp.explain_pnl(portfolio, previous_market, current_market)
    if not pnl_results:
        raise AssertionError("Expected P&L explain results")

    for result in pnl_results:
        print(
            f"{result.trade_id:<28} total={result.total_pnl:>12,.2f} "
            f"carry={result.carry_pnl:>12,.2f} market={result.market_move_pnl:>12,.2f}"
        )
    return pnl_results


def factor_variance(factor):
    if factor.factor_type in (qrp.FactorType.RateForward, qrp.FactorType.RateZero):
        return 1e-8
    if factor.factor_type == qrp.FactorType.FXSpot:
        return 1e-4
    if factor.factor_type == qrp.FactorType.EquitySpot:
        return 4e-4
    if factor.factor_type == qrp.FactorType.Volatility:
        return 0.0625
    return 0.0


def build_covariance(factors):
    covariance = qrp.Matrix(len(factors), len(factors), 0.0)
    for i, factor_i in enumerate(factors):
        for j, factor_j in enumerate(factors):
            if i == j:
                covariance[i, j] = factor_variance(factor_i)
            else:
                covariance[i, j] = 0.15 * math.sqrt(factor_variance(factor_i) * factor_variance(factor_j))
    return covariance


def print_traces(traces, mode):
    for trace in traces[:2]:
        print(f"\nPath {trace.path_index + 1}")
        if mode == qrp.MonteCarloMode.AgedHorizonRevaluation:
            print(f"  Dates:        {trace.valuation_date_before} -> {trace.valuation_date_after}")
            print(f"  Frozen aged:  {trace.portfolio_value_frozen_aged:,.2f}")
            print(f"  Shocked aged: {trace.portfolio_value_after:,.2f}")
            print(f"  Total P&L:    {trace.total_pnl:,.2f}")
        else:
            print(f"  Value:        {trace.portfolio_value_before:,.2f} -> {trace.portfolio_value_after:,.2f}")
            print(f"  Path P&L:     {trace.path_pnl:,.2f}")

        print("  Factor shocks:")
        for factor_id, shock in trace.factor_shocks.items():
            print(f"    {factor_id:<28} {shock:+.8f}")


def run_monte_carlo(portfolio, market, factors, bindings):
    section("7. Monte Carlo")
    mc_factors = [factor for factor in factors if factor.factor_id != "RF:RATES:USD:OIS:PARALLEL"]
    covariance = build_covariance(mc_factors)

    cases = [
        ("HorizonShockOnly", qrp.MonteCarloMode.HorizonShockOnly, 1.0),
        ("AgedHorizonRevaluation", qrp.MonteCarloMode.AgedHorizonRevaluation, 10.0),
    ]

    results = []
    for label, mode, horizon_days in cases:
        config = qrp.MonteCarloConfig()
        config.horizon_days = horizon_days
        config.mode = mode
        config.num_paths = 100
        config.seed = 42

        result = qrp.run_simulation(portfolio, market, mc_factors, bindings, covariance, config)
        if not result.portfolio_pnls:
            raise AssertionError(f"{label} produced no P&L paths")

        mean_pnl = sum(result.portfolio_pnls) / len(result.portfolio_pnls)
        print(
            f"{label:<24} horizon={horizon_days:>4.0f}d "
            f"VaR95={result.var_95:>12,.2f} ES95={result.expected_shortfall_95:>12,.2f} "
            f"mean={mean_pnl:>12,.2f}"
        )
        print_traces(result.traces, mode)
        results.append((label, result, mean_pnl))

    return results


def validate_golden_outputs(golden, valuation_results, total_npv, stress_results, risk_results, pnl_results, mc_results):
    section("Golden Regression Checks")

    valuation = golden["valuation"]
    valuation_tolerance = valuation.get("tolerance", 1.0)
    assert_close("valuation.total_npv", total_npv, valuation["total_npv"], valuation_tolerance)
    valuation_by_trade = {result.trade_id: result for result in valuation_results}
    for trade_id, expected in valuation["trades"].items():
        actual = valuation_by_trade[trade_id]
        assert_close(f"valuation.{trade_id}.npv", actual.npv, expected["npv"], valuation_tolerance)
        if actual.tags.get("product_type") != expected["product_type"]:
            raise AssertionError(f"valuation.{trade_id}.product_type drifted")
        if actual.tags.get("status") != expected["support_status"]:
            raise AssertionError(f"valuation.{trade_id}.support_status drifted")

    stress = golden["stress"]
    stress_tolerance = stress.get("tolerance", 1.0)
    stress_by_name = {result.scenario_name: result for result in stress_results}
    for scenario_name, expected in stress.items():
        if scenario_name == "tolerance":
            continue
        actual = stress_by_name[scenario_name]
        assert_close(f"stress.{scenario_name}.total_pnl", actual.total_pnl, expected["total_pnl"], stress_tolerance)
        for trade_id, expected_pnl in expected["trade_pnls"].items():
            assert_close(
                f"stress.{scenario_name}.{trade_id}",
                actual.trade_pnls[trade_id],
                expected_pnl,
                stress_tolerance)

    risk = golden["risk"]
    risk_tolerance = risk.get("tolerance", 1.0)
    risk_by_trade = {result.trade_id: result for result in risk_results}
    for trade_id, expected in risk.items():
        if trade_id == "tolerance":
            continue
        actual = risk_by_trade[trade_id]
        assert_close(f"risk.{trade_id}.pv01", actual.pv01, expected["pv01"], risk_tolerance)
        assert_close(f"risk.{trade_id}.cs01", actual.cs01, expected["cs01"], risk_tolerance)

    pnl = golden["pnl_explain"]
    pnl_tolerance = pnl.get("tolerance", 1.0)
    pnl_by_trade = {result.trade_id: result for result in pnl_results}
    for trade_id, expected in pnl.items():
        if trade_id == "tolerance":
            continue
        actual = pnl_by_trade[trade_id]
        assert_close(f"pnl.{trade_id}.carry_pnl", actual.carry_pnl, expected["carry_pnl"], pnl_tolerance)
        assert_close(f"pnl.{trade_id}.market_move_pnl", actual.market_move_pnl, expected["market_move_pnl"], pnl_tolerance)
        assert_close(f"pnl.{trade_id}.total_pnl", actual.total_pnl, expected["total_pnl"], pnl_tolerance)

    monte_carlo = golden["monte_carlo"]
    mc_by_label = {label: (result, mean_pnl) for label, result, mean_pnl in mc_results}
    for label, expected in monte_carlo.items():
        actual, mean_pnl = mc_by_label[label]
        tolerance = expected.get("tolerance", 50.0)
        assert_close(f"mc.{label}.expected_shortfall_95", actual.expected_shortfall_95, expected["expected_shortfall_95"], tolerance)
        assert_close(f"mc.{label}.mean_pnl", mean_pnl, expected["mean_pnl"], tolerance)
        assert_close(f"mc.{label}.var_95", actual.var_95, expected["var_95"], tolerance)

    print("Golden regression checks passed.")


def validate_product_family_outputs(product_families, valuation_results, stress_results, risk_results, pnl_results):
    section("Product-Family Golden Checks")

    pnl_by_trade = {result.trade_id: result for result in pnl_results}
    risk_by_trade = {result.trade_id: result for result in risk_results}
    stress_by_name = {result.scenario_name: result for result in stress_results}
    valuation_by_trade = {result.trade_id: result for result in valuation_results}

    for family in product_families:
        asset_class = family["asset_class"]
        fixture_name = Path(family["fixture_path"]).name
        tolerances = family.get("tolerances", {})

        for trade_id, expected in family["trades"].items():
            valuation_result = valuation_by_trade[trade_id]
            risk_result = risk_by_trade[trade_id]
            pnl_result = pnl_by_trade[trade_id]

            if valuation_result.tags.get("asset_class") != asset_class:
                raise AssertionError(f"{fixture_name}.{trade_id}.asset_class drifted")
            if valuation_result.tags.get("product_type") != expected["product_type"]:
                raise AssertionError(f"{fixture_name}.{trade_id}.product_type drifted")
            if valuation_result.tags.get("status") != expected["support_status"]:
                raise AssertionError(f"{fixture_name}.{trade_id}.support_status drifted")

            assert_close(
                f"{fixture_name}.{trade_id}.valuation.npv",
                valuation_result.npv,
                expected["valuation"]["npv"],
                tolerances.get("valuation", 1.0))
            assert_close(
                f"{fixture_name}.{trade_id}.risk.cs01",
                risk_result.cs01,
                expected["risk"]["cs01"],
                tolerances.get("risk", 1.0))
            assert_close(
                f"{fixture_name}.{trade_id}.risk.pv01",
                risk_result.pv01,
                expected["risk"]["pv01"],
                tolerances.get("risk", 1.0))
            assert_close(
                f"{fixture_name}.{trade_id}.pnl.carry_pnl",
                pnl_result.carry_pnl,
                expected["pnl_explain"]["carry_pnl"],
                tolerances.get("pnl_explain", 1.0))
            assert_close(
                f"{fixture_name}.{trade_id}.pnl.market_move_pnl",
                pnl_result.market_move_pnl,
                expected["pnl_explain"]["market_move_pnl"],
                tolerances.get("pnl_explain", 1.0))
            assert_close(
                f"{fixture_name}.{trade_id}.pnl.total_pnl",
                pnl_result.total_pnl,
                expected["pnl_explain"]["total_pnl"],
                tolerances.get("pnl_explain", 1.0))

            for scenario_name, expected_pnl in expected.get("stress", {}).items():
                stress_result = stress_by_name[scenario_name]
                assert_close(
                    f"{fixture_name}.{trade_id}.stress.{scenario_name}",
                    stress_result.trade_pnls[trade_id],
                    expected_pnl,
                    tolerances.get("stress", 1.0))

        print(f"{asset_class:<8} fixture passed: {fixture_name}")


def run_optional_cvxpy_worker_check():
    section("8. Optional CVXPY Worker Check")
    worker_path = project_root / "python" / "qrp" / "optimization" / "cvxpy_worker.py"
    spec = importlib.util.spec_from_file_location("qrp_cvxpy_worker", worker_path)
    if spec is None or spec.loader is None:
        print(f"Skipped: could not load CVXPY worker from {worker_path}.")
        return None

    worker = importlib.util.module_from_spec(spec)

    try:
        spec.loader.exec_module(worker)
    except ImportError as exc:
        print(f"Skipped: CVXPY worker dependencies are not available in this Python environment ({exc}).")
        print("Install the optional optimization dependencies with:")
        print("  uv sync --extra optimization")
        print("Then run the demo with the uv-managed interpreter:")
        print("  uv run python python/examples/demo_platform.py")
        return None

    request = {
        "name": "Demo maximum-return problem",
        "variables": [
            {"id": "AAPL", "lb": 0.0, "ub": 1.0, "integer": False},
            {"id": "MSFT", "lb": 0.0, "ub": 1.0, "integer": False},
        ],
        "objectives": [
            {"type": "MaximizeReturn", "expected_returns": {"AAPL": 0.10, "MSFT": 0.08}}
        ],
        "constraints": [
            {"type": "LinearEquality", "coefficients": {"AAPL": 1.0, "MSFT": 1.0}, "target": 1.0}
        ],
        "config": {"solver": "OSQP", "tolerance": 1e-8, "max_iterations": 10000, "verbose": False},
    }

    result = worker.solve_optimization(request)
    print(json.dumps(result, indent=2, sort_keys=True))
    if result.get("status") not in {"optimal", "optimal_inaccurate"}:
        raise AssertionError(f"CVXPY worker check failed: {result}")
    return result


def parse_args():
    parser = argparse.ArgumentParser(
        description="Run the Quant Risk Platform Python demo.")
    parser.add_argument(
        "--dashboard",
        action="store_true",
        help="Create reports/demo_risk_dashboard.html and open it in the default web browser.")
    return parser.parse_args()


def run_demo(dashboard=False):
    section("Quant Risk Platform Python Demo")
    market_path = project_root / "data" / "market" / "demo_market.json"
    portfolio_path = project_root / "data" / "portfolios" / "demo_portfolio.json"
    golden_path = project_root / "data" / "regression" / "demo_golden.json"
    product_family_golden_path = project_root / "data" / "regression" / "product_families"
    scenario_path = project_root / "data" / "scenarios" / "demo_scenarios.json"

    golden = load_golden_expectations(golden_path)
    product_families = load_product_family_expectations(product_family_golden_path)
    market = qrp.load_market(str(market_path))
    portfolio = qrp.load_portfolio(str(portfolio_path))
    scenario_payload, factors, bindings, scenarios = load_factor_scenario_set(scenario_path)

    print(f"Market date:       {market.valuation_date}")
    print(f"Portfolio:         {portfolio.portfolio_id} ({len(portfolio.trades)} trades)")
    print(f"Scenario set:      {scenario_payload['scenario_set_id']} ({len(scenarios)} scenarios)")
    print(f"Factors/bindings:  {len(factors)} factors / {len(bindings)} bindings")

    print_market_data_coverage(market, factors, bindings)
    valuation_results, total_npv = print_portfolio_valuation(portfolio, market)
    run_factor_resolution_checks(market, factors, bindings, scenarios)
    stress_results = run_stress(portfolio, market, factors, bindings, scenarios)
    risk_results = run_risk(portfolio, market, factors, bindings)
    pnl_results = run_pnl_explain(portfolio, market_path)
    mc_results = run_monte_carlo(portfolio, market, factors, bindings)
    validate_golden_outputs(golden, valuation_results, total_npv, stress_results, risk_results, pnl_results, mc_results)
    validate_product_family_outputs(product_families, valuation_results, stress_results, risk_results, pnl_results)
    optimization_result = run_optional_cvxpy_worker_check()
    if dashboard:
        dashboard_path = create_plotly_dashboard(
            portfolio=portfolio,
            valuation_results=valuation_results,
            total_npv=total_npv,
            stress_results=stress_results,
            risk_results=risk_results,
            pnl_results=pnl_results,
            mc_results=mc_results,
            factors=factors,
            market_valuation_date=market.valuation_date,
            scenarios=scenarios,
            output_path=project_root / "reports" / "demo_risk_dashboard.html",
            optimization_result=optimization_result,
        )
        if dashboard_path:
            print(f"\nDashboard written to: {dashboard_path}")
            webbrowser.open(dashboard_path.resolve().as_uri())
            print("Dashboard opened in your default web browser.")
        else:
            print("Dashboard was not generated, so there is no webpage to open.")

    section("Demo completed successfully")


if __name__ == "__main__":
    args = parse_args()
    run_demo(dashboard=args.dashboard)
