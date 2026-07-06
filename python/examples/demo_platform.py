"""Runs the end-to-end platform demo, golden checks, and optional dashboard export."""

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
            project_root / "build" / "dev" / "python" / "RelWithDebInfo",
            project_root / "build" / "dev" / "python",
            project_root / "build" / "dev-shared" / "python" / "RelWithDebInfo",
            project_root / "build" / "dev-shared" / "python",
            project_root / "build" / "release" / "python" / "Release",
            project_root / "build" / "release" / "python",
            project_root / "build" / "release-shared" / "python" / "Release",
            project_root / "build" / "release-shared" / "python",
        ]
        wrong_runtime_extensions = []
        for candidate in candidates:
            extension_files = list(candidate.glob("quant_risk_platform*.pyd")) + list(
                candidate.glob("quant_risk_platform*.so")
            )
            matching_runtime_files = [
                path
                for path in extension_files
                if any(
                    path.name.endswith(suffix)
                    for suffix in importlib.machinery.EXTENSION_SUFFIXES
                )
            ]
            if matching_runtime_files:
                build_path = str(candidate)
                break
            wrong_runtime_extensions.extend(extension_files)
        if not build_path and wrong_runtime_extensions:
            suffixes = ", ".join(importlib.machinery.EXTENSION_SUFFIXES)
            found = "\n  ".join(str(path) for path in wrong_runtime_extensions)
            print(
                "Warning: found compiled quant_risk_platform extensions, but none match this Python runtime."
            )
            print(f"Current Python: {sys.executable}")
            print(f"Expected extension suffixes: {suffixes}")
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
    "CommoditySpot": qrp.FactorType.CommoditySpot,
    "CreditRecovery": qrp.FactorType.CreditRecovery,
    "CreditSpread": qrp.FactorType.CreditSpread,
    "Custom": qrp.FactorType.Custom,
    "EquityBorrowRate": qrp.FactorType.EquityBorrowRate,
    "EquityDividendYield": qrp.FactorType.EquityDividendYield,
    "EquityForward": qrp.FactorType.EquityForward,
    "EquitySpot": qrp.FactorType.EquitySpot,
    "FXForwardPoint": qrp.FactorType.FXForwardPoint,
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
    "COMMODITY_ENERGY_SUPPLY_SHOCK",
    "CREDIT_RECESSION",
    "CROSS_ASSET_REPRICING",
    "EQUITY_CRASH_VOL_SPIKE",
    "EQUITY_INDEX_SELL_OFF",
    "EQUITY_RALLY_VOL_CRUSH",
    "EUR_CURVE_BEAR_STEEPENER",
    "EUR_RATES_UP_50BP",
    "FX_CARRY_UNWIND",
    "GLOBAL_RISK_OFF",
    "GLOBAL_RISK_ON",
    "USD_CURVE_BEAR_FLATTENER",
    "USD_CURVE_BEAR_STEEPENER",
    "USD_CURVE_BULL_STEEPENER",
    "USD_FUNDING_STRESS",
    "USD_LIBOR_OIS_WIDENING",
    "USD_RATES_PARALLEL_DOWN_100BP",
    "USD_RATES_PARALLEL_UP_100BP",
    "USD_STRENGTH",
    "USD_WEAKNESS",
    "VOL_CRUSH",
    "VOL_SPIKE",
}

DASHBOARD_PORTFOLIOS = [
    "liquidity_reserve_model_portfolio.json",
    "balanced_core_model_portfolio.json",
    "growth_global_macro_portfolio.json",
    "high_growth_equity_volatility_portfolio.json",
    "adventurous_commodity_volatility_portfolio.json",
]

MONTE_CARLO_CASES = [
    ("HorizonShockOnly", qrp.MonteCarloMode.HorizonShockOnly, 1.0),
    ("AgedHorizonRevaluation", qrp.MonteCarloMode.AgedHorizonRevaluation, 10.0),
]

DASHBOARD_PORTFOLIO_MONTE_CARLO_CASES = [MONTE_CARLO_CASES[0]]
ASSET_COLUMN_WIDTH = 10
PRODUCT_COLUMN_WIDTH = 34
POSITION_COLUMN_WIDTH = 34
POSITION_RULE_WIDTH = 120


def sort_text(value):
    return str(value or "").casefold()


def trade_sort_tuple(asset_class, product_type, trade_id):
    return (sort_text(asset_class), sort_text(product_type), sort_text(trade_id))


