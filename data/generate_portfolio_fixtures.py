#!/usr/bin/env python3
"""Generate portfolio-backed structural golden fixtures.

The sample portfolios are intentionally financial/risk-profile oriented. They
do not encode roadmap stage labels; product coverage is represented through
books and trades that a portfolio manager could plausibly own.
"""

from __future__ import annotations

import json
from collections import defaultdict
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
PORTFOLIO_DIR = ROOT / "data" / "portfolios"
FIXTURE_DIR = ROOT / "data" / "regression" / "portfolio_fixtures"

STATUS_BY_TYPE = {
    "bermudan_swaption": "supported",
    "cap_floor": "supported",
    "cds": "supported",
    "cds_index": "supported",
    "cds_option": "supported",
    "commodity_calendar_spread_option": "supported",
    "commodity_forward": "supported",
    "commodity_future": "supported",
    "commodity_future_option": "supported",
    "commodity_future_strip": "supported",
    "commodity_spot": "supported",
    "commodity_swing": "supported",
    "gas_storage": "supported",
    "credit_bond": "supported",
    "credit_index_option": "supported",
    "deposit": "supported",
    "equity_forward": "supported",
    "equity_future": "supported",
    "equity_option": "supported",
    "equity_spot": "supported",
    "european_swaption": "supported",
    "fixed_rate_bond": "supported",
    "floating_rate_note": "supported",
    "fra": "supported",
    "fx_forward": "supported",
    "fx_option": "supported",
    "fx_spot": "supported",
    "fx_swap": "supported",
    "interest_rate_future": "supported",
    "ndf": "supported",
    "ois_swap": "supported",
    "vanilla_swap": "supported",
}

RISK = {
    "liquidity_reserve": "LIQUIDITY_RESERVE",
    "cash_plus": "CASH_PLUS",
    "defensive_income": "DEFENSIVE_INCOME",
    "conservative_income": "CONSERVATIVE_INCOME",
    "cautious": "CAUTIOUS",
    "stable_balanced": "STABLE_BALANCED",
    "balanced_core": "BALANCED_CORE",
    "balanced_growth": "BALANCED_GROWTH",
    "growth": "GROWTH",
    "high_growth": "HIGH_GROWTH",
    "aggressive_growth": "AGGRESSIVE_GROWTH",
    "adventurous": "ADVENTUROUS",
}


def book(risk_key: str, sleeve: str) -> str:
    return f"BOOK:{RISK[risk_key]}:{sleeve}"


def sort_text(value) -> str:
    return str(value or "").casefold()


def hierarchy_key(*layers) -> tuple[str, ...]:
    return tuple(sort_text(layer) for layer in layers)


def trade_hierarchy_key(trade: dict) -> tuple[str, str, str]:
    return hierarchy_key(trade["asset_class"], trade["type"], trade["id"])


def base(
    trade_id: str,
    asset_class: str,
    product_type: str,
    book_id: str,
    strategy: str,
    direction: str,
) -> dict:
    return {
        "id": trade_id,
        "asset_class": asset_class,
        "type": product_type,
        "currency": "USD",
        "direction": direction,
        "book": book_id,
        "strategy": strategy,
    }


def dep(trade_id, book_id, strategy, notional=1_000_000, rate=0.052, direction="lend"):
    trade = base(trade_id, "rates", "deposit", book_id, strategy, direction)
    trade.update(
        {
            "notional": notional,
            "start_date": "2026-03-26",
            "maturity_date": "2026-06-26",
            "details": {"deposit_rate": rate},
        }
    )
    return trade


def fra(
    trade_id, book_id, strategy, notional=3_000_000, strike=0.0535, direction="long"
):
    trade = base(trade_id, "rates", "fra", book_id, strategy, direction)
    trade.update(
        {
            "notional": notional,
            "start_date": "2026-06-24",
            "maturity_date": "2026-09-24",
            "details": {"strike_rate": strike, "floating_index": "USD_LIBOR_3M"},
        }
    )
    return trade


def irf(trade_id, book_id, strategy, quantity=4, ref_price=94.25, direction="long"):
    trade = base(
        trade_id, "rates", "interest_rate_future", book_id, strategy, direction
    )
    trade.update(
        {
            "quantity": quantity,
            "start_date": "2026-06-17",
            "maturity_date": "2026-09-17",
            "details": {
                "contract_size": 2500,
                "reference_price": ref_price,
                "floating_index": "USD_LIBOR_3M",
                "future_quote_id": "USD_IR_FUT_JUN26",
            },
        }
    )
    return trade


def swap(
    trade_id, book_id, strategy, notional=5_000_000, fixed=0.052, direction="pay_fixed"
):
    trade = base(trade_id, "rates", "vanilla_swap", book_id, strategy, direction)
    trade.update(
        {
            "notional": notional,
            "start_date": "2026-03-26",
            "maturity_date": "2031-03-26",
            "details": {"fixed_rate": fixed, "floating_index": "USD_LIBOR_3M"},
        }
    )
    return trade


def ois(
    trade_id,
    book_id,
    strategy,
    notional=4_000_000,
    fixed=0.052,
    direction="receive_fixed",
):
    trade = base(trade_id, "rates", "ois_swap", book_id, strategy, direction)
    trade.update(
        {
            "notional": notional,
            "start_date": "2026-03-26",
            "maturity_date": "2031-03-26",
            "details": {"fixed_rate": fixed, "overnight_index": "SOFR", "spread": 0.0},
        }
    )
    return trade


def bond(
    trade_id, book_id, strategy, notional=2_000_000, coupon=0.055, maturity="2031-03-26"
):
    trade = base(trade_id, "rates", "fixed_rate_bond", book_id, strategy, "long")
    trade.update(
        {
            "notional": notional,
            "start_date": "2026-03-26",
            "maturity_date": maturity,
            "details": {"coupon_rate": coupon, "frequency": "Annual"},
        }
    )
    return trade


def frn(trade_id, book_id, strategy, notional=2_000_000, spread=0.001):
    trade = base(trade_id, "rates", "floating_rate_note", book_id, strategy, "long")
    trade.update(
        {
            "notional": notional,
            "start_date": "2026-03-26",
            "maturity_date": "2029-03-26",
            "details": {
                "floating_index": "USD_LIBOR_3M",
                "frequency": "Quarterly",
                "spread": spread,
            },
        }
    )
    return trade


def cap(
    trade_id, book_id, strategy, notional=2_000_000, strike=0.055, cap_floor_type="cap"
):
    trade = base(trade_id, "rates", "cap_floor", book_id, strategy, "long")
    trade.update(
        {
            "notional": notional,
            "start_date": "2026-06-24",
            "maturity_date": "2027-06-24",
            "details": {
                "strike_rate": strike,
                "cap_floor_type": cap_floor_type,
                "floating_index": "USD_LIBOR_3M",
                "volatility_quote_id": "USD_CAP_VOL_1Y",
            },
        }
    )
    return trade


def eswaption(trade_id, book_id, strategy, notional=3_000_000, fixed=0.055):
    trade = base(trade_id, "rates", "european_swaption", book_id, strategy, "payer")
    trade.update(
        {
            "notional": notional,
            "start_date": "2027-03-24",
            "maturity_date": "2031-03-24",
            "details": {
                "option_expiry_date": "2027-03-24",
                "fixed_rate": fixed,
                "floating_index": "USD_LIBOR_3M",
                "volatility_quote_id": "USD_SWAPTION_VOL_1Y_5Y",
            },
        }
    )
    return trade