def portfolio_trade_sort_map(portfolio, valuation_results=None):
    sort_map = {
        trade.id: trade_sort_tuple(trade.asset_class, trade.type, trade.id)
        for trade in portfolio.trades
    }
    for result in valuation_results or []:
        sort_map[result.trade_id] = trade_sort_tuple(
            result.tags.get("asset_class", ""),
            result.tags.get("product_type", ""),
            result.trade_id,
        )
    return sort_map


def portfolio_trade_metadata_map(portfolio, valuation_results=None):
    metadata = {
        trade.id: {
            "asset_class": trade.asset_class,
            "product_type": trade.type,
            "trade_id": trade.id,
        }
        for trade in portfolio.trades
    }
    for result in valuation_results or []:
        metadata[result.trade_id] = {
            "asset_class": result.tags.get("asset_class", ""),
            "product_type": result.tags.get("product_type", ""),
            "trade_id": result.trade_id,
        }
    return metadata


def result_sort_key(result, sort_map):
    return sort_map.get(result.trade_id, trade_sort_tuple("", "", result.trade_id))


def trade_id_sort_key(trade_id, sort_map):
    return sort_map.get(trade_id, trade_sort_tuple("", "", trade_id))


def trade_metadata(metadata_by_trade, trade_id):
    return metadata_by_trade.get(
        trade_id,
        {"asset_class": "", "product_type": "", "trade_id": trade_id},
    )


def position_header(*numeric_columns):
    labels = (
        f"{'Asset':<{ASSET_COLUMN_WIDTH}} "
        f"{'Product':<{PRODUCT_COLUMN_WIDTH}} "
        f"{'Position':<{POSITION_COLUMN_WIDTH}}"
    )
    numbers = " ".join(f"{label:>12}" for label in numeric_columns)
    return f"{labels} {numbers}" if numbers else labels


def position_prefix(metadata):
    return (
        f"{metadata['asset_class']:<{ASSET_COLUMN_WIDTH}} "
        f"{metadata['product_type']:<{PRODUCT_COLUMN_WIDTH}} "
        f"{metadata['trade_id']:<{POSITION_COLUMN_WIDTH}}"
    )


def trade_expectation_sort_key(trade_id, expectation, default_asset_class=""):
    return trade_sort_tuple(
        expectation.get("asset_class", default_asset_class),
        expectation.get("product_type", ""),
        trade_id,
    )


def ordered_trade_expectations(trades, default_asset_class=""):
    return {
        trade_id: trades[trade_id]
        for trade_id in sorted(
            trades,
            key=lambda item: trade_expectation_sort_key(
                item, trades[item], default_asset_class
            ),
        )
    }


def canonicalize_golden_order(golden):
    valuation_trades = golden.get("valuation", {}).get("trades", {})
    if not valuation_trades:
        return golden

    ordered_valuation = ordered_trade_expectations(valuation_trades)
    golden["valuation"]["trades"] = ordered_valuation
    sort_map = {
        trade_id: trade_expectation_sort_key(trade_id, expectation)
        for trade_id, expectation in ordered_valuation.items()
    }

    for section_name in ("risk", "pnl_explain"):
        section = golden.get(section_name, {})
        tolerance = section.get("tolerance")
        ordered_section = {
            trade_id: section[trade_id]
            for trade_id in sorted(
                (key for key in section if key != "tolerance"),
                key=lambda item: sort_map.get(item, trade_sort_tuple("", "", item)),
            )
        }
        if tolerance is not None:
            ordered_section["tolerance"] = tolerance
        golden[section_name] = ordered_section

    for scenario_name, scenario in golden.get("stress", {}).items():
        if scenario_name == "tolerance" or "trade_pnls" not in scenario:
            continue
        trade_pnls = scenario["trade_pnls"]
        scenario["trade_pnls"] = {
            trade_id: trade_pnls[trade_id]
            for trade_id in sorted(
                trade_pnls,
                key=lambda item: sort_map.get(item, trade_sort_tuple("", "", item)),
            )
        }
    return golden


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


def assert_key_sets_match(label, actual_keys, expected_keys):
    actual = set(actual_keys)
    expected = set(expected_keys)
    if actual == expected:
        return

    messages = []
    missing = sorted(expected - actual)
    extra = sorted(actual - expected)
    if missing:
        messages.append(f"missing {missing}")
    if extra:
        messages.append(f"extra {extra}")
    raise AssertionError(f"{label}: key set drifted ({'; '.join(messages)})")