def bswaption(trade_id, book_id, strategy, notional=3_000_000, fixed=0.055):
    trade = base(trade_id, "rates", "bermudan_swaption", book_id, strategy, "payer")
    trade.update(
        {
            "notional": notional,
            "start_date": "2026-06-24",
            "maturity_date": "2031-06-24",
            "details": {
                "fixed_rate": fixed,
                "floating_index": "USD_LIBOR_3M",
                "volatility": 0.012,
                "mean_reversion": 0.03,
                "exercise_dates": [
                    "2026-06-24",
                    "2027-06-24",
                    "2028-06-24",
                    "2029-06-24",
                ],
            },
        }
    )
    return trade


def fxspot(
    trade_id, book_id, strategy, notional=1_000_000, ref_rate=1.08, direction="buy_base"
):
    trade = base(trade_id, "fx", "fx_spot", book_id, strategy, direction)
    trade.update(
        {
            "notional": notional,
            "details": {
                "base_currency": "EUR",
                "quote_currency": "USD",
                "reference_rate": ref_rate,
                "spot_quote_id": "EURUSD",
            },
        }
    )
    return trade


def fxfwd(
    trade_id,
    book_id,
    strategy,
    notional=2_000_000,
    forward=1.1025,
    direction="buy_base",
):
    trade = base(trade_id, "fx", "fx_forward", book_id, strategy, direction)
    trade.update(
        {
            "notional": notional,
            "start_date": "2026-03-24",
            "maturity_date": "2026-09-24",
            "details": {
                "base_currency": "EUR",
                "quote_currency": "USD",
                "forward_rate": forward,
                "spot_quote_id": "EURUSD",
                "forward_points_quote_id": "EURUSD_FWDPTS_6M",
            },
        }
    )
    return trade


def fxswap(trade_id, book_id, strategy, notional=1_500_000, direction="buy_base"):
    trade = base(trade_id, "fx", "fx_swap", book_id, strategy, direction)
    trade.update(
        {
            "notional": notional,
            "start_date": "2026-06-24",
            "maturity_date": "2027-03-24",
            "details": {
                "base_currency": "EUR",
                "quote_currency": "USD",
                "near_rate": 1.088,
                "far_rate": 1.091,
                "spot_quote_id": "EURUSD",
                "near_forward_points_quote_id": "EURUSD_FWDPTS_3M",
                "far_forward_points_quote_id": "EURUSD_FWDPTS_1Y",
            },
        }
    )
    return trade


def ndf(trade_id, book_id, strategy, notional=2_000_000, forward=1.103):
    trade = base(trade_id, "fx", "ndf", book_id, strategy, "buy_base")
    trade.update(
        {
            "notional": notional,
            "maturity_date": "2026-09-24",
            "details": {
                "base_currency": "EUR",
                "quote_currency": "USD",
                "fixing_date": "2026-09-22",
                "forward_rate": forward,
                "spot_quote_id": "EURUSD",
                "forward_points_quote_id": "EURUSD_FWDPTS_6M",
            },
        }
    )
    return trade


def fxopt(
    trade_id, book_id, strategy, notional=1_000_000, strike=1.10, option_type="call"
):
    trade = base(trade_id, "fx", "fx_option", book_id, strategy, "long")
    trade.update(
        {
            "notional": notional,
            "details": {
                "base_currency": "EUR",
                "quote_currency": "USD",
                "strike_rate": strike,
                "expiry_date": "2026-09-24",
                "settlement_date": "2026-09-28",
                "option_type": option_type,
                "spot_quote_id": "EURUSD",
                "volatility_quote_id": "EURUSD_VOL_6M_ATM",
            },
        }
    )
    return trade


def cbond(trade_id, book_id, strategy, notional=2_000_000, coupon=0.055):
    trade = base(trade_id, "credit", "credit_bond", book_id, strategy, "long")
    trade.update(
        {
            "notional": notional,
            "start_date": "2026-03-24",
            "maturity_date": "2031-03-24",
            "details": {
                "coupon_rate": coupon,
                "frequency": "Annual",
                "issuer": "ACME",
                "spread_quote_id": "ACME_BOND_SPREAD_5Y",
            },
        }
    )
    return trade


def cds(
    trade_id,
    book_id,
    strategy,
    notional=3_000_000,
    coupon=0.010,
    direction="buy_protection",
):
    trade = base(trade_id, "credit", "cds", book_id, strategy, direction)
    trade.update(
        {
            "notional": notional,
            "start_date": "2026-03-24",
            "maturity_date": "2031-03-24",
            "details": {
                "coupon_rate": coupon,
                "frequency": "Quarterly",
                "issuer": "ACME",
                "recovery_quote_id": "ACME_RECOVERY",
                "spread_quote_id": "ACME_CDS_5Y",
            },
        }
    )
    return trade


def cdx(
    trade_id,
    book_id,
    strategy,
    notional=5_000_000,
    coupon=0.009,
    direction="buy_protection",
):
    trade = base(trade_id, "credit", "cds_index", book_id, strategy, direction)
    trade.update(
        {
            "notional": notional,
            "start_date": "2026-03-24",
            "maturity_date": "2031-03-24",
            "details": {
                "coupon_rate": coupon,
                "frequency": "Quarterly",
                "index_factor": 0.98,
                "index_name": "CDX_IG",
                "recovery_quote_id": "CDX_IG_RECOVERY",
                "spread_quote_id": "CDX_IG_5Y",
            },
        }
    )
    return trade


def cdsopt(trade_id, book_id, strategy, notional=3_000_000, strike=0.015):
    trade = base(trade_id, "credit", "cds_option", book_id, strategy, "long")
    trade.update(
        {
            "notional": notional,
            "maturity_date": "2031-03-24",
            "details": {
                "expiry_date": "2027-03-24",
                "strike_spread": strike,
                "frequency": "Quarterly",
                "issuer": "ACME",
                "option_type": "call",
                "recovery_quote_id": "ACME_RECOVERY",
                "spread_quote_id": "ACME_CDS_5Y",
                "volatility_quote_id": "ACME_CDS_OPTION_VOL_1Y_5Y",
            },
        }
    )
    return trade


def cdxopt(trade_id, book_id, strategy, notional=5_000_000, strike=0.009):
    trade = base(trade_id, "credit", "credit_index_option", book_id, strategy, "long")
    trade.update(
        {
            "notional": notional,
            "maturity_date": "2031-03-24",
            "details": {
                "expiry_date": "2027-03-24",
                "index_factor": 0.98,
                "index_name": "CDX_IG",
                "strike_spread": strike,
                "frequency": "Quarterly",
                "option_type": "call",
                "recovery_quote_id": "CDX_IG_RECOVERY",
                "spread_quote_id": "CDX_IG_5Y",
                "volatility_quote_id": "CDX_IG_OPTION_VOL_1Y_5Y",
            },
        }
    )
    return trade


def cspot(trade_id, book_id, strategy, quantity=1000, ref_price=77.0, direction="long"):
    trade = base(trade_id, "commodity", "commodity_spot", book_id, strategy, direction)
    trade.update(
        {
            "quantity": quantity,
            "details": {
                "underlier": "WTI",
                "reference_price": ref_price,
                "quote_id": "WTI_SPOT",
                "unit": "bbl",
            },
        }
    )
    return trade


def cfwd(trade_id, book_id, strategy, quantity=1000, price=78.0):
    trade = base(trade_id, "commodity", "commodity_forward", book_id, strategy, "long")
    trade.update(
        {
            "quantity": quantity,
            "maturity_date": "2026-09-24",
            "details": {
                "underlier": "WTI",
                "contract_price": price,
                "forward_quote_id": "WTI_FUT_6M",
                "tenor": "6M",
                "unit": "bbl",
            },
        }
    )
    return trade


def cfut(trade_id, book_id, strategy, quantity=2, ref_price=78.0, direction="long"):
    trade = base(
        trade_id, "commodity", "commodity_future", book_id, strategy, direction
    )
    trade.update(
        {
            "quantity": quantity,
            "maturity_date": "2026-09-24",
            "details": {
                "underlier": "WTI",
                "contract_size": 1000,
                "reference_price": ref_price,
                "future_quote_id": "WTI_FUT_6M",
                "tenor": "6M",
                "unit": "bbl",
            },
        }
    )
    return trade


def cstrip(trade_id, book_id, strategy, quantity=1, ref_price=78.5):
    trade = base(
        trade_id, "commodity", "commodity_future_strip", book_id, strategy, "long"
    )
    trade.update(
        {
            "quantity": quantity,
            "start_date": "2026-09-24",
            "maturity_date": "2026-12-24",
            "details": {
                "underlier": "WTI",
                "contract_size": 1000,
                "reference_price": ref_price,
                "future_quote_ids": ["WTI_FUT_6M", "WTI_FUT_9M"],
                "weights": [0.5, 0.5],
                "unit": "bbl",
            },
        }
    )
    return trade


def cfutopt(trade_id, book_id, strategy, quantity=1, strike=80.0):
    trade = base(
        trade_id, "commodity", "commodity_future_option", book_id, strategy, "long"
    )
    trade.update(
        {
            "quantity": quantity,
            "maturity_date": "2026-09-24",
            "details": {
                "underlier": "WTI",
                "contract_size": 1000,
                "strike_price": strike,
                "expiry_date": "2026-09-24",
                "future_quote_id": "WTI_FUT_6M",
                "volatility_quote_id": "WTI_VOL_6M_ATM",
                "option_type": "call",
                "unit": "bbl",
            },
        }
    )
    return trade


def ccal(trade_id, book_id, strategy, quantity=1, strike=0.5):
    trade = base(
        trade_id,
        "commodity",
        "commodity_calendar_spread_option",
        book_id,
        strategy,
        "long",
    )
    trade.update(
        {
            "quantity": quantity,
            "details": {
                "underlier": "WTI",
                "contract_size": 1000,
                "strike_spread": strike,
                "expiry_date": "2026-09-24",
                "near_future_quote_id": "WTI_FUT_6M",
                "far_future_quote_id": "WTI_FUT_9M",
                "volatility_quote_id": "WTI_VOL_6M_ATM",
                "option_type": "call",
                "unit": "bbl",
            },
        }
    )
    return trade


def swing(trade_id, book_id, strategy):
    trade = base(trade_id, "commodity", "commodity_swing", book_id, strategy, "long")
    trade.update(
        {
            "start_date": "2026-04-01",
            "maturity_date": "2026-12-24",
            "details": {
                "underlier": "TTF",
                "min_quantity": 100,
                "max_quantity": 250,
                "strike_price": 31.0,
                "forward_quote_ids": ["TTF_FWD_Q1", "TTF_FWD_Q2"],
                "exercise_dates": ["2026-06-24", "2026-09-24"],
                "volatility": 0.40,
                "unit": "MWh",
            },
        }
    )
    return trade


def storage(trade_id, book_id, strategy):
    trade = base(trade_id, "commodity", "gas_storage", book_id, strategy, "long")
    trade.update(
        {
            "start_date": "2026-04-01",
            "maturity_date": "2026-12-24",
            "details": {
                "underlier": "TTF",
                "min_inventory": 0,
                "max_inventory": 200,
                "initial_inventory": 50,
                "terminal_inventory_target": 50,
                "terminal_inventory_penalty": 100,
                "max_injection_quantity": 100,
                "max_withdrawal_quantity": 100,
                "injection_cost": 0.20,
                "withdrawal_cost": 0.20,
                "forward_quote_ids": ["TTF_FWD_Q1", "TTF_FWD_Q2"],
                "exercise_dates": ["2026-06-24", "2026-09-24"],
                "unit": "MWh",
            },
        }
    )
    return trade


def eqspot(trade_id, book_id, strategy, quantity=100, ref_price=180.0):
    trade = base(trade_id, "equity", "equity_spot", book_id, strategy, "long")
    trade.update(
        {
            "quantity": quantity,
            "details": {"underlier": "AAPL", "reference_price": ref_price},
        }
    )
    return trade


def eqfwd(trade_id, book_id, strategy, quantity=100, forward=184.0):
    trade = base(trade_id, "equity", "equity_forward", book_id, strategy, "long")
    trade.update(
        {
            "quantity": quantity,
            "maturity_date": "2026-09-24",
            "details": {
                "underlier": "AAPL",
                "forward_price": forward,
                "spot_quote_id": "AAPL",
                "dividend_yield_quote_id": "AAPL_DIVYLD_1Y",
                "borrow_rate_quote_id": "AAPL_BORROW_1Y",
            },
        }
    )
    return trade


def eqfut(trade_id, book_id, strategy, quantity=2, ref_price=5350.0):
    trade = base(trade_id, "equity", "equity_future", book_id, strategy, "long")
    trade.update(
        {
            "quantity": quantity,
            "maturity_date": "2026-09-24",
            "details": {
                "underlier": "SPX",
                "contract_size": 50,
                "reference_price": ref_price,
                "future_quote_id": "ES_FUT_6M",
                "spot_quote_id": "SPX",
            },
        }
    )
    return trade


def eqopt(
    trade_id,
    book_id,
    strategy,
    quantity=100,
    strike=185.0,
    option_type="call",
    exercise="european",
):
    trade = base(trade_id, "equity", "equity_option", book_id, strategy, "long")
    trade.update(
        {
            "quantity": quantity,
            "details": {
                "underlier": "AAPL",
                "strike_price": strike,
                "expiry_date": "2026-09-24",
                "settlement_date": "2026-09-24",
                "spot_quote_id": "AAPL",
                "dividend_yield_quote_id": "AAPL_DIVYLD_1Y",
                "borrow_rate_quote_id": "AAPL_BORROW_1Y",
                "volatility_quote_id": "AAPL_VOL_6M_ATM",
                "exercise_style": exercise,
                "option_type": option_type,
            },
        }
    )
    return trade