def expected_regression_keys(section):
    return [key for key in section if key != "tolerance"]


def material_value_map(values, tolerance):
    return {
        str(key): float(value)
        for key, value in dict(values).items()
        if abs(float(value)) > tolerance
    }


def enum_name(value):
    return getattr(value, "name", str(value).rsplit(".", 1)[-1])


def load_golden_expectations(path):
    with open(path, "r", encoding="utf-8") as handle:
        return canonicalize_golden_order(json.load(handle))


def load_product_family_expectations(path):
    fixtures = []
    for fixture_path in sorted(path.glob("*_golden.json")):
        with open(fixture_path, "r", encoding="utf-8") as handle:
            fixture = json.load(handle)
        fixture["trades"] = ordered_trade_expectations(
            fixture["trades"], fixture.get("asset_class", "")
        )
        fixture["fixture_path"] = str(fixture_path)
        fixtures.append(fixture)
    if not fixtures:
        raise ValueError(f"No product-family golden fixtures found in {path}")
    return sorted(
        fixtures,
        key=lambda item: trade_sort_tuple(
            item.get("asset_class", ""), "", Path(item["fixture_path"]).name
        ),
    )


def quote_values(market):
    return {quote.id: quote.value for quote in market.quotes}


def print_market_data_coverage(market, factors, bindings):
    section("1. Market Data Coverage")
    diagnostics = qrp.collect_market_snapshot_diagnostics(market)
    blocking = [
        item for item in diagnostics if qrp.is_blocking_market_data_diagnostic(item)
    ]
    if blocking:
        for item in blocking:
            print(qrp.format_market_data_diagnostic(item))
        raise AssertionError("Market snapshot has blocking diagnostics")

    quote_ids = {quote.id for quote in market.quotes}
    factor_quote_ids = {quote_id for factor in factors for quote_id in factor.quote_ids}
    binding_quote_ids = {binding.quote_id for binding in bindings}
    curve_quote_ids = {
        quote_id for curve in market.curves for quote_id in curve.quote_ids
    }

    print(f"Snapshot:          {market.snapshot_id}")
    print(f"Market date:       {market.valuation_date}")
    print(
        f"Quotes/curves:     {len(market.quotes)} quotes / {len(market.curves)} curves"
    )
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
    for result in sorted(
        qrp.build_rates_market_report(market),
        key=lambda item: (enum_name(item.id.currency), item.id.family),
    ):
        status = "built" if result.built else "skipped"
        curve_id = f"{enum_name(result.id.currency)}:{result.id.family}"
        print(
            f"  {curve_id:<16} {status:<7} {enum_name(result.purpose):<12} {result.status_message}"
        )

    missing_binding_quotes = sorted(binding_quote_ids - quote_ids)
    missing_factor_quotes = sorted(factor_quote_ids - quote_ids)
    if missing_binding_quotes or missing_factor_quotes:
        missing = sorted(set(missing_binding_quotes) | set(missing_factor_quotes))
        raise AssertionError(
            f"Scenario factors reference missing market quotes: {', '.join(missing)}"
        )

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
    scenarios = [
        build_scenario(name, item) for name, item in payload["scenarios"].items()
    ]
    missing_scenarios = REQUIRED_STRESS_SCENARIOS - set(payload["scenarios"])
    if missing_scenarios:
        missing = ", ".join(sorted(missing_scenarios))
        raise ValueError(f"Missing required stress scenarios: {missing}")
    for binding in bindings:
        if binding.factor_id not in factor_ids:
            raise ValueError(
                f"Binding references undefined factor_id: {binding.factor_id}"
            )
    for scenario in scenarios:
        for factor_id in scenario.factor_shocks:
            if factor_id not in factor_ids:
                raise ValueError(
                    f"Scenario references undefined factor_id: {factor_id}"
                )
    return payload, factors, bindings, scenarios