def make_portfolio(portfolio_id, name, defensive, growth, style, description, trades):
    by_book = defaultdict(list)
    ordered_trades = sorted(trades, key=trade_hierarchy_key)
    for trade in ordered_trades:
        by_book[trade["book"]].append(trade["id"])
    portfolio = {
        "portfolio_id": portfolio_id,
        "portfolio_name": name,
        "portfolio_style": style,
        "description": description,
        "base_currency": "USD",
        "asset_mix": {"defensive_assets_pct": defensive, "growth_assets_pct": growth},
        "books": [
            {
                "book_id": book_id,
                "book_name": book_id.removeprefix("BOOK:")
                .replace(":", " ")
                .replace("_", " ")
                .title(),
                "trades": sorted(ids),
            }
            for book_id, ids in sorted(by_book.items())
        ],
        "trades": ordered_trades,
    }
    if portfolio_id == "adventurous_model":
        portfolio["asset_mix"]["defensive_assets_pct_range"] = [0, 5]
        portfolio["asset_mix"]["growth_assets_pct_range"] = [95, 100]
    return portfolio


def demo_product_gallery_portfolio():
    def demo_book(sleeve):
        return f"BOOK:DEMO:{sleeve}"

    def with_fields(trade, **fields):
        trade.update(fields)
        return trade

    return make_portfolio(
        "demo_portfolio",
        "Multi-Asset Product Gallery",
        50,
        50,
        "demo_gallery",
        "Institutional multi-asset demo portfolio covering every currently supported product family.",
        [
            with_fields(
                dep(
                    "demo_usd_deposit_001",
                    demo_book("RATES_CASH"),
                    "LIQUIDITY_BUFFER",
                    1_000_000,
                ),
                start_date="2026-04-15",
                maturity_date="2026-07-15",
            ),
            fra(
                "demo_usd_fra_001",
                demo_book("RATES_RELATIVE_VALUE"),
                "USD_FRA_VIEW",
                2_000_000,
            ),
            irf(
                "demo_usd_ir_future_001",
                demo_book("RATES_RELATIVE_VALUE"),
                "SOFR_FUTURE",
                4,
            ),
            swap(
                "swap_usd_001",
                demo_book("RATES_HEDGE"),
                "USD_SWAP_HEDGE",
                5_000_000,
                0.052,
                "pay_fixed",
            ),
            with_fields(
                ois(
                    "demo_usd_ois_001",
                    demo_book("RATES_HEDGE"),
                    "USD_OIS_HEDGE",
                    3_000_000,
                ),
                start_date="2026-04-15",
            ),
            bond(
                "bond_usd_001",
                demo_book("RATES_INCOME"),
                "USD_CORE_BOND",
                2_000_000,
                0.055,
            ),
            frn(
                "demo_usd_frn_001",
                demo_book("RATES_INCOME"),
                "USD_FLOATING_RATE_NOTE",
                1_500_000,
            ),
            cap(
                "demo_usd_cap_001",
                demo_book("RATES_VOLATILITY"),
                "USD_RATE_CAP",
                2_000_000,
            ),
            eswaption(
                "demo_usd_european_swaption_001",
                demo_book("RATES_VOLATILITY"),
                "USD_PAYER_SWAPTION",
                2_000_000,
            ),
            bswaption(
                "demo_usd_bermudan_swaption_001",
                demo_book("RATES_VOLATILITY"),
                "USD_BERMUDAN_SWAPTION",
                2_000_000,
            ),
            cbond(
                "demo_acme_credit_bond_001",
                demo_book("CREDIT_INCOME"),
                "ACME_CREDIT_BOND",
                1_500_000,
            ),
            cds(
                "demo_acme_cds_001",
                demo_book("CREDIT_HEDGE"),
                "ACME_CDS_PROTECTION",
                3_000_000,
            ),
            cdx(
                "demo_cdx_index_001",
                demo_book("CREDIT_BETA"),
                "CDX_IG_OVERLAY",
                4_000_000,
            ),
            cdsopt(
                "demo_acme_cds_option_001",
                demo_book("CREDIT_VOLATILITY"),
                "ACME_CDS_OPTION",
                3_000_000,
            ),
            cdxopt(
                "demo_cdx_index_option_001",
                demo_book("CREDIT_VOLATILITY"),
                "CDX_IG_INDEX_OPTION",
                5_000_000,
            ),
            fxspot(
                "demo_eurusd_spot_001", demo_book("FX_CASH"), "EURUSD_SPOT", 1_000_000
            ),
            fxfwd(
                "fx_fwd_eurusd_001",
                demo_book("FX_HEDGE"),
                "EURUSD_FORWARD_HEDGE",
                2_000_000,
                1.1025,
                "buy_base",
            ),
            fxswap(
                "demo_eurusd_swap_001",
                demo_book("FX_CARRY"),
                "EURUSD_SWAP_ROLL",
                1_500_000,
            ),
            ndf(
                "demo_eurusd_ndf_001",
                demo_book("FX_RELATIVE_VALUE"),
                "EURUSD_NDF",
                1_500_000,
            ),
            fxopt(
                "demo_eurusd_option_001",
                demo_book("FX_VOLATILITY"),
                "EURUSD_CALL",
                1_000_000,
            ),
            cspot(
                "demo_wti_spot_001",
                demo_book("COMMODITY_DELTA"),
                "WTI_SPOT",
                1000,
                77.0,
            ),
            cfwd(
                "demo_wti_forward_001",
                demo_book("COMMODITY_DELTA"),
                "WTI_FORWARD",
                1000,
                78.0,
            ),
            cfut("demo_wti_future_001", demo_book("COMMODITY_DELTA"), "WTI_FUTURE", 2),
            cstrip(
                "demo_wti_strip_001", demo_book("COMMODITY_CURVE"), "WTI_CALENDAR_STRIP"
            ),
            cfutopt(
                "demo_wti_future_option_001",
                demo_book("COMMODITY_VOLATILITY"),
                "WTI_FUTURE_OPTION",
                1,
            ),
            ccal(
                "demo_wti_calendar_option_001",
                demo_book("COMMODITY_VOLATILITY"),
                "WTI_CALENDAR_OPTION",
                1,
            ),
            swing(
                "demo_ttf_swing_001",
                demo_book("COMMODITY_FLEXIBILITY"),
                "TTF_SWING_OPTIONALITY",
            ),
            storage(
                "demo_ttf_storage_001",
                demo_book("COMMODITY_FLEXIBILITY"),
                "TTF_STORAGE_OPTIONALITY",
            ),
            eqspot(
                "equity_spot_aapl_001",
                demo_book("EQUITY_DELTA"),
                "AAPL_DELTA",
                250,
                180.0,
            ),
            eqfwd(
                "demo_aapl_forward_001",
                demo_book("EQUITY_DERIVATIVES"),
                "AAPL_FORWARD",
                200,
            ),
            eqfut(
                "demo_spx_future_001", demo_book("EQUITY_DERIVATIVES"), "SPX_FUTURE", 2
            ),
            eqopt(
                "demo_aapl_option_001",
                demo_book("EQUITY_VOLATILITY"),
                "AAPL_EUROPEAN_CALL",
                200,
            ),
        ],
    )