def print_portfolio_valuation(portfolio, market):
    section("2. Portfolio Valuation")
    valuation_results = qrp.price_portfolio(portfolio, market)
    total_npv = sum(result.npv for result in valuation_results)
    metadata_by_trade = portfolio_trade_metadata_map(portfolio, valuation_results)
    sort_map = portfolio_trade_sort_map(portfolio, valuation_results)
    print(f"{position_header('NPV')} {'CCY':<3} {'Status':<20}")
    print("-" * POSITION_RULE_WIDTH)
    for result in sorted(
        valuation_results, key=lambda item: result_sort_key(item, sort_map)
    ):
        status = result.tags.get("status", "unknown")
        metadata = trade_metadata(metadata_by_trade, result.trade_id)
        print(
            f"{position_prefix(metadata)} {result.npv:>12,.2f} "
            f"{result.currency:<3} {status:<20}"
        )
    print("-" * POSITION_RULE_WIDTH)
    print(f"{'TOTAL':<{ASSET_COLUMN_WIDTH + PRODUCT_COLUMN_WIDTH + POSITION_COLUMN_WIDTH + 2}} {total_npv:>12,.2f} USD")
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
        "EURUSD": base["EURUSD"]
        * math.exp(scenario.factor_shocks["RF:FX:EURUSD:SPOT"]),
        "AAPL": base["AAPL"] * (1.0 + scenario.factor_shocks["RF:EQ:AAPL:SPOT"]),
        "USD_CAP_VOL_1Y": base["USD_CAP_VOL_1Y"]
        + scenario.factor_shocks["RF:RATESVOL:USD:CAP:1Y:ATM"] / 100.0,
    }

    for quote_id, expected_value in expected.items():
        assert_close(quote_id, shocked[quote_id], expected_value)
        print(f"{quote_id:<18} {base[quote_id]:>10.6f} -> {shocked[quote_id]:>10.6f}")

    print("Factor resolution checks passed.")


def run_stress(portfolio, market, factors, bindings, scenarios):
    section("4. Historical Stress")
    metadata_by_trade = portfolio_trade_metadata_map(portfolio)
    sort_map = portfolio_trade_sort_map(portfolio)
    stress_results = qrp.run_historical_stress(
        portfolio, market, scenarios, factors, bindings
    )
    if len(stress_results) != len(scenarios):
        raise AssertionError("Stress result count does not match scenario count")

    ordered_results = sorted(stress_results, key=lambda item: sort_text(item.scenario_name))
    for result in ordered_results:
        print(f"{result.scenario_name:<28} total P&L = {result.total_pnl:>15,.2f}")
        print(f"  {position_header('Trade P&L')}")
        print(f"  {'-' * POSITION_RULE_WIDTH}")
        for trade_id, pnl in sorted(
            result.trade_pnls.items(),
            key=lambda item: trade_id_sort_key(item[0], sort_map),
        ):
            if abs(pnl) > 1e-8:
                metadata = trade_metadata(metadata_by_trade, trade_id)
                print(f"  {position_prefix(metadata)} {pnl:>12,.2f}")
    return ordered_results


def run_risk(portfolio, market, factors, bindings):
    section("5. Risk Sensitivities")
    risk_results = qrp.compute_risk(portfolio, market, factors, bindings)
    if not risk_results:
        raise AssertionError("Expected risk results")

    metadata_by_trade = portfolio_trade_metadata_map(portfolio)
    sort_map = portfolio_trade_sort_map(portfolio)
    print(position_header("PV01", "CS01"))
    print("-" * POSITION_RULE_WIDTH)
    for result in sorted(
        risk_results, key=lambda item: result_sort_key(item, sort_map)
    ):
        metadata = trade_metadata(metadata_by_trade, result.trade_id)
        print(
            f"{position_prefix(metadata)} {result.pv01:>12,.2f} {result.cs01:>12,.2f}"
        )
        for node, value in sorted(
            result.bucketed_risk.items(), key=lambda item: sort_text(item[0])
        ):
            if abs(value) > 1e-8:
                print(f"  {node:<24} {value:>12,.2f}")
    return risk_results


def compute_pnl_explain(portfolio, market_path):
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
    return pnl_results


def run_pnl_explain(portfolio, market_path):
    section("6. P&L Explain")
    pnl_results = compute_pnl_explain(portfolio, market_path)
    metadata_by_trade = portfolio_trade_metadata_map(portfolio)
    sort_map = portfolio_trade_sort_map(portfolio)
    print(position_header("Total", "Carry", "Market"))
    print("-" * POSITION_RULE_WIDTH)
    for result in sorted(pnl_results, key=lambda item: result_sort_key(item, sort_map)):
        metadata = trade_metadata(metadata_by_trade, result.trade_id)
        print(
            f"{position_prefix(metadata)} {result.total_pnl:>12,.2f} "
            f"{result.carry_pnl:>12,.2f} {result.market_move_pnl:>12,.2f}"
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
                covariance[i, j] = 0.15 * math.sqrt(
                    factor_variance(factor_i) * factor_variance(factor_j)
                )
    return covariance


def compute_monte_carlo(portfolio, market, factors, bindings, cases=None):
    mc_factors = [
        factor for factor in factors if factor.factor_id != "RF:RATES:USD:OIS:PARALLEL"
    ]
    covariance = build_covariance(mc_factors)
    selected_cases = cases or MONTE_CARLO_CASES

    results = []
    for label, mode, horizon_days in selected_cases:
        config = qrp.MonteCarloConfig()
        config.horizon_days = horizon_days
        config.mode = mode
        config.num_paths = 100
        config.seed = 42

        result = qrp.run_simulation(
            portfolio, market, mc_factors, bindings, covariance, config
        )
        if not result.portfolio_pnls:
            raise AssertionError(f"{label} produced no P&L paths")

        mean_pnl = sum(result.portfolio_pnls) / len(result.portfolio_pnls)
        results.append((label, result, mean_pnl))

    return results


def print_traces(traces, mode):
    for trace in traces[:2]:
        print(f"\nPath {trace.path_index + 1}")
        if mode == qrp.MonteCarloMode.AgedHorizonRevaluation:
            print(
                f"  Dates:        {trace.valuation_date_before} -> {trace.valuation_date_after}"
            )
            print(f"  Frozen aged:  {trace.portfolio_value_frozen_aged:,.2f}")
            print(f"  Shocked aged: {trace.portfolio_value_after:,.2f}")
            print(f"  Total P&L:    {trace.total_pnl:,.2f}")
        else:
            print(
                f"  Value:        {trace.portfolio_value_before:,.2f} -> {trace.portfolio_value_after:,.2f}"
            )
            print(f"  Path P&L:     {trace.path_pnl:,.2f}")

        print("  Factor shocks:")
        for factor_id, shock in sorted(
            trace.factor_shocks.items(), key=lambda item: sort_text(item[0])
        ):
            print(f"    {factor_id:<28} {shock:+.8f}")


def run_monte_carlo(portfolio, market, factors, bindings):
    section("7. Monte Carlo")
    results = compute_monte_carlo(
        portfolio, market, factors, bindings, MONTE_CARLO_CASES
    )
    for (label, mode, horizon_days), (_, result, mean_pnl) in zip(
        MONTE_CARLO_CASES, results
    ):
        print(
            f"{label:<24} horizon={horizon_days:>4.0f}d "
            f"VaR95={result.var_95:>12,.2f} ES95={result.expected_shortfall_95:>12,.2f} "
            f"mean={mean_pnl:>12,.2f}"
        )
        print_traces(result.traces, mode)

    return results


def compute_portfolio_analytics(
    portfolio, market, market_path, factors, bindings, scenarios, monte_carlo_cases=None
):
    valuation_results = qrp.price_portfolio(portfolio, market)
    total_npv = sum(result.npv for result in valuation_results)
    stress_results = qrp.run_historical_stress(
        portfolio, market, scenarios, factors, bindings
    )
    risk_results = qrp.compute_risk(portfolio, market, factors, bindings)
    pnl_results = compute_pnl_explain(portfolio, market_path)
    mc_results = compute_monte_carlo(
        portfolio, market, factors, bindings, monte_carlo_cases
    )
    return {
        "mc_results": mc_results,
        "pnl_results": pnl_results,
        "portfolio": portfolio,
        "risk_results": risk_results,
        "stress_results": stress_results,
        "total_npv": total_npv,
        "valuation_results": valuation_results,
    }


def build_dashboard_portfolio_views(
    primary_portfolio_id, market, market_path, factors, bindings, scenarios
):
    views = []
    seen = {primary_portfolio_id}
    portfolio_dir = project_root / "data" / "portfolios"
    for portfolio_file in DASHBOARD_PORTFOLIOS:
        portfolio = qrp.load_portfolio(str(portfolio_dir / portfolio_file))
        if portfolio.portfolio_id in seen:
            continue
        views.append(
            compute_portfolio_analytics(
                portfolio,
                market,
                market_path,
                factors,
                bindings,
                scenarios,
                DASHBOARD_PORTFOLIO_MONTE_CARLO_CASES,
            )
        )
        seen.add(portfolio.portfolio_id)
    return views


def validate_golden_outputs(
    golden,
    valuation_results,
    total_npv,
    stress_results,
    risk_results,
    pnl_results,
    mc_results,
):
    section("Golden Regression Checks")

    valuation = golden["valuation"]
    valuation_tolerance = valuation.get("tolerance", 1.0)
    assert_close(
        "valuation.total_npv", total_npv, valuation["total_npv"], valuation_tolerance
    )
    valuation_by_trade = {result.trade_id: result for result in valuation_results}
    assert_key_sets_match("valuation.trades", valuation_by_trade, valuation["trades"])
    for trade_id, expected in valuation["trades"].items():
        actual = valuation_by_trade[trade_id]
        assert_close(
            f"valuation.{trade_id}.npv",
            actual.npv,
            expected["npv"],
            valuation_tolerance,
        )
        if actual.tags.get("asset_class") != expected["asset_class"]:
            raise AssertionError(f"valuation.{trade_id}.asset_class drifted")
        if actual.tags.get("product_type") != expected["product_type"]:
            raise AssertionError(f"valuation.{trade_id}.product_type drifted")
        if actual.tags.get("status") != expected["support_status"]:
            raise AssertionError(f"valuation.{trade_id}.support_status drifted")

    stress = golden["stress"]
    stress_tolerance = stress.get("tolerance", 1.0)
    stress_by_name = {result.scenario_name: result for result in stress_results}
    assert_key_sets_match(
        "stress.scenarios", stress_by_name, expected_regression_keys(stress)
    )
    for scenario_name in expected_regression_keys(stress):
        expected = stress[scenario_name]
        actual = stress_by_name[scenario_name]
        assert_close(
            f"stress.{scenario_name}.total_pnl",
            actual.total_pnl,
            expected["total_pnl"],
            stress_tolerance,
        )
        actual_trade_pnls = material_value_map(actual.trade_pnls, stress_tolerance)
        assert_key_sets_match(
            f"stress.{scenario_name}.trade_pnls",
            actual_trade_pnls,
            expected["trade_pnls"],
        )
        for trade_id, expected_pnl in expected["trade_pnls"].items():
            assert_close(
                f"stress.{scenario_name}.{trade_id}",
                dict(actual.trade_pnls).get(trade_id, 0.0),
                expected_pnl,
                stress_tolerance,
            )

    risk = golden["risk"]
    risk_tolerance = risk.get("tolerance", 1.0)
    risk_by_trade = {result.trade_id: result for result in risk_results}
    assert_key_sets_match("risk.trades", risk_by_trade, expected_regression_keys(risk))
    for trade_id in expected_regression_keys(risk):
        expected = risk[trade_id]
        actual = risk_by_trade[trade_id]
        for component in ["bucketed_risk", "cs01", "fx_delta", "fx_vega", "pv01"]:
            if component == "bucketed_risk":
                actual_buckets = material_value_map(
                    actual.bucketed_risk, risk_tolerance
                )
                assert_key_sets_match(
                    f"risk.{trade_id}.bucketed_risk",
                    actual_buckets,
                    expected[component],
                )
                for node, expected_value in expected[component].items():
                    assert_close(
                        f"risk.{trade_id}.bucketed_risk.{node}",
                        dict(actual.bucketed_risk).get(node, 0.0),
                        expected_value,
                        risk_tolerance,
                    )
            else:
                assert_close(
                    f"risk.{trade_id}.{component}",
                    getattr(actual, component),
                    expected[component],
                    risk_tolerance,
                )

    pnl = golden["pnl_explain"]
    pnl_tolerance = pnl.get("tolerance", 1.0)
    pnl_by_trade = {result.trade_id: result for result in pnl_results}
    assert_key_sets_match("pnl.trades", pnl_by_trade, expected_regression_keys(pnl))
    for trade_id in expected_regression_keys(pnl):
        expected = pnl[trade_id]
        actual = pnl_by_trade[trade_id]
        for component in [
            "carry_pnl",
            "cash_pnl",
            "market_move_pnl",
            "residual_pnl",
            "total_pnl",
        ]:
            assert_close(
                f"pnl.{trade_id}.{component}",
                getattr(actual, component, 0.0),
                expected[component],
                pnl_tolerance,
            )

    monte_carlo = golden["monte_carlo"]
    mc_by_label = {label: (result, mean_pnl) for label, result, mean_pnl in mc_results}
    assert_key_sets_match("mc.cases", mc_by_label, monte_carlo)
    for label, expected in monte_carlo.items():
        actual, mean_pnl = mc_by_label[label]
        tolerance = expected.get("tolerance", 50.0)
        assert_close(
            f"mc.{label}.base_portfolio_value",
            actual.base_portfolio_value,
            expected["base_portfolio_value"],
            tolerance,
        )
        assert_close(
            f"mc.{label}.expected_shortfall_95",
            actual.expected_shortfall_95,
            expected["expected_shortfall_95"],
            tolerance,
        )
        assert_close(f"mc.{label}.mean_pnl", mean_pnl, expected["mean_pnl"], tolerance)
        assert_close(f"mc.{label}.var_95", actual.var_95, expected["var_95"], tolerance)

    print("Golden regression checks passed.")


def validate_product_family_outputs(
    product_families, valuation_results, stress_results, risk_results, pnl_results
):
    section("Product-Family Golden Checks")

    pnl_by_trade = {result.trade_id: result for result in pnl_results}
    risk_by_trade = {result.trade_id: result for result in risk_results}
    stress_by_name = {result.scenario_name: result for result in stress_results}
    valuation_by_trade = {result.trade_id: result for result in valuation_results}
    fixture_by_trade = {}

    for family in product_families:
        for trade_id in family["trades"]:
            if trade_id in fixture_by_trade:
                raise AssertionError(
                    f"Duplicate product-family golden coverage for {trade_id}"
                )
            fixture_by_trade[trade_id] = Path(family["fixture_path"]).name

    assert_key_sets_match(
        "product_families.trades", valuation_by_trade, fixture_by_trade
    )

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
                raise AssertionError(
                    f"{fixture_name}.{trade_id}.support_status drifted"
                )

            assert_close(
                f"{fixture_name}.{trade_id}.valuation.npv",
                valuation_result.npv,
                expected["valuation"]["npv"],
                tolerances.get("valuation", 1.0),
            )
            assert_close(
                f"{fixture_name}.{trade_id}.risk.cs01",
                risk_result.cs01,
                expected["risk"]["cs01"],
                tolerances.get("risk", 1.0),
            )
            assert_close(
                f"{fixture_name}.{trade_id}.risk.fx_delta",
                risk_result.fx_delta,
                expected["risk"]["fx_delta"],
                tolerances.get("risk", 1.0),
            )
            assert_close(
                f"{fixture_name}.{trade_id}.risk.fx_vega",
                risk_result.fx_vega,
                expected["risk"]["fx_vega"],
                tolerances.get("risk", 1.0),
            )
            assert_close(
                f"{fixture_name}.{trade_id}.risk.pv01",
                risk_result.pv01,
                expected["risk"]["pv01"],
                tolerances.get("risk", 1.0),
            )
            actual_buckets = material_value_map(
                risk_result.bucketed_risk, tolerances.get("risk", 1.0)
            )
            assert_key_sets_match(
                f"{fixture_name}.{trade_id}.risk.bucketed_risk",
                actual_buckets,
                expected["risk"]["bucketed_risk"],
            )
            for node, expected_value in expected["risk"]["bucketed_risk"].items():
                assert_close(
                    f"{fixture_name}.{trade_id}.risk.bucketed_risk.{node}",
                    dict(risk_result.bucketed_risk).get(node, 0.0),
                    expected_value,
                    tolerances.get("risk", 1.0),
                )
            assert_close(
                f"{fixture_name}.{trade_id}.pnl.carry_pnl",
                pnl_result.carry_pnl,
                expected["pnl_explain"]["carry_pnl"],
                tolerances.get("pnl_explain", 1.0),
            )
            assert_close(
                f"{fixture_name}.{trade_id}.pnl.cash_pnl",
                getattr(pnl_result, "cash_pnl", 0.0),
                expected["pnl_explain"]["cash_pnl"],
                tolerances.get("pnl_explain", 1.0),
            )
            assert_close(
                f"{fixture_name}.{trade_id}.pnl.market_move_pnl",
                pnl_result.market_move_pnl,
                expected["pnl_explain"]["market_move_pnl"],
                tolerances.get("pnl_explain", 1.0),
            )
            assert_close(
                f"{fixture_name}.{trade_id}.pnl.residual_pnl",
                getattr(pnl_result, "residual_pnl", 0.0),
                expected["pnl_explain"]["residual_pnl"],
                tolerances.get("pnl_explain", 1.0),
            )
            assert_close(
                f"{fixture_name}.{trade_id}.pnl.total_pnl",
                pnl_result.total_pnl,
                expected["pnl_explain"]["total_pnl"],
                tolerances.get("pnl_explain", 1.0),
            )

            for scenario_name, expected_pnl in expected.get("stress", {}).items():
                stress_result = stress_by_name[scenario_name]
                assert_close(
                    f"{fixture_name}.{trade_id}.stress.{scenario_name}",
                    stress_result.trade_pnls[trade_id],
                    expected_pnl,
                    tolerances.get("stress", 1.0),
                )

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
        print(
            f"Skipped: CVXPY worker dependencies are not available in this Python environment ({exc})."
        )
        print("Install the optional optimization dependencies with:")
        print("  uv sync --project python --extra optimization")
        print("Then run the demo with the uv-managed interpreter:")
        print("  uv run --project python python python/examples/demo_platform.py")
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
            {
                "type": "LinearEquality",
                "coefficients": {"AAPL": 1.0, "MSFT": 1.0},
                "target": 1.0,
            }
        ],
        "config": {
            "solver": "OSQP",
            "tolerance": 1e-8,
            "max_iterations": 10000,
            "verbose": False,
        },
    }

    result = worker.solve_optimization(request)
    print(json.dumps(result, indent=2, sort_keys=True))
    if result.get("status") not in {"optimal", "optimal_inaccurate"}:
        raise AssertionError(f"CVXPY worker check failed: {result}")
    return result