def manifest_for(portfolio):
    by_book = defaultdict(list)
    trades = {}
    for trade in portfolio["trades"]:
        by_book[trade["book"]].append(trade["id"])
        trades[trade["id"]] = {
            "asset_class": trade["asset_class"],
            "book": trade["book"],
            "product_type": trade["type"],
            "support_status": STATUS_BY_TYPE[trade["type"]],
        }
    ordered_trades = sorted(
        trades.items(),
        key=lambda item: hierarchy_key(
            item[1]["asset_class"], item[1]["product_type"], item[0]
        ),
    )
    return {
        "fixture_id": f"{portfolio['portfolio_id']}_coverage",
        "description": f"Portfolio-backed structural golden coverage for {portfolio['portfolio_name']}.",
        "market_path": "market/demo_market.json",
        "portfolio_id": portfolio["portfolio_id"],
        "portfolio_path": f"portfolios/{portfolio['portfolio_id']}_portfolio.json",
        "asset_mix": portfolio["asset_mix"],
        "expected_books": {
            book_id: sorted(ids) for book_id, ids in sorted(by_book.items())
        },
        "trades": {trade_id: trade for trade_id, trade in ordered_trades},
    }


def portfolios():
    return [
        make_portfolio(
            "liquidity_reserve_model",
            "Liquidity Reserve Model",
            100,
            0,
            "basic_model",
            "Cash and short-duration rates holdings for capital preservation and immediate liquidity.",
            [
                dep(
                    "lr_usd_deposit_001",
                    book("liquidity_reserve", "CASH_AND_BILLS"),
                    "CASH_LIQUIDITY",
                    5_000_000,
                ),
                bond(
                    "lr_usd_tbill_proxy_001",
                    book("liquidity_reserve", "CASH_AND_BILLS"),
                    "SHORT_DURATION",
                    1_000_000,
                    0.0525,
                    "2027-03-26",
                ),
            ],
        ),
        make_portfolio(
            "cash_plus_model",
            "Cash Plus Model",
            95,
            5,
            "basic_model",
            "Liquidity portfolio with a small liquid growth sleeve and simple FX hedge.",
            [
                dep(
                    "cp_usd_deposit_001",
                    book("cash_plus", "CASH"),
                    "CASH_LIQUIDITY",
                    4_000_000,
                ),
                frn(
                    "cp_usd_frn_001",
                    book("cash_plus", "SHORT_DURATION_INCOME"),
                    "FLOATING_RATE_INCOME",
                    1_500_000,
                    0.0008,
                ),
                eqspot(
                    "cp_aapl_overlay_001",
                    book("cash_plus", "LIQUID_GROWTH"),
                    "LOW_WEIGHT_EQUITY",
                    50,
                    184.0,
                ),
                fxfwd(
                    "cp_eurusd_hedge_001",
                    book("cash_plus", "FX_HEDGE"),
                    "EURUSD_HEDGE",
                    500_000,
                    1.101,
                    "sell_base",
                ),
            ],
        ),
        make_portfolio(
            "defensive_income_model",
            "Defensive Income Model",
            90,
            10,
            "basic_model",
            "Income-biased defensive allocation with modest credit and equity participation.",
            [
                bond(
                    "di_usd_core_bond_001",
                    book("defensive_income", "CORE_INCOME"),
                    "USD_CORE_BONDS",
                    2_500_000,
                    0.054,
                ),
                frn(
                    "di_usd_frn_001",
                    book("defensive_income", "CORE_INCOME"),
                    "FLOATING_RATE_INCOME",
                    1_500_000,
                ),
                cbond(
                    "di_acme_credit_bond_001",
                    book("defensive_income", "CREDIT_INCOME"),
                    "INVESTMENT_GRADE_CREDIT",
                    1_000_000,
                ),
                eqspot(
                    "di_aapl_quality_equity_001",
                    book("defensive_income", "QUALITY_EQUITY"),
                    "QUALITY_EQUITY",
                    75,
                    183.0,
                ),
            ],
        ),
        make_portfolio(
            "conservative_income_model",
            "Conservative Income Model",
            80,
            20,
            "basic_model",
            "Conservative multi-asset income with explicit credit protection.",
            [
                bond(
                    "ci_usd_core_bond_001",
                    book("conservative_income", "CORE_INCOME"),
                    "USD_CORE_BONDS",
                    2_000_000,
                    0.0545,
                ),
                cbond(
                    "ci_acme_credit_bond_001",
                    book("conservative_income", "CREDIT_INCOME"),
                    "INVESTMENT_GRADE_CREDIT",
                    1_500_000,
                    0.056,
                ),
                cds(
                    "ci_acme_cds_hedge_001",
                    book("conservative_income", "CREDIT_HEDGE"),
                    "CREDIT_PROTECTION",
                    1_000_000,
                ),
                eqspot(
                    "ci_aapl_income_equity_001",
                    book("conservative_income", "EQUITY_INCOME"),
                    "QUALITY_EQUITY",
                    100,
                    182.0,
                ),
                fxfwd(
                    "ci_eurusd_income_hedge_001",
                    book("conservative_income", "FX_HEDGE"),
                    "EURUSD_HEDGE",
                    750_000,
                    1.102,
                    "sell_base",
                ),
            ],
        ),
        make_portfolio(
            "cautious_model",
            "Cautious Model",
            70,
            30,
            "basic_model",
            "Cautious allocation mixing rates income, credit, FX, commodities, and equity beta.",
            [
                swap(
                    "ca_usd_duration_swap_001",
                    book("cautious", "RATES_HEDGE"),
                    "USD_DURATION",
                    2_000_000,
                    0.0525,
                    "receive_fixed",
                ),
                cbond(
                    "ca_acme_credit_bond_001",
                    book("cautious", "CREDIT_INCOME"),
                    "INVESTMENT_GRADE_CREDIT",
                    1_000_000,
                ),
                fxfwd(
                    "ca_eurusd_forward_001",
                    book("cautious", "FX_CARRY"),
                    "EURUSD_CARRY",
                    1_000_000,
                ),
                cspot(
                    "ca_wti_spot_001",
                    book("cautious", "REAL_ASSETS"),
                    "ENERGY_REAL_ASSET",
                    500,
                    76.8,
                ),
                eqspot(
                    "ca_aapl_equity_001",
                    book("cautious", "QUALITY_EQUITY"),
                    "QUALITY_EQUITY",
                    150,
                    181.0,
                ),
            ],
        ),
        make_portfolio(
            "stable_balanced_model",
            "Stable Balanced Model",
            60,
            40,
            "basic_model",
            "Balanced defensive/growth allocation with hedged rates, credit index, FX, equity, and commodities.",
            [
                ois(
                    "sb_usd_ois_001",
                    book("stable_balanced", "RATES_HEDGE"),
                    "USD_OIS_HEDGE",
                    2_000_000,
                ),
                bond(
                    "sb_usd_bond_001",
                    book("stable_balanced", "CORE_INCOME"),
                    "USD_CORE_BONDS",
                    1_500_000,
                ),
                cdx(
                    "sb_cdx_index_001",
                    book("stable_balanced", "CREDIT_BETA"),
                    "CDX_IG_OVERLAY",
                    3_000_000,
                    0.009,
                    "sell_protection",
                ),
                fxswap(
                    "sb_eurusd_swap_001",
                    book("stable_balanced", "FX_HEDGE"),
                    "EURUSD_ROLL",
                    750_000,
                    "sell_base",
                ),
                cfwd(
                    "sb_wti_forward_001",
                    book("stable_balanced", "REAL_ASSETS"),
                    "WTI_FORWARD",
                    700,
                    78.1,
                ),
                eqfut(
                    "sb_spx_future_001",
                    book("stable_balanced", "EQUITY_BETA"),
                    "SPX_BETA",
                    1,
                ),
            ],
        ),
        make_portfolio(
            "balanced_core_model",
            "Balanced Core Model",
            50,
            50,
            "basic_model",
            "Core 50/50 portfolio with liquid rates, credit, FX, equity, and commodity books.",
            [
                dep(
                    "bc_usd_deposit_001",
                    book("balanced_core", "LIQUIDITY"),
                    "CASH_BUFFER",
                ),
                bond(
                    "bc_usd_bond_001",
                    book("balanced_core", "CORE_INCOME"),
                    "USD_CORE_BONDS",
                    1_000_000,
                ),
                cbond(
                    "bc_acme_credit_bond_001",
                    book("balanced_core", "CREDIT_INCOME"),
                    "INVESTMENT_GRADE_CREDIT",
                    1_000_000,
                    0.056,
                ),
                fxspot(
                    "bc_eurusd_spot_001",
                    book("balanced_core", "FX"),
                    "EURUSD_CASH",
                    500_000,
                ),
                cfut(
                    "bc_wti_future_001",
                    book("balanced_core", "REAL_ASSETS"),
                    "WTI_FUTURE",
                    1,
                ),
                eqspot(
                    "bc_aapl_equity_001",
                    book("balanced_core", "QUALITY_EQUITY"),
                    "QUALITY_EQUITY",
                    200,
                ),
            ],
        ),
        make_portfolio(
            "balanced_growth_model",
            "Balanced Growth Model",
            40,
            60,
            "basic_model",
            "Growth-tilted balanced allocation using options for convexity and hedging.",
            [
                cap(
                    "bg_usd_cap_001",
                    book("balanced_growth", "RATES_OPTION_HEDGE"),
                    "USD_RATE_CAP",
                    2_000_000,
                ),
                cds(
                    "bg_acme_cds_001",
                    book("balanced_growth", "CREDIT_HEDGE"),
                    "CREDIT_PROTECTION",
                    1_000_000,
                ),
                fxopt(
                    "bg_eurusd_option_001",
                    book("balanced_growth", "FX_VOLATILITY"),
                    "EURUSD_CALL",
                    750_000,
                ),
                cstrip(
                    "bg_wti_strip_001",
                    book("balanced_growth", "REAL_ASSETS"),
                    "WTI_CALENDAR_STRIP",
                ),
                eqfwd(
                    "bg_aapl_forward_001",
                    book("balanced_growth", "EQUITY_DERIVATIVES"),
                    "AAPL_FORWARD",
                    150,
                ),
            ],
        ),
        make_portfolio(
            "growth_model",
            "Growth Model",
            30,
            70,
            "basic_model",
            "Growth allocation emphasizing equity beta, commodity convexity, FX optionality, and macro hedges.",
            [
                eswaption(
                    "gr_usd_swaption_001",
                    book("growth", "MACRO_HEDGE"),
                    "USD_PAYER_SWAPTION",
                    2_000_000,
                ),
                cdxopt(
                    "gr_cdx_option_001",
                    book("growth", "CREDIT_VOLATILITY"),
                    "CDX_IG_CALL",
                    3_000_000,
                ),
                fxopt(
                    "gr_eurusd_option_001",
                    book("growth", "FX_VOLATILITY"),
                    "EURUSD_CALL",
                ),
                cfutopt(
                    "gr_wti_option_001",
                    book("growth", "COMMODITY_VOLATILITY"),
                    "WTI_UPSIDE",
                ),
                eqfut(
                    "gr_spx_future_001", book("growth", "EQUITY_BETA"), "SPX_BETA", 2
                ),
                eqopt(
                    "gr_aapl_call_001",
                    book("growth", "EQUITY_VOLATILITY"),
                    "AAPL_CALL",
                    150,
                ),
            ],
        ),
        make_portfolio(
            "high_growth_model",
            "High Growth Model",
            20,
            80,
            "basic_model",
            "High-growth allocation with concentrated equity, commodity, credit, and FX volatility books.",
            [
                ois(
                    "hg_usd_ois_001",
                    book("high_growth", "RATES_HEDGE"),
                    "USD_OIS_HEDGE",
                    1_000_000,
                ),
                cdsopt(
                    "hg_acme_cds_option_001",
                    book("high_growth", "CREDIT_VOLATILITY"),
                    "ACME_CDS_CALL",
                    2_000_000,
                ),
                fxfwd(
                    "hg_eurusd_forward_001",
                    book("high_growth", "FX_CARRY"),
                    "EURUSD_CARRY",
                    1_000_000,
                    1.103,
                ),
                ccal(
                    "hg_wti_calendar_001",
                    book("high_growth", "COMMODITY_VOLATILITY"),
                    "WTI_CALENDAR_VOL",
                ),
                eqfwd(
                    "hg_aapl_forward_001",
                    book("high_growth", "EQUITY_DERIVATIVES"),
                    "AAPL_FORWARD",
                    200,
                ),
                eqopt(
                    "hg_aapl_put_001",
                    book("high_growth", "EQUITY_VOLATILITY"),
                    "AAPL_AMERICAN_PUT",
                    150,
                    190.0,
                    "put",
                    "american",
                ),
            ],
        ),
        make_portfolio(
            "aggressive_growth_model",
            "Aggressive Growth Model",
            10,
            90,
            "basic_model",
            "Aggressive portfolio dominated by derivative growth and volatility exposures.",
            [
                bswaption(
                    "ag_usd_bermudan_001",
                    book("aggressive_growth", "RATES_OPTION_HEDGE"),
                    "USD_BERMUDAN_SWAPTION",
                    2_000_000,
                ),
                cdxopt(
                    "ag_cdx_option_001",
                    book("aggressive_growth", "CREDIT_VOLATILITY"),
                    "CDX_IG_CALL",
                    4_000_000,
                ),
                fxswap(
                    "ag_eurusd_swap_001",
                    book("aggressive_growth", "FX_CARRY"),
                    "EURUSD_SWAP",
                    1_000_000,
                ),
                cfutopt(
                    "ag_wti_option_001",
                    book("aggressive_growth", "COMMODITY_VOLATILITY"),
                    "WTI_UPSIDE",
                    2,
                ),
                ccal(
                    "ag_wti_calendar_001",
                    book("aggressive_growth", "COMMODITY_VOLATILITY"),
                    "WTI_CALENDAR_VOL",
                    2,
                ),
                eqopt(
                    "ag_aapl_call_001",
                    book("aggressive_growth", "EQUITY_VOLATILITY"),
                    "AAPL_CALL",
                    250,
                ),
            ],
        ),
        make_portfolio(
            "adventurous_model",
            "Adventurous Model",
            5,
            95,
            "basic_model",
            "Adventurous allocation with maximum growth and alternative risk exposures.",
            [
                fra(
                    "ad_usd_fra_001",
                    book("adventurous", "RATES_RELATIVE_VALUE"),
                    "USD_FRA_VIEW",
                    2_000_000,
                ),
                ndf(
                    "ad_eurusd_ndf_001",
                    book("adventurous", "FX_RELATIVE_VALUE"),
                    "EURUSD_NDF",
                    1_500_000,
                ),
                cdsopt(
                    "ad_acme_cds_option_001",
                    book("adventurous", "CREDIT_VOLATILITY"),
                    "ACME_CDS_CALL",
                    3_000_000,
                ),
                swing(
                    "ad_ttf_swing_001",
                    book("adventurous", "COMMODITY_FLEXIBILITY"),
                    "TTF_SWING_OPTIONALITY",
                ),
                cfutopt(
                    "ad_wti_option_001",
                    book("adventurous", "COMMODITY_VOLATILITY"),
                    "WTI_UPSIDE",
                    2,
                ),
                eqopt(
                    "ad_aapl_american_put_001",
                    book("adventurous", "EQUITY_VOLATILITY"),
                    "AAPL_AMERICAN_PUT",
                    250,
                    190.0,
                    "put",
                    "american",
                ),
            ],
        ),
        make_portfolio(
            "stable_balanced_rates_hedged",
            "Stable Balanced Rates Hedged",
            60,
            40,
            "thematic_variant",
            "Rates-focused balanced portfolio covering cash, curve, bond, cap/floor, and swaption products.",
            [
                dep(
                    "sbrh_deposit_001",
                    book("stable_balanced", "RATES_CASH"),
                    "CASH_LIQUIDITY",
                ),
                fra(
                    "sbrh_fra_001",
                    book("stable_balanced", "RATES_RELATIVE_VALUE"),
                    "USD_FRA",
                ),
                irf(
                    "sbrh_ir_future_001",
                    book("stable_balanced", "RATES_RELATIVE_VALUE"),
                    "SOFR_FUTURE",
                    5,
                ),
                swap(
                    "sbrh_swap_001", book("stable_balanced", "RATES_HEDGE"), "USD_SWAP"
                ),
                ois(
                    "sbrh_ois_001",
                    book("stable_balanced", "RATES_HEDGE"),
                    "USD_OIS",
                    5_000_000,
                ),
                bond(
                    "sbrh_fixed_bond_001",
                    book("stable_balanced", "RATES_INCOME"),
                    "USD_FIXED_BOND",
                ),
                frn("sbrh_frn_001", book("stable_balanced", "RATES_INCOME"), "USD_FRN"),
                cap(
                    "sbrh_cap_001",
                    book("stable_balanced", "RATES_VOLATILITY"),
                    "USD_CAP",
                    3_000_000,
                ),
                eswaption(
                    "sbrh_european_swaption_001",
                    book("stable_balanced", "RATES_VOLATILITY"),
                    "USD_PAYER_SWAPTION",
                    4_000_000,
                ),
                bswaption(
                    "sbrh_bermudan_swaption_001",
                    book("stable_balanced", "RATES_VOLATILITY"),
                    "USD_BERMUDAN_SWAPTION",
                    4_000_000,
                ),
            ],
        ),
        make_portfolio(
            "cautious_fx_carry",
            "Cautious FX Carry",
            70,
            30,
            "thematic_variant",
            "Cautious FX book covering spot, forwards, swaps, NDFs, and vanilla options.",
            [
                fxspot(
                    "cfx_eurusd_spot_001", book("cautious", "FX_CASH"), "EURUSD_SPOT"
                ),
                fxfwd(
                    "cfx_eurusd_forward_001",
                    book("cautious", "FX_CARRY"),
                    "EURUSD_FORWARD",
                ),
                fxswap(
                    "cfx_eurusd_swap_001", book("cautious", "FX_CARRY"), "EURUSD_SWAP"
                ),
                ndf(
                    "cfx_eurusd_ndf_001",
                    book("cautious", "FX_RELATIVE_VALUE"),
                    "EURUSD_NDF",
                ),
                fxopt(
                    "cfx_eurusd_option_001",
                    book("cautious", "FX_VOLATILITY"),
                    "EURUSD_CALL",
                ),
            ],
        ),
        make_portfolio(
            "conservative_credit_income",
            "Conservative Credit Income",
            80,
            20,
            "thematic_variant",
            "Conservative credit portfolio covering bonds, CDS, indices, and spread options.",
            [
                cbond(
                    "cci_acme_credit_bond_001",
                    book("conservative_income", "CREDIT_INCOME"),
                    "INVESTMENT_GRADE_CREDIT",
                ),
                cds(
                    "cci_acme_cds_001",
                    book("conservative_income", "CREDIT_HEDGE"),
                    "ACME_PROTECTION",
                    5_000_000,
                ),
                cdx(
                    "cci_cdx_index_001",
                    book("conservative_income", "CREDIT_BETA"),
                    "CDX_IG_OVERLAY",
                    10_000_000,
                ),
                cdsopt(
                    "cci_acme_cds_option_001",
                    book("conservative_income", "CREDIT_VOLATILITY"),
                    "ACME_CDS_CALL",
                    5_000_000,
                ),
                cdxopt(
                    "cci_cdx_option_001",
                    book("conservative_income", "CREDIT_VOLATILITY"),
                    "CDX_IG_CALL",
                    10_000_000,
                ),
            ],
        ),
        make_portfolio(
            "balanced_multi_asset_income",
            "Balanced Multi-Asset Income",
            50,
            50,
            "thematic_variant",
            "Balanced income portfolio spanning rates, credit, FX hedge, real assets, and quality equity.",
            [
                dep(
                    "bmai_deposit_001",
                    book("balanced_core", "LIQUIDITY"),
                    "CASH_BUFFER",
                ),
                bond(
                    "bmai_fixed_bond_001",
                    book("balanced_core", "CORE_INCOME"),
                    "USD_CORE_BONDS",
                ),
                frn(
                    "bmai_frn_001",
                    book("balanced_core", "CORE_INCOME"),
                    "USD_FRN",
                    1_500_000,
                ),
                cbond(
                    "bmai_credit_bond_001",
                    book("balanced_core", "CREDIT_INCOME"),
                    "INVESTMENT_GRADE_CREDIT",
                    1_500_000,
                    0.056,
                ),
                cdx(
                    "bmai_cdx_index_001",
                    book("balanced_core", "CREDIT_BETA"),
                    "CDX_IG_OVERLAY",
                    3_000_000,
                    0.009,
                    "sell_protection",
                ),
                fxfwd(
                    "bmai_eurusd_hedge_001",
                    book("balanced_core", "FX_HEDGE"),
                    "EURUSD_HEDGE",
                    750_000,
                    1.102,
                    "sell_base",
                ),
                cfwd(
                    "bmai_wti_forward_001",
                    book("balanced_core", "REAL_ASSETS"),
                    "WTI_FORWARD",
                    500,
                    78.1,
                ),
                eqspot(
                    "bmai_aapl_equity_001",
                    book("balanced_core", "QUALITY_EQUITY"),
                    "QUALITY_EQUITY",
                    150,
                    181.0,
                ),
            ],
        ),
        make_portfolio(
            "growth_global_macro",
            "Growth Global Macro",
            30,
            70,
            "thematic_variant",
            "Growth global macro portfolio with rates, FX, credit, commodity, and equity derivative books.",
            [
                swap(
                    "ggm_usd_swap_001",
                    book("growth", "GLOBAL_MACRO_RATES"),
                    "USD_CURVE_VIEW",
                    4_000_000,
                    0.0525,
                ),
                eswaption(
                    "ggm_usd_swaption_001",
                    book("growth", "GLOBAL_MACRO_RATES"),
                    "USD_RATE_CONVEXITY",
                ),
                fxfwd(
                    "ggm_eurusd_forward_001",
                    book("growth", "GLOBAL_MACRO_FX"),
                    "EURUSD_FORWARD_CARRY",
                ),
                fxopt(
                    "ggm_eurusd_option_001",
                    book("growth", "GLOBAL_MACRO_FX"),
                    "EURUSD_CALL",
                ),
                cdx(
                    "ggm_cdx_index_001",
                    book("growth", "GLOBAL_MACRO_CREDIT"),
                    "CDX_IG_OVERLAY",
                ),
                cspot(
                    "ggm_wti_spot_001",
                    book("growth", "GLOBAL_MACRO_COMMODITY"),
                    "WTI_DIRECTIONAL",
                    2000,
                    76.8,
                ),
                cfwd(
                    "ggm_wti_forward_001",
                    book("growth", "GLOBAL_MACRO_COMMODITY"),
                    "WTI_FORWARD_CARRY",
                    1500,
                    77.9,
                ),
                cfut(
                    "ggm_wti_future_001",
                    book("growth", "GLOBAL_MACRO_COMMODITY"),
                    "WTI_LISTED_FUTURES",
                    3,
                    78.8,
                    "short",
                ),
                cstrip(
                    "ggm_wti_strip_001",
                    book("growth", "GLOBAL_MACRO_COMMODITY"),
                    "WTI_CALENDAR_STRIP",
                    2,
                    78.65,
                ),
                cfutopt(
                    "ggm_wti_option_001",
                    book("growth", "COMMODITY_VOLATILITY"),
                    "WTI_UPSIDE_CONVEXITY",
                    4,
                ),
                ccal(
                    "ggm_wti_calendar_001",
                    book("growth", "COMMODITY_VOLATILITY"),
                    "WTI_CALENDAR_VOL",
                    3,
                ),
                swing(
                    "ggm_ttf_swing_001",
                    book("growth", "COMMODITY_FLEXIBILITY"),
                    "TTF_SWING_OPTIONALITY",
                ),
                eqfwd(
                    "ggm_aapl_forward_001",
                    book("growth", "EQUITY_DERIVATIVES"),
                    "AAPL_FORWARD_CARRY",
                    1000,
                    191.0,
                ),
                eqfut(
                    "ggm_spx_future_001",
                    book("growth", "EQUITY_DERIVATIVES"),
                    "SPX_BETA_OVERLAY",
                    2,
                ),
                eqopt(
                    "ggm_aapl_call_001",
                    book("growth", "EQUITY_VOLATILITY"),
                    "AAPL_EUROPEAN_CALL",
                    1000,
                    190.0,
                ),
                eqopt(
                    "ggm_aapl_put_001",
                    book("growth", "EQUITY_VOLATILITY"),
                    "AAPL_AMERICAN_PUT",
                    1000,
                    185.0,
                    "put",
                    "american",
                ),
            ],
        ),
        make_portfolio(
            "high_growth_equity_volatility",
            "High Growth Equity Volatility",
            20,
            80,
            "thematic_variant",
            "High-growth equity portfolio covering spot, forwards, futures, and European/American option risk.",
            [
                eqspot(
                    "hgev_aapl_spot_001",
                    book("high_growth", "EQUITY_DELTA"),
                    "AAPL_DELTA",
                    300,
                ),
                eqfwd(
                    "hgev_aapl_forward_001",
                    book("high_growth", "EQUITY_DERIVATIVES"),
                    "AAPL_FORWARD",
                    250,
                ),
                eqfut(
                    "hgev_spx_future_001",
                    book("high_growth", "EQUITY_DERIVATIVES"),
                    "SPX_BETA",
                    3,
                ),
                eqopt(
                    "hgev_aapl_call_001",
                    book("high_growth", "EQUITY_VOLATILITY"),
                    "AAPL_EUROPEAN_CALL",
                    250,
                ),
                eqopt(
                    "hgev_aapl_euro_put_001",
                    book("high_growth", "EQUITY_VOLATILITY"),
                    "AAPL_EUROPEAN_PUT",
                    250,
                    190.0,
                    "put",
                ),
                eqopt(
                    "hgev_aapl_american_put_001",
                    book("high_growth", "EQUITY_VOLATILITY"),
                    "AAPL_AMERICAN_PUT",
                    250,
                    190.0,
                    "put",
                    "american",
                ),
            ],
        ),
        make_portfolio(
            "adventurous_commodity_volatility",
            "Adventurous Commodity Volatility",
            5,
            95,
            "thematic_variant",
            (
                "Adventurous commodity portfolio covering spot, forwards, futures, strips, "
                "future options, spread options, and swing optionality."
            ),
            [
                cspot(
                    "acv_wti_spot_001",
                    book("adventurous", "COMMODITY_DELTA"),
                    "WTI_SPOT",
                ),
                cfwd(
                    "acv_wti_forward_001",
                    book("adventurous", "COMMODITY_DELTA"),
                    "WTI_FORWARD",
                ),
                cfut(
                    "acv_wti_future_001",
                    book("adventurous", "COMMODITY_DELTA"),
                    "WTI_FUTURE",
                    3,
                ),
                cstrip(
                    "acv_wti_strip_001",
                    book("adventurous", "COMMODITY_CURVE"),
                    "WTI_CALENDAR_STRIP",
                ),
                cfutopt(
                    "acv_wti_option_001",
                    book("adventurous", "COMMODITY_VOLATILITY"),
                    "WTI_UPSIDE",
                    2,
                ),
                ccal(
                    "acv_wti_calendar_001",
                    book("adventurous", "COMMODITY_VOLATILITY"),
                    "WTI_CALENDAR_VOL",
                    2,
                ),
                swing(
                    "acv_ttf_swing_001",
                    book("adventurous", "COMMODITY_FLEXIBILITY"),
                    "TTF_SWING_OPTIONALITY",
                ),
            ],
        ),
    ]


def write_json(path: Path, payload: dict) -> None:
    path.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")


def main() -> None:
    PORTFOLIO_DIR.mkdir(parents=True, exist_ok=True)
    FIXTURE_DIR.mkdir(parents=True, exist_ok=True)

    write_json(PORTFOLIO_DIR / "demo_portfolio.json", demo_product_gallery_portfolio())

    generated = portfolios()
    portfolio_ids = [portfolio["portfolio_id"] for portfolio in generated]
    if len(portfolio_ids) != len(set(portfolio_ids)):
        raise RuntimeError("duplicate portfolio id in generated fixtures")
    for portfolio in generated:
        trade_ids = [trade["id"] for trade in portfolio["trades"]]
        if len(trade_ids) != len(set(trade_ids)):
            raise RuntimeError(f"duplicate trade id in {portfolio['portfolio_id']}")
        write_json(
            PORTFOLIO_DIR / f"{portfolio['portfolio_id']}_portfolio.json", portfolio
        )
        write_json(
            FIXTURE_DIR / f"{portfolio['portfolio_id']}_coverage.json",
            manifest_for(portfolio),
        )

    print(f"wrote {len(generated)} portfolios and {len(generated)} portfolio fixtures")


if __name__ == "__main__":
    main()