def parse_args():
    parser = argparse.ArgumentParser(
        description="Run the Quant Risk Platform Python demo."
    )
    parser.add_argument(
        "--dashboard",
        action="store_true",
        help="Create reports/demo_risk_dashboard.html and open it in the default web browser.",
    )
    return parser.parse_args()


def run_demo(dashboard=False):
    section("Quant Risk Platform Python Demo")
    market_path = project_root / "data" / "market" / "demo_market.json"
    portfolio_path = project_root / "data" / "portfolios" / "demo_portfolio.json"
    golden_path = project_root / "data" / "regression" / "demo_golden.json"
    product_family_golden_path = (
        project_root / "data" / "regression" / "product_families"
    )
    scenario_path = project_root / "data" / "scenarios" / "demo_scenarios.json"

    golden = load_golden_expectations(golden_path)
    product_families = load_product_family_expectations(product_family_golden_path)
    market = qrp.load_market(str(market_path))
    portfolio = qrp.load_portfolio(str(portfolio_path))
    scenario_payload, factors, bindings, scenarios = load_factor_scenario_set(
        scenario_path
    )

    print(f"Market date:       {market.valuation_date}")
    print(
        f"Portfolio:         {portfolio.portfolio_id} ({len(portfolio.trades)} trades)"
    )
    print(
        f"Scenario set:      {scenario_payload['scenario_set_id']} ({len(scenarios)} scenarios)"
    )
    print(f"Factors/bindings:  {len(factors)} factors / {len(bindings)} bindings")

    print_market_data_coverage(market, factors, bindings)
    valuation_results, total_npv = print_portfolio_valuation(portfolio, market)
    run_factor_resolution_checks(market, factors, bindings, scenarios)
    stress_results = run_stress(portfolio, market, factors, bindings, scenarios)
    risk_results = run_risk(portfolio, market, factors, bindings)
    pnl_results = run_pnl_explain(portfolio, market_path)
    mc_results = run_monte_carlo(portfolio, market, factors, bindings)
    validate_golden_outputs(
        golden,
        valuation_results,
        total_npv,
        stress_results,
        risk_results,
        pnl_results,
        mc_results,
    )
    validate_product_family_outputs(
        product_families, valuation_results, stress_results, risk_results, pnl_results
    )
    optimization_result = run_optional_cvxpy_worker_check()
    if dashboard:
        portfolio_views = build_dashboard_portfolio_views(
            portfolio.portfolio_id, market, market_path, factors, bindings, scenarios
        )
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
            portfolio_views=portfolio_views,
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
