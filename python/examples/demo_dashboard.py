"""Creates the standalone Plotly HTML dashboard for the demo analytics workflow."""

import argparse
import html
import json
import math
import webbrowser
from collections import defaultdict
from datetime import date, datetime, timedelta
from pathlib import Path

FINANCE_FONT = '"IBM Plex Sans", "Aptos", "Bahnschrift", sans-serif'
MONO_FONT = '"IBM Plex Mono", "Cascadia Mono", "SFMono-Regular", monospace'
CATEGORY_TICKANGLE = -35
CATEGORY_TICKFONT = {"family": FINANCE_FONT, "size": 9}
DASHBOARD_PORTFOLIOS = [
    "liquidity_reserve_model_portfolio.json",
    "balanced_core_model_portfolio.json",
    "growth_global_macro_portfolio.json",
    "high_growth_equity_volatility_portfolio.json",
    "adventurous_commodity_volatility_portfolio.json",
]

DASHBOARD_ASSET_DIR = Path(__file__).with_name("dashboard")

DASHBOARD_STYLE_PRESETS = {
    "institutional": {
        "accent": "#5faea4",
        "accent2": "#7aa7d9",
        "amber": "#d5ac63",
        "asset_colors": {
            "commodity": "#dfbf72",
            "credit": "#bba1d9",
            "crypto": "#9bc7d8",
            "equity": "#e49a9d",
            "fx": "#94b9e2",
            "inflation": "#e5b08b",
            "rates": "#7bc5ba",
            "volatility": "#9bcfaa",
        },
        "colorway": [
            "#7bc5ba",
            "#94b9e2",
            "#dfbf72",
            "#e49a9d",
            "#bba1d9",
            "#9bcfaa",
            "#9bc7d8",
        ],
        "danger": "#d3777c",
        "fill_text": "#14231e",
        "label": "Institutional",
    },
    "aurora": {
        "accent": "#73d6cf",
        "accent2": "#b09ae3",
        "amber": "#efc66f",
        "asset_colors": {
            "commodity": "#f3d184",
            "credit": "#c1abea",
            "crypto": "#9edcf2",
            "equity": "#f3a7b5",
            "fx": "#abb9ee",
            "inflation": "#f4b9a5",
            "rates": "#84ded8",
            "volatility": "#bfdd8b",
        },
        "colorway": [
            "#84ded8",
            "#abb9ee",
            "#f3d184",
            "#f3a7b5",
            "#c1abea",
            "#bfdd8b",
            "#9edcf2",
        ],
        "danger": "#e98295",
        "fill_text": "#132421",
        "label": "Aurora",
    },
    "terminal": {
        "accent": "#22c55e",
        "accent2": "#06b6d4",
        "amber": "#eab308",
        "asset_colors": {
            "commodity": "#eab308",
            "credit": "#a3e635",
            "crypto": "#14b8a6",
            "equity": "#ef4444",
            "fx": "#06b6d4",
            "inflation": "#f97316",
            "rates": "#22c55e",
            "volatility": "#84cc16",
        },
        "colorway": [
            "#22c55e",
            "#06b6d4",
            "#eab308",
            "#ef4444",
            "#a3e635",
            "#14b8a6",
            "#f97316",
        ],
        "danger": "#ef4444",
        "fill_text": "#03120a",
        "label": "Terminal",
    },
    "nordic": {
        "accent": "#76cfc4",
        "accent2": "#8eb5e6",
        "amber": "#d7bd72",
        "asset_colors": {
            "commodity": "#dec982",
            "credit": "#c0b2e2",
            "crypto": "#9dd3e8",
            "equity": "#eaa5a2",
            "fx": "#a5c4ea",
            "inflation": "#e8bb94",
            "rates": "#93d6cc",
            "volatility": "#a6d7bd",
        },
        "colorway": [
            "#93d6cc",
            "#a5c4ea",
            "#dec982",
            "#eaa5a2",
            "#c0b2e2",
            "#a6d7bd",
            "#9dd3e8",
        ],
        "danger": "#de8580",
        "fill_text": "#10231f",
        "label": "Nordic",
    },
}
DEFAULT_STYLE_PRESET = "institutional"
DASHBOARD_COLORWAY = DASHBOARD_STYLE_PRESETS[DEFAULT_STYLE_PRESET]["colorway"]
NEGATIVE_COLOR = DASHBOARD_STYLE_PRESETS[DEFAULT_STYLE_PRESET]["danger"]
POSITIVE_COLOR = DASHBOARD_STYLE_PRESETS[DEFAULT_STYLE_PRESET]["accent"]
WARNING_COLOR = DASHBOARD_STYLE_PRESETS[DEFAULT_STYLE_PRESET]["amber"]
DIVERGING_SCALE = [
    [0.0, NEGATIVE_COLOR],
    [0.5, "#f6ead6"],
    [1.0, POSITIVE_COLOR],
]
PRODUCT_COLOR_VARIANTS = [
    ("#000000", 0.10),
    ("#ffffff", 0.12),
    ("#000000", 0.22),
    ("#ffffff", 0.28),
    ("#000000", 0.32),
    ("#ffffff", 0.38),
]
TRADE_COLOR_VARIANTS = [
    ("#000000", 0.04),
    ("#000000", 0.08),
    ("#000000", 0.12),
    ("#000000", 0.16),
]
SUPPORT_STATUS_ORDER = [
    "supported",
    "partially_supported",
    "unsupported",
    "not_supported",
    "unknown",
]


def optional_plotly_modules():
    try:
        import plotly.graph_objects as go
        import plotly.io as pio
        from plotly.subplots import make_subplots
    except ImportError as exc:
        print(f"Skipped: Plotly dashboard dependencies are not available in this Python environment ({exc}).")
        print("Install the optional dashboard dependencies with:")
        print("  uv sync --project python --extra dashboard")
        print("Then run the dashboard with the uv-managed interpreter:")
        print("  uv run --project python --extra dashboard python python/examples/demo_dashboard.py")
        return None
    return go, make_subplots, pio


def read_dashboard_asset(name):
    return (DASHBOARD_ASSET_DIR / name).read_text(encoding="utf-8")


def json_for_script(value):
    return (
        json.dumps(value, sort_keys=True)
        .replace("&", "\\u0026")
        .replace("<", "\\u003c")
        .replace(">", "\\u003e")
        .replace("\u2028", "\\u2028")
        .replace("\u2029", "\\u2029")
    )


def render_asset_template(template, replacements):
    rendered = template
    for token, value in replacements.items():
        rendered = rendered.replace(token, value)
    missing = sorted(token for token in replacements if token in rendered)
    if missing:
        raise ValueError(f"Dashboard template placeholders were not replaced: {missing}")
    return rendered


def render_select_option(template, value, label):
    return render_asset_template(
        template,
        {
            "__QRP_OPTION_LABEL__": html.escape(label),
            "__QRP_OPTION_VALUE__": html.escape(value),
        },
    )


def html_id(value):
    text = str(value or "").strip().casefold()
    cleaned = "".join(char if char.isalnum() else "-" for char in text)
    collapsed = "-".join(part for part in cleaned.split("-") if part)
    return collapsed or "item"


def dashboard_style_options(option_template):
    return "\n".join(
        render_select_option(option_template, key, preset["label"]) for key, preset in DASHBOARD_STYLE_PRESETS.items()
    )


def active_style_preset():
    return DASHBOARD_STYLE_PRESETS[DEFAULT_STYLE_PRESET]


def style_color(index):
    colorway = active_style_preset()["colorway"]
    return colorway[index % len(colorway)]


def stable_color_index(value, modulo):
    if modulo <= 0:
        return 0
    return sum((index + 1) * ord(char) for index, char in enumerate(str(value))) % modulo


def hex_to_rgb(color):
    clean = str(color or "").lstrip("#")
    if len(clean) != 6:
        return None
    try:
        return tuple(int(clean[index : index + 2], 16) for index in (0, 2, 4))
    except ValueError:
        return None


def mix_hex(base_color, mix_color, weight):
    base_rgb = hex_to_rgb(base_color)
    mix_rgb = hex_to_rgb(mix_color)
    if base_rgb is None or mix_rgb is None:
        return base_color
    clamped = max(0.0, min(1.0, float(weight)))
    mixed = [round(base * (1.0 - clamped) + mix * clamped) for base, mix in zip(base_rgb, mix_rgb)]
    return "#{:02x}{:02x}{:02x}".format(*mixed)


def node_asset_class(node_id):
    node = str(node_id or "")
    if node.startswith("asset:"):
        return node.removeprefix("asset:")
    if node.startswith("product:"):
        parts = node.split(":")
        return parts[1] if len(parts) > 1 else ""
    if node.startswith("trade:"):
        parts = node.split(":")
        return parts[1] if len(parts) > 3 else ""
    return ""


def product_node_id_from_trade_node(node_id):
    parts = str(node_id or "").split(":")
    if len(parts) < 4 or parts[0] != "trade":
        return ""
    return f"product:{parts[1]}:{parts[2]}"


def trade_node_id(row):
    return f"trade:{row['asset_class']}:{row['product_type']}:{row['trade_id']}"


def product_family_color(asset_color, product_node_id):
    mix_color, weight = PRODUCT_COLOR_VARIANTS[stable_color_index(product_node_id, len(PRODUCT_COLOR_VARIANTS))]
    return mix_hex(asset_color, mix_color, weight)


def trade_family_color(product_color, trade_node_id):
    mix_color, weight = TRADE_COLOR_VARIANTS[stable_color_index(trade_node_id, len(TRADE_COLOR_VARIANTS))]
    return mix_hex(product_color, mix_color, weight)


def semantic_node_color(node_id, index=0):
    preset = active_style_preset()
    node = str(node_id or "")
    if node == "portfolio":
        return preset["amber"]

    asset_class = node_asset_class(node)
    if node.startswith("product:"):
        asset_color = preset["asset_colors"].get(asset_class, style_color(index))
        return product_family_color(asset_color, node)
    if node.startswith("trade:"):
        asset_color = preset["asset_colors"].get(asset_class, style_color(index))
        product_color = product_family_color(asset_color, product_node_id_from_trade_node(node))
        return trade_family_color(product_color, node)
    return preset["asset_colors"].get(asset_class, style_color(index))


def trade_metric_color(row):
    if float(row.get("display_npv", row.get("npv", 0.0)) or 0.0) < 0.0:
        return active_style_preset()["danger"]
    return semantic_node_color(trade_node_id(row))


def support_status_color(status):
    preset = active_style_preset()
    normalized = str(status or "").lower()
    if "partial" in normalized:
        return preset["amber"]
    if any(token in normalized for token in ("error", "missing", "not_supported", "unsupported")):
        return preset["danger"]
    if "supported" in normalized:
        return preset["accent"]
    return preset["accent2"]


def support_status_label(status):
    return str(status or "unknown").replace("_", " ").title()


def support_status_text(status, count, total):
    if count <= 0:
        return ""
    share = 100.0 * count / total if total else 0.0
    return f"{support_status_label(status)}: {count:.0f} / {share:.0f}%"


def support_statuses_from_counts(status_counts):
    known = [status for status in SUPPORT_STATUS_ORDER if status in status_counts]
    extras = sorted(status for status in status_counts if status not in SUPPORT_STATUS_ORDER)
    return known + extras


def sort_text(value):
    return str(value or "").casefold()


def split_identifier(value):
    return [token for token in str(value or "").replace(":", "_").replace("-", "_").split("_") if token]


def title_token(value):
    return str(value or "").replace("_", " ").title()


def trade_sort_key(row):
    return (
        sort_text(row.get("asset_class")),
        sort_text(row.get("product_type")),
        sort_text(row.get("trade_id")),
    )


def sort_trade_rows(rows):
    return sorted(rows, key=trade_sort_key)


def trade_id_sort_key(trade_id, trade_rows):
    by_trade = {row["trade_id"]: row for row in trade_rows}
    return trade_sort_key(by_trade.get(trade_id, {"trade_id": trade_id}))


def trade_row_for_id(trade_id, trade_rows):
    by_trade = {row["trade_id"]: row for row in trade_rows}
    return by_trade.get(
        trade_id,
        {"asset_class": "", "product_type": "", "trade_id": trade_id},
    )


def trade_hierarchy_axis(trade_ids, trade_rows):
    rows = [trade_row_for_id(trade_id, trade_rows) for trade_id in trade_ids]
    return [
        [row.get("asset_class", "") for row in rows],
        [row.get("product_type", "") for row in rows],
        [row.get("trade_id", "") for row in rows],
    ]


def trade_hierarchy_labels(trade_ids, trade_rows):
    rows = [trade_row_for_id(trade_id, trade_rows) for trade_id in trade_ids]
    return [f"{row.get('asset_class', '')} / {row.get('product_type', '')} / {row.get('trade_id', '')}" for row in rows]


def tenor_days(value):
    token = str(value or "").strip().upper()
    if token == "SPOT":
        return 0.0
    if len(token) < 2:
        return None
    unit = token[-1]
    multiplier = {"D": 1.0, "W": 7.0, "M": 30.0, "Y": 365.0}.get(unit)
    if multiplier is None:
        return None
    try:
        return float(token[:-1]) * multiplier
    except ValueError:
        return None


def tenor_sort_key(value):
    days = tenor_days(value)
    if days is None:
        return (1, sort_text(value))
    return (0, days)


def identifier_tenor_sort_key(value):
    for token in reversed(split_identifier(value)):
        if tenor_days(token) is not None:
            return (0, *tenor_sort_key(token))
    return (1, sort_text(value))


RISK_BUCKET_GROUP_ORDER = {
    "Spot": 0,
    "Short Tenor": 1,
    "Medium Tenor": 2,
    "Long Tenor": 3,
    "Other": 9,
}


def risk_bucket_group(value):
    days = tenor_days(value)
    if days is None:
        return "Other"
    if days == 0.0:
        return "Spot"
    if days <= 180.0:
        return "Short Tenor"
    if days <= 3.0 * 365.0:
        return "Medium Tenor"
    return "Long Tenor"


def risk_bucket_sort_key(value):
    group = risk_bucket_group(value)
    return (RISK_BUCKET_GROUP_ORDER.get(group, 9), *tenor_sort_key(value))


def risk_bucket_hierarchy_axis(values):
    return [
        [risk_bucket_group(value) for value in values],
        [str(value or "") for value in values],
    ]


def scenario_group_label(scenario_name):
    tokens = [token.upper() for token in split_identifier(scenario_name)]
    if not tokens:
        return "Other"
    if tokens[0] == "CROSS":
        return "Cross Asset"
    if tokens[0] == "COMMODITY":
        return "Commodity"
    if tokens[0] == "CREDIT":
        return "Credit"
    if tokens[0] == "EQUITY":
        return "Equity"
    if tokens[0] == "FX" or (tokens[0] == "USD" and len(tokens) > 1 and tokens[1] in {"STRENGTH", "WEAKNESS"}):
        return "FX"
    if tokens[0] == "GLOBAL":
        return "Global"
    if tokens[0] == "VOL":
        return "Volatility"
    if tokens[0] in {"AUD", "CAD", "CHF", "EUR", "GBP", "JPY", "USD"}:
        if tokens[1:2] and tokens[1] in {"CURVE", "FUNDING", "LIBOR", "OIS", "RATES"}:
            return f"{tokens[0]} Rates"
        return "FX"
    return title_token(tokens[0])


def scenario_sort_key(scenario_name):
    return (
        sort_text(scenario_group_label(scenario_name)),
        tuple(sort_text(token) for token in split_identifier(scenario_name)),
    )


def scenario_hierarchy_axis(scenario_names):
    return [
        [scenario_group_label(name) for name in scenario_names],
        [axis_tick_label(name, max_line_length=16, max_lines=2) for name in scenario_names],
    ]


def scenario_hierarchy_labels(scenario_names):
    return [f"{scenario_group_label(name)} / {name}" for name in scenario_names]


def bounded_chart_height(
    item_count,
    *,
    base_height,
    row_height,
    min_height=420,
    max_height=1180,
):
    return max(min_height, min(max_height, base_height + int(item_count) * row_height))


def categorical_left_margin(labels, *, min_margin=72, max_margin=260, char_width=6):
    longest = max((len(str(label)) for label in labels), default=0)
    return max(min_margin, min(max_margin, 28 + longest * char_width))


def bold_chart_text(value):
    """Return Plotly-safe bold text for chart titles and subplot annotations."""
    return f"<b>{html.escape(str(value or ''), quote=False)}</b>"


def axis_tick_label(value, *, max_line_length=18, max_lines=3):
    """Wrap long categorical chart labels at natural separators."""
    raw_value = str(value or "")
    tokens = [token for token in raw_value.replace(":", "_").split("_") if token]
    if not tokens:
        return raw_value
    lines = []
    current_tokens = []
    consumed_tokens = 0
    for token in tokens:
        candidate_tokens = [*current_tokens, token]
        candidate = "_".join(candidate_tokens)
        if current_tokens and len(candidate) > max_line_length:
            line = "_".join(current_tokens)
            if len(line) > max_line_length:
                line = f"{line[:max_line_length]}..."
            lines.append(line)
            current_tokens = [token]
            consumed_tokens += 1
            if len(lines) >= max_lines - 1:
                break
            continue
        current_tokens = candidate_tokens
        consumed_tokens += 1
        if len(lines) >= max_lines - 1:
            break
    if current_tokens and len(lines) < max_lines:
        line = "_".join(current_tokens)
        if len(line) > max_line_length:
            line = f"{line[:max_line_length]}..."
        lines.append(line)
    if consumed_tokens < len(tokens) and len(lines) == max_lines:
        lines[-1] = f"{lines[-1].rstrip('.')}..."
    return "<br>".join(lines)


def diagonal_category_axis(*, title_text=None, multicategory=False):
    kwargs = {
        "automargin": True,
        "tickangle": CATEGORY_TICKANGLE,
        "tickfont": CATEGORY_TICKFONT,
    }
    if title_text:
        kwargs["title_text"] = title_text
    if multicategory:
        kwargs["type"] = "multicategory"
    return kwargs


def support_diagnostic_rows(trade_rows):
    return [row for row in trade_rows if row["status"] != "supported"]


def padded_number_range(values, minimum_span=1.0):
    numeric_values = [float(value or 0.0) for value in values]
    min_value = min([0.0] + numeric_values)
    max_value = max([0.0] + numeric_values)
    span = max(max_value - min_value, float(minimum_span))
    if min_value < 0.0:
        min_value -= span * 0.16
    else:
        min_value = 0.0
    if max_value > 0.0:
        max_value += span * 0.16
    else:
        max_value = 0.0
    return [min_value, max_value]


def enum_label(value):
    return str(value).split(".")[-1]


def money(value):
    return f"${value:,.0f}"


def pct(value):
    return f"{100.0 * value:,.2f}%"


def dashboard_generated_at():
    return datetime.now().astimezone()


def parse_valuation_date(value):
    if isinstance(value, datetime):
        return value.date()
    if isinstance(value, date):
        return value
    if value:
        try:
            return date.fromisoformat(str(value)[:10])
        except ValueError:
            return None
    return None


def format_date(value):
    parsed = parse_valuation_date(value)
    return parsed.isoformat() if parsed else "n/a"


def format_timestamp(value):
    if isinstance(value, datetime):
        timestamp = value
    elif value:
        try:
            timestamp = datetime.fromisoformat(str(value))
        except ValueError:
            return str(value)
    else:
        timestamp = dashboard_generated_at()
    return timestamp.astimezone().strftime("%Y-%m-%d %H:%M:%S %Z")


def horizon_date(as_of_date, horizon_days):
    parsed = parse_valuation_date(as_of_date)
    if parsed is None:
        return None
    return parsed + timedelta(days=int(round(float(horizon_days or 0.0))))


def horizon_axis(as_of_date, horizon_days):
    start = parse_valuation_date(as_of_date)
    end = horizon_date(as_of_date, horizon_days)
    if start is None or end is None:
        return [0, horizon_days]
    return [start.isoformat(), end.isoformat()]


def short_factor_id(factor_id):
    return factor_id.replace("RF:", "")


RISK_FACTOR_DOMAIN_LABELS = {
    "COM": "Commodity",
    "COMVOL": "Commodity Volatility",
    "CREDIT": "Credit",
    "CREDITVOL": "Credit Volatility",
    "EQ": "Equity",
    "EQVOL": "Equity Volatility",
    "FX": "FX",
    "FXVOL": "FX Volatility",
    "RATES": "Rates",
    "RATESVOL": "Rates Volatility",
}


def factor_parts(factor_id):
    return str(factor_id or "").split(":")


def factor_domain(factor_id):
    parts = factor_parts(factor_id)
    if len(parts) > 1 and parts[0] == "RF":
        return parts[1]
    return parts[0] if parts else ""


def factor_group_label(factor_id):
    domain = factor_domain(factor_id).upper()
    return RISK_FACTOR_DOMAIN_LABELS.get(domain, title_token(domain or "Other"))


def factor_subgroup_label(factor_id):
    parts = factor_parts(factor_id)
    domain = factor_domain(factor_id).upper()
    if len(parts) < 3:
        return "Other"
    if domain in {"RATES", "RATESVOL"} and len(parts) >= 4:
        return f"{parts[2]} {parts[3]}"
    return parts[2]


def factor_sort_key(factor):
    factor_id = getattr(factor, "factor_id", "")
    return (
        sort_text(factor_group_label(factor_id)),
        sort_text(factor_subgroup_label(factor_id)),
        identifier_tenor_sort_key(factor_id),
        tuple(sort_text(token) for token in split_identifier(short_factor_id(factor_id))),
    )


def factor_hierarchy_axis(factors):
    return [
        [factor_group_label(factor.factor_id) for factor in factors],
        [factor_subgroup_label(factor.factor_id) for factor in factors],
        [axis_tick_label(short_factor_id(factor.factor_id), max_line_length=14, max_lines=2) for factor in factors],
    ]


FUNDED_NPV_PRODUCT_TYPES = {
    "credit_bond",
    "fixed_rate_bond",
    "floating_rate_note",
}


def trade_details(trade):
    details = getattr(trade, "details", {}) or {}
    if isinstance(details, dict):
        return details
    try:
        return dict(details)
    except (TypeError, ValueError):
        return {}


def first_numeric(*values):
    for value in values:
        if value is None:
            continue
        try:
            numeric = float(value)
        except (TypeError, ValueError):
            continue
        if math.isfinite(numeric) and abs(numeric) > 0.0:
            return numeric
    return None


def trade_exposure_proxy(trade, valuation_result):
    notional = getattr(trade, "notional", None)
    if notional is not None and abs(float(notional)) > 0.0:
        return abs(float(notional))

    details = trade_details(trade)
    quantity = getattr(trade, "quantity", None)
    if quantity is None:
        quantity = first_numeric(
            getattr(trade, "max_quantity", None),
            details.get("max_quantity"),
            details.get("quantity"),
        )
    reference_price = first_numeric(
        getattr(trade, "reference_price", None),
        getattr(trade, "contract_price", None),
        getattr(trade, "strike_price", None),
        details.get("reference_price"),
        details.get("contract_price"),
        details.get("strike_price"),
    )
    contract_multiplier = first_numeric(
        getattr(trade, "contract_size", None),
        getattr(trade, "multiplier", None),
        getattr(trade, "index_multiplier", None),
        details.get("contract_size"),
        details.get("multiplier"),
        details.get("index_multiplier"),
    )
    contract_multiplier = contract_multiplier if contract_multiplier is not None else 1.0
    if quantity is not None and reference_price is not None:
        return abs(float(quantity) * reference_price * contract_multiplier)

    return abs(float(valuation_result.npv))


def trade_display_npv(trade, valuation_result, product_type):
    npv = float(valuation_result.npv)
    if product_type not in FUNDED_NPV_PRODUCT_TYPES:
        return npv

    notional = first_numeric(getattr(trade, "notional", None))
    if notional is None:
        return npv
    par_value = abs(notional)
    return npv + par_value if npv < 0.0 else npv - par_value


def uses_balanced_demo_exposure(portfolio):
    label = f"{getattr(portfolio, 'portfolio_id', '')} {portfolio_label(portfolio)}".lower()
    return "demo" in label and "portfolio" in label


def balance_demo_exposures(portfolio, rows):
    if not uses_balanced_demo_exposure(portfolio):
        return rows

    asset_totals = defaultdict(float)
    for row in rows:
        asset_totals[row["asset_class"]] += max(0.0, float(row["raw_exposure"] or 0.0))
    positive_assets = {asset_class: total for asset_class, total in asset_totals.items() if total > 0.0}
    if len(positive_assets) < 2:
        return rows

    target_total = sum(positive_assets.values())
    target_asset_exposure = target_total / len(positive_assets)
    for row in rows:
        raw_exposure = max(0.0, float(row["raw_exposure"] or 0.0))
        asset_total = positive_assets.get(row["asset_class"], 0.0)
        row["exposure"] = raw_exposure * target_asset_exposure / asset_total if asset_total > 0.0 else raw_exposure
    return rows


def build_trade_rows(portfolio, valuation_results):
    valuation_by_trade = {result.trade_id: result for result in valuation_results}
    rows = []
    for trade in portfolio.trades:
        valuation = valuation_by_trade[trade.id]
        product_type = valuation.tags.get("product_type", trade.type)
        raw_exposure = trade_exposure_proxy(trade, valuation)
        rows.append(
            {
                "asset_class": valuation.tags.get("asset_class", trade.asset_class),
                "book": trade.book,
                "currency": valuation.currency,
                "display_npv": trade_display_npv(trade, valuation, product_type),
                "exposure": raw_exposure,
                "model": getattr(valuation, "model_name", "") or valuation.tags.get("model", ""),
                "npv": valuation.npv,
                "product_type": product_type,
                "raw_exposure": raw_exposure,
                "status": valuation.tags.get("status", "unknown"),
                "status_message": getattr(valuation, "status_message", ""),
                "strategy": trade.strategy,
                "trade_id": trade.id,
            }
        )
    return sort_trade_rows(balance_demo_exposures(portfolio, rows))


def aggregate_rows(rows, key, value):
    totals = defaultdict(float)
    for row in rows:
        totals[row[key]] += row[value]
    return dict(sorted(totals.items()))


def number_attr(obj, name, default=0.0):
    return float(getattr(obj, name, default) or 0.0)


def allocation_donut_segments(trade_rows, node_id="portfolio"):
    totals = defaultdict(float)
    segment_ids = {}
    if node_id.startswith("asset:"):
        asset_class = node_id.removeprefix("asset:")
        for row in trade_rows:
            product_type = row["product_type"]
            totals[product_type] += row["exposure"]
            segment_ids[product_type] = f"product:{asset_class}:{product_type}"
    elif node_id.startswith("product:"):
        for row in trade_rows:
            totals[row["trade_id"]] += row["exposure"]
            segment_ids[row["trade_id"]] = trade_node_id(row)
    elif node_id.startswith("trade:") and trade_rows:
        row = trade_rows[0]
        totals[row["trade_id"]] = row["exposure"]
        segment_ids[row["trade_id"]] = trade_node_id(row)
    else:
        for row in trade_rows:
            asset_class = row["asset_class"]
            totals[asset_class] += row["exposure"]
            segment_ids[asset_class] = f"asset:{asset_class}"

    labels = list(sorted(totals))
    return {
        "colors": [semantic_node_color(segment_ids[label], index) for index, label in enumerate(labels)],
        "ids": [segment_ids[label] for label in labels],
        "labels": labels,
        "values": [totals[label] for label in labels],
    }


def treemap_node_colors(node_ids):
    return [semantic_node_color(node_id, index) for index, node_id in enumerate(node_ids)]


ASSET_RETURN_MODELS = {
    "commodity": {"beta": 1.20, "drift": 0.065, "phase": 4.4, "vol": 0.24},
    "credit": {"beta": 0.72, "drift": 0.052, "phase": 2.2, "vol": 0.075},
    "equity": {"beta": 1.08, "drift": 0.085, "phase": 3.1, "vol": 0.18},
    "fx": {"beta": 0.58, "drift": 0.035, "phase": 5.3, "vol": 0.10},
    "rates": {"beta": -0.18, "drift": 0.038, "phase": 1.3, "vol": 0.045},
}
PERFORMANCE_DEFAULT_PERIOD = "2Y"
PERFORMANCE_HISTORY_DAYS = 3652
PERFORMANCE_PERIOD_DAYS = {
    "1Y": 365,
    "2Y": 730,
    "5Y": 1826,
    "10Y": PERFORMANCE_HISTORY_DAYS,
    "Inception": None,
}
DEFAULT_POLICY_BENCHMARK_WEIGHTS = {
    "commodity": 0.10,
    "credit": 0.25,
    "equity": 0.20,
    "fx": 0.05,
    "rates": 0.40,
}
POLICY_BENCHMARK_PROFILES = {
    "commodity": {
        "commodity": 0.55,
        "credit": 0.10,
        "equity": 0.10,
        "fx": 0.05,
        "rates": 0.20,
    },
    "defensive": {
        "commodity": 0.03,
        "credit": 0.30,
        "equity": 0.07,
        "fx": 0.00,
        "rates": 0.60,
    },
    "fx": {
        "commodity": 0.05,
        "credit": 0.10,
        "equity": 0.05,
        "fx": 0.50,
        "rates": 0.30,
    },
    "growth": {
        "commodity": 0.10,
        "credit": 0.20,
        "equity": 0.50,
        "fx": 0.05,
        "rates": 0.15,
    },
    "income": {
        "commodity": 0.05,
        "credit": 0.35,
        "equity": 0.05,
        "fx": 0.00,
        "rates": 0.55,
    },
    "liquidity": {
        "commodity": 0.00,
        "credit": 0.10,
        "equity": 0.00,
        "fx": 0.05,
        "rates": 0.85,
    },
}


def normalized_weights(weights):
    clean = {key: max(0.0, float(value or 0.0)) for key, value in weights.items() if float(value or 0.0) > 0.0}
    total = sum(clean.values())
    if total <= 0.0:
        return dict(DEFAULT_POLICY_BENCHMARK_WEIGHTS)
    return {key: value / total for key, value in sorted(clean.items())}


def portfolio_asset_weights(trade_rows):
    exposure_by_asset = defaultdict(float)
    for row in trade_rows:
        exposure_by_asset[row["asset_class"]] += max(0.0, float(row["exposure"] or 0.0))
    return normalized_weights(exposure_by_asset)


def benchmark_profile_key(portfolio):
    label = f"{getattr(portfolio, 'portfolio_id', '')} {portfolio_label(portfolio)}".lower()
    if "commodity" in label:
        return "commodity"
    if "cash" in label or "liquidity" in label:
        return "liquidity"
    if "defensive" in label or "conservative" in label or "cautious" in label:
        return "defensive"
    if "income" in label or "credit" in label:
        return "income"
    if "fx" in label:
        return "fx"
    if "growth" in label or "aggressive" in label or "adventurous" in label:
        return "growth"
    return "balanced"


def policy_benchmark_weights(portfolio):
    profile = benchmark_profile_key(portfolio)
    if profile == "balanced":
        return dict(DEFAULT_POLICY_BENCHMARK_WEIGHTS)
    return dict(POLICY_BENCHMARK_PROFILES[profile])


def benchmark_name(portfolio):
    profile = benchmark_profile_key(portfolio)
    if profile == "balanced":
        return "QRP Balanced Policy Benchmark"
    return f"QRP {title_token(profile)} Policy Benchmark"


def business_dates(end_date, lookback_days=PERFORMANCE_HISTORY_DAYS):
    end = parse_valuation_date(end_date) or date.today()
    start = end - timedelta(days=lookback_days)
    days = []
    current = start
    while current <= end:
        if current.weekday() < 5:
            days.append(current)
        current += timedelta(days=1)
    return days


def stable_wave(key, index, scale=1.0):
    seed = stable_color_index(key, 997) / 997.0
    return scale * (
        0.55 * math.sin(index / 17.0 + seed * math.tau)
        + 0.30 * math.sin(index / 43.0 + seed * math.pi)
        + 0.15 * math.cos(index / 7.0 + seed * math.tau)
    )


def market_regime_shock(progress):
    return (
        -0.0027 * math.exp(-(((progress - 0.28) / 0.055) ** 2))
        - 0.0020 * math.exp(-(((progress - 0.68) / 0.045) ** 2))
        + 0.0016 * math.exp(-(((progress - 0.47) / 0.075) ** 2))
        + 0.0012 * math.exp(-(((progress - 0.82) / 0.065) ** 2))
    )


def asset_daily_return(asset_class, index, sample_count):
    model = ASSET_RETURN_MODELS.get(asset_class, ASSET_RETURN_MODELS["rates"])
    progress = index / max(sample_count - 1, 1)
    common = market_regime_shock(progress)
    daily_drift = model["drift"] / 252.0
    daily_noise = model["vol"] / math.sqrt(252.0) * stable_wave(f"{asset_class}:{index}", index, scale=0.22)
    return daily_drift + model["beta"] * common + daily_noise


def weighted_daily_return(weights, index, sample_count):
    return sum(weight * asset_daily_return(asset_class, index, sample_count) for asset_class, weight in weights.items())


def portfolio_alpha(portfolio):
    key = f"{getattr(portfolio, 'portfolio_id', '')}:{portfolio_label(portfolio)}"
    return (stable_color_index(key, 2001) / 2000.0 - 0.5) * 0.018


def drawdown_series(values):
    peak = values[0] if values else 0.0
    drawdowns = []
    for value in values:
        peak = max(peak, value)
        denominator = abs(peak) if abs(peak) > 1e-12 else 1.0
        drawdowns.append((value - peak) / denominator)
    return drawdowns


def max_drawdown_detail(dates, values):
    if not values:
        return {"amount": 0.0, "date": "", "value": 0.0}
    drawdowns = drawdown_series(values)
    trough_index = min(range(len(drawdowns)), key=lambda index: drawdowns[index])
    return {
        "amount": drawdowns[trough_index],
        "date": dates[trough_index].isoformat(),
        "value": values[trough_index],
    }


def cumulative_return(values):
    if not values:
        return 0.0
    start = values[0]
    if abs(start) <= 1e-12:
        return 0.0
    return values[-1] / start - 1.0


def period_start_index(dates, lookback_days):
    if not dates or lookback_days is None:
        return 0
    threshold = dates[-1] - timedelta(days=lookback_days)
    for index, item in enumerate(dates):
        if item >= threshold:
            return index
    return 0


def performance_period_summary(dates, portfolio_values, benchmark_values, lookback_days):
    start_index = period_start_index(dates, lookback_days)
    period_dates = dates[start_index:]
    period_portfolio = portfolio_values[start_index:]
    period_benchmark = benchmark_values[start_index:]
    return {
        "active_return": cumulative_return(period_portfolio) - cumulative_return(period_benchmark),
        "benchmark_return": cumulative_return(period_benchmark),
        "max_benchmark_drawdown": max_drawdown_detail(period_dates, period_benchmark),
        "max_portfolio_drawdown": max_drawdown_detail(period_dates, period_portfolio),
        "portfolio_return": cumulative_return(period_portfolio),
        "start_date": period_dates[0].isoformat() if period_dates else "",
    }


def calendar_year_returns(dates, portfolio_values, benchmark_values):
    if not dates:
        return {"benchmark": [], "dates": [], "labels": [], "portfolio": []}

    labels = []
    label_dates = []
    portfolio_returns = []
    benchmark_returns = []
    start_index = 0
    current_year = dates[0].year

    for index, item in enumerate(dates[1:], start=1):
        if item.year == current_year:
            continue
        end_index = index - 1
        label = str(current_year)
        if start_index == 0 and (dates[start_index].month, dates[start_index].day) != (1, 1):
            label = f"{current_year} partial"
        labels.append(label)
        label_dates.append(dates[end_index].isoformat())
        portfolio_returns.append(cumulative_return(portfolio_values[start_index : end_index + 1]))
        benchmark_returns.append(cumulative_return(benchmark_values[start_index : end_index + 1]))
        start_index = index
        current_year = item.year

    label = f"{current_year} YTD"
    labels.append(label)
    label_dates.append(dates[-1].isoformat())
    portfolio_returns.append(cumulative_return(portfolio_values[start_index:]))
    benchmark_returns.append(cumulative_return(benchmark_values[start_index:]))
    return {
        "benchmark": benchmark_returns,
        "dates": label_dates,
        "labels": labels,
        "portfolio": portfolio_returns,
    }


def build_performance_history(portfolio, trade_rows, market_as_of):
    dates = business_dates(market_as_of)
    portfolio_weights = portfolio_asset_weights(trade_rows)
    benchmark_weights = policy_benchmark_weights(portfolio)
    portfolio_values = [100.0]
    benchmark_values = [100.0]
    alpha = portfolio_alpha(portfolio) / 252.0
    sample_count = len(dates)
    for index in range(1, sample_count):
        portfolio_return = (
            weighted_daily_return(portfolio_weights, index, sample_count)
            + alpha
            + stable_wave(f"{portfolio_label(portfolio)}:{index}", index, scale=0.00045)
        )
        benchmark_return = weighted_daily_return(benchmark_weights, index, sample_count)
        portfolio_values.append(portfolio_values[-1] * (1.0 + portfolio_return))
        benchmark_values.append(benchmark_values[-1] * (1.0 + benchmark_return))

    portfolio_drawdowns = drawdown_series(portfolio_values)
    benchmark_drawdowns = drawdown_series(benchmark_values)
    active_returns = [
        portfolio_value / benchmark_value - 1.0
        for portfolio_value, benchmark_value in zip(portfolio_values, benchmark_values)
    ]
    period_summaries = {
        label: performance_period_summary(dates, portfolio_values, benchmark_values, lookback_days)
        for label, lookback_days in PERFORMANCE_PERIOD_DAYS.items()
    }
    default_summary = period_summaries[PERFORMANCE_DEFAULT_PERIOD]
    return {
        "active_return": default_summary["active_return"],
        "active_returns": active_returns,
        "benchmark_drawdowns": benchmark_drawdowns,
        "benchmark_name": benchmark_name(portfolio),
        "benchmark_return": default_summary["benchmark_return"],
        "benchmark_values": benchmark_values,
        "calendar_year_returns": calendar_year_returns(dates, portfolio_values, benchmark_values),
        "dates": [item.isoformat() for item in dates],
        "default_period": PERFORMANCE_DEFAULT_PERIOD,
        "default_start_date": default_summary["start_date"],
        "max_benchmark_drawdown": default_summary["max_benchmark_drawdown"],
        "max_portfolio_drawdown": default_summary["max_portfolio_drawdown"],
        "periods": period_summaries,
        "portfolio_drawdowns": portfolio_drawdowns,
        "portfolio_return": default_summary["portfolio_return"],
        "portfolio_values": portfolio_values,
        "portfolio_weights": portfolio_weights,
    }


def apply_finance_layout(fig, height, showlegend=False, barmode=None, title=None, yaxis_title=None):
    layout = {
        "colorway": DASHBOARD_COLORWAY,
        "font": {"color": "#16231d", "family": FINANCE_FONT, "size": 12},
        "height": height,
        "legend": {
            "bgcolor": "rgba(0,0,0,0)",
            "font": {"family": FINANCE_FONT, "size": 12},
            "orientation": "h",
            "y": -0.12,
        },
        "margin": {"b": 72, "l": 64, "r": 32, "t": 76},
        "paper_bgcolor": "rgba(0,0,0,0)",
        "plot_bgcolor": "rgba(0,0,0,0)",
        "showlegend": showlegend,
        "template": "plotly_white",
        "title_font": {"family": FINANCE_FONT, "size": 18},
    }
    if barmode:
        layout["barmode"] = barmode
    if title:
        layout["title"] = {"text": bold_chart_text(title)}
    if yaxis_title:
        layout["yaxis_title"] = yaxis_title

    fig.update_layout(**layout)
    for annotation in fig.layout.annotations or []:
        annotation.update(
            font={"color": "#24372f", "family": FINANCE_FONT, "size": 13},
            text=bold_chart_text(annotation.text),
        )
    fig.update_xaxes(
        gridcolor="rgba(35, 55, 47, 0.13)",
        linecolor="rgba(35, 55, 47, 0.25)",
        tickfont={"family": FINANCE_FONT, "size": 11},
        title_font={"family": FINANCE_FONT, "size": 12},
        zerolinecolor="rgba(35, 55, 47, 0.24)",
    )
    fig.update_yaxes(
        gridcolor="rgba(35, 55, 47, 0.13)",
        linecolor="rgba(35, 55, 47, 0.25)",
        tickfont={"family": FINANCE_FONT, "size": 11},
        title_font={"family": FINANCE_FONT, "size": 12},
        zerolinecolor="rgba(35, 55, 47, 0.24)",
    )
    return fig


def render_dashboard_html(title, views, output_path, pio, report_context=None):
    output_path.parent.mkdir(parents=True, exist_ok=True)
    context = report_context or {}
    market_as_of = context.get("market_as_of", "n/a")
    generated_at = context.get("generated_at", "n/a")
    allocation_controls_template = read_dashboard_asset("allocation_controls.html")
    card_template = read_dashboard_asset("card.html")
    lazy_panel_body_template = read_dashboard_asset("lazy_panel_body.html")
    option_template = read_dashboard_asset("select_option.html")
    panel_note_template = read_dashboard_asset("panel_note.html")
    panel_template = read_dashboard_asset("panel.html")
    portfolio_view_template = read_dashboard_asset("portfolio_view.html")
    tab_button_template = read_dashboard_asset("dashboard_tab_button.html")
    tabs_template = read_dashboard_asset("dashboard_tabs.html")
    style_options = dashboard_style_options(option_template)
    portfolio_options = "\n".join(render_select_option(option_template, view["id"], view["label"]) for view in views)
    plotly_index = 0
    view_html = []
    for view_index, view in enumerate(views):
        card_html = "\n".join(
            render_asset_template(
                card_template,
                {
                    "__QRP_CARD_LABEL__": html.escape(card["label"]),
                    "__QRP_CARD_NOTE__": html.escape(card["note"]),
                    "__QRP_CARD_VALUE__": html.escape(card["value"]),
                },
            )
            for card in view["cards"]
        )
        figure_html = []
        tab_html = []
        view_slug = html_id(view["id"])
        for figure_index, figure in enumerate(view["figures"]):
            if len(figure) == 3:
                name, fig, note = figure
            else:
                name, fig = figure
                note = ""
            if isinstance(fig, dict) and fig.get("kind") == "html":
                panel_body = fig["html"]
            else:
                include_plotly = "cdn" if plotly_index == 0 else False
                plotly_index += 1
                panel_body = pio.to_html(fig, full_html=False, include_plotlyjs=include_plotly)
            panel_id = f"{view_slug}-panel-{figure_index + 1}"
            tab_id = f"{view_slug}-tab-{figure_index + 1}"
            is_active_panel = figure_index == 0
            should_render_immediately = view_index == 0 and figure_index == 0
            allocation_controls = allocation_controls_template if name == "Portfolio Allocation" else ""
            panel_note_html = (
                render_asset_template(panel_note_template, {"__QRP_PANEL_NOTE__": html.escape(note)}) if note else ""
            )
            rendered_panel_body = (
                panel_body
                if should_render_immediately
                else render_asset_template(lazy_panel_body_template, {"__QRP_LAZY_PANEL_BODY__": panel_body})
            )
            tab_html.append(
                render_asset_template(
                    tab_button_template,
                    {
                        "__QRP_PANEL_ID__": html.escape(panel_id),
                        "__QRP_TAB_ID__": html.escape(tab_id),
                        "__QRP_TAB_LABEL__": html.escape(name),
                        "__QRP_TAB_SELECTED__": "true" if is_active_panel else "false",
                    },
                )
            )
            figure_html.append(
                render_asset_template(
                    panel_template,
                    {
                        "__QRP_ALLOCATION_CONTROLS__": allocation_controls,
                        "__QRP_PANEL_BODY__": rendered_panel_body,
                        "__QRP_PANEL_HIDDEN__": "" if is_active_panel else " hidden",
                        "__QRP_PANEL_ID__": html.escape(panel_id),
                        "__QRP_PANEL_NOTE_HTML__": panel_note_html,
                        "__QRP_PANEL_TAB_ID__": html.escape(tab_id),
                        "__QRP_PANEL_TITLE__": html.escape(name),
                    },
                )
            )
        hidden = "" if view_index == 0 else " hidden"
        tabs_html = render_asset_template(tabs_template, {"__QRP_TAB_BUTTONS__": "".join(tab_html)})
        view_html.append(
            render_asset_template(
                portfolio_view_template,
                {
                    "__QRP_VIEW_CARDS__": card_html,
                    "__QRP_VIEW_DESCRIPTION__": html.escape(view["description"]),
                    "__QRP_VIEW_FIGURES__": "".join(figure_html),
                    "__QRP_VIEW_HIDDEN__": hidden,
                    "__QRP_VIEW_ID__": html.escape(view["id"]),
                    "__QRP_VIEW_LABEL__": html.escape(view["label"]),
                    "__QRP_VIEW_SUMMARY__": html.escape(view["summary"]),
                    "__QRP_VIEW_TABS__": tabs_html,
                },
            )
        )
    view_html = "\n".join(view_html)
    template = read_dashboard_asset("dashboard_template.html")
    document = render_asset_template(
        template,
        {
            "__QRP_DASHBOARD_CSS__": read_dashboard_asset("dashboard.css"),
            "__QRP_DASHBOARD_JS__": read_dashboard_asset("dashboard.js"),
            "__QRP_GENERATED_AT__": html.escape(generated_at),
            "__QRP_INITIAL_THEME_JS__": read_dashboard_asset("initial_theme.js"),
            "__QRP_MARKET_AS_OF__": html.escape(market_as_of),
            "__QRP_PORTFOLIO_OPTIONS__": portfolio_options,
            "__QRP_STYLE_OPTIONS__": style_options,
            "__QRP_STYLE_PRESETS_JSON__": json_for_script(DASHBOARD_STYLE_PRESETS),
            "__QRP_TITLE__": html.escape(title),
            "__QRP_VIEW_HTML__": view_html,
        },
    )
    output_path.write_text(document, encoding="utf-8")
    return output_path


def make_portfolio_allocation_figure(go, make_subplots, trade_rows):
    fill_text_color = active_style_preset()["fill_text"]
    trade_rows = sort_trade_rows(trade_rows)
    exposure_by_asset = aggregate_rows(trade_rows, "asset_class", "exposure")
    npv_axis = [
        [row["asset_class"] for row in trade_rows],
        [row["product_type"] for row in trade_rows],
        [row["trade_id"] for row in trade_rows],
    ]
    npv_axis_labels = [f"{row['asset_class']} / {row['product_type']} / {row['trade_id']}" for row in trade_rows]
    npv_values = [row.get("display_npv", row["npv"]) for row in trade_rows]
    donut = allocation_donut_segments(trade_rows)
    chart_height = bounded_chart_height(
        len(trade_rows),
        base_height=560,
        row_height=28,
        min_height=860,
        max_height=1280,
    )
    row_heights = [0.38, 0.62] if len(trade_rows) > 22 else [0.46, 0.54]

    treemap_ids = ["portfolio"]
    treemap_labels = ["All portfolio"]
    treemap_parents = [""]
    treemap_values = [sum(row["exposure"] for row in trade_rows)]

    for asset_class, exposure in exposure_by_asset.items():
        asset_id = f"asset:{asset_class}"
        treemap_ids.append(asset_id)
        treemap_labels.append(asset_class)
        treemap_parents.append("portfolio")
        treemap_values.append(exposure)

    exposure_by_product = defaultdict(float)
    for row in trade_rows:
        product_key = (row["asset_class"], row["product_type"])
        exposure_by_product[product_key] += row["exposure"]

    for (asset_class, product_type), exposure in sorted(exposure_by_product.items()):
        product_id = f"product:{asset_class}:{product_type}"
        treemap_ids.append(product_id)
        treemap_labels.append(product_type)
        treemap_parents.append(f"asset:{asset_class}")
        treemap_values.append(exposure)

    for row in trade_rows:
        treemap_ids.append(trade_node_id(row))
        treemap_labels.append(row["trade_id"])
        treemap_parents.append(f"product:{row['asset_class']}:{row['product_type']}")
        treemap_values.append(row["exposure"])

    fig = make_subplots(
        rows=2,
        cols=2,
        column_widths=[0.52, 0.48],
        horizontal_spacing=0.08,
        row_heights=row_heights,
        specs=[
            [{"type": "domain"}, {"type": "domain"}],
            [{"type": "xy", "colspan": 2}, None],
        ],
        subplot_titles=[
            "Exposure Map",
            "Allocation Donut",
            "NPV by Position",
        ],
        vertical_spacing=0.11,
    )
    fig.add_trace(
        go.Treemap(
            branchvalues="total",
            hovertemplate="<b>%{label}</b><br>Exposure: %{value:,.0f}<extra></extra>",
            ids=treemap_ids,
            labels=treemap_labels,
            marker={
                "colors": treemap_node_colors(treemap_ids),
                "line": {"color": "rgba(255,255,255,0.26)", "width": 1.5},
            },
            parents=treemap_parents,
            pathbar={
                "edgeshape": ">",
                "side": "top",
                "thickness": 24,
                "visible": True,
            },
            textfont={"color": fill_text_color, "family": FINANCE_FONT, "size": 15},
            texttemplate="<b>%{label}</b><br>%{value:,.0f}",
            values=treemap_values,
        ),
        row=1,
        col=1,
    )
    fig.add_trace(
        go.Pie(
            automargin=True,
            customdata=donut["ids"],
            hole=0.62,
            hovertemplate="<b>%{label}</b><br>Exposure: %{value:,.0f}<br>Share: %{percent}<extra></extra>",
            ids=donut["ids"],
            labels=donut["labels"],
            marker={
                "colors": donut["colors"],
                "line": {"color": "rgba(255,255,255,0.38)", "width": 2},
            },
            name="Allocation",
            sort=False,
            textfont={"color": fill_text_color, "family": FINANCE_FONT, "size": 12},
            textinfo="label+percent",
            textposition="outside",
            values=donut["values"],
        ),
        row=1,
        col=2,
    )
    fig.add_trace(
        go.Bar(
            cliponaxis=False,
            constraintext="none",
            customdata=[
                [
                    row["asset_class"],
                    row["product_type"],
                    row["trade_id"],
                    row["npv"],
                ]
                for row in trade_rows
            ],
            hovertemplate=(
                "<b>%{customdata[2]}</b><br>"
                "Asset: %{customdata[0]}<br>"
                "Product: %{customdata[1]}<br>"
                "Display NPV: %{x:,.2f}<br>"
                "Raw NPV: %{customdata[3]:,.2f}<extra></extra>"
            ),
            orientation="h",
            x=npv_values,
            y=npv_axis,
            marker_color=[trade_metric_color(row) for row in trade_rows],
            text=[money(value) for value in npv_values],
            textposition="auto",
        ),
        row=2,
        col=1,
    )
    apply_finance_layout(fig, height=chart_height)
    fig.update_layout(
        margin={
            "b": 76,
            "l": categorical_left_margin(npv_axis_labels, min_margin=180, max_margin=300, char_width=4),
            "r": 44,
            "t": 130,
        }
    )
    for annotation in fig.layout.annotations:
        if annotation.text in {
            bold_chart_text("Exposure Map"),
            bold_chart_text("Allocation Donut"),
        }:
            annotation.update(yshift=38)
    fig.update_layout(
        meta={
            "qrpPanel": "portfolio_allocation",
            "tradeRows": trade_rows,
        }
    )
    fig.update_xaxes(range=padded_number_range(npv_values), title_text="Display NPV", row=2, col=1)
    fig.update_yaxes(autorange="reversed", automargin=True, row=2, col=1, type="multicategory")
    return fig


def make_trade_inventory_figure(go, trade_rows):
    columns = [
        ("trade_id", "Position", "text"),
        ("asset_class", "Asset", "text"),
        ("product_type", "Product", "text"),
        ("book", "Book", "text"),
        ("strategy", "Strategy", "text"),
        ("exposure", "Exposure", "number"),
        ("npv", "NPV", "number"),
        ("status", "Status", "text"),
    ]
    return {
        "html": sortable_table_html(
            "trade-inventory",
            sort_trade_rows(trade_rows),
            columns,
            max_height=bounded_chart_height(
                len(trade_rows),
                base_height=160,
                row_height=34,
                min_height=360,
                max_height=760,
            ),
        ),
        "kind": "html",
    }


def sortable_value(value, kind):
    if kind == "number":
        return f"{float(value or 0.0):.12f}"
    return sort_text(value)


def cell_display(row, key, kind, status_template):
    value = row.get(key, "")
    if key == "status":
        return render_asset_template(
            status_template,
            {
                "__QRP_STATUS_CLASS__": html.escape(str(value).replace("_", "-")),
                "__QRP_STATUS_LABEL__": html.escape(support_status_label(value)),
            },
        )
    if kind == "number":
        return html.escape(money(float(value or 0.0)))
    return html.escape(str(value or ""))


def row_cell_html(row, key, kind, cell_template, status_template):
    return render_asset_template(
        cell_template,
        {
            "__QRP_CELL_DISPLAY__": cell_display(row, key, kind, status_template),
            "__QRP_CELL_KIND__": html.escape(kind),
            "__QRP_CELL_SORT_VALUE__": html.escape(sortable_value(row.get(key), kind)),
        },
    )


def sortable_table_html(table_id, rows, columns, *, max_height):
    cell_template = read_dashboard_asset("table_cell.html")
    empty_row_template = read_dashboard_asset("table_empty_row.html")
    header_cell_template = read_dashboard_asset("table_header_cell.html")
    row_template = read_dashboard_asset("table_row.html")
    status_template = read_dashboard_asset("status_pill.html")
    table_template = read_dashboard_asset("data_table.html")
    header_html = "".join(
        render_asset_template(
            header_cell_template,
            {
                "__QRP_COLUMN_INDEX__": str(index),
                "__QRP_COLUMN_KIND__": html.escape(kind),
                "__QRP_COLUMN_LABEL__": html.escape(label),
            },
        )
        for index, (_, label, kind) in enumerate(columns)
    )
    body_html = "\n".join(
        render_asset_template(
            row_template,
            {
                "__QRP_ROW_CELLS__": "".join(
                    row_cell_html(row, key, kind, cell_template, status_template) for key, _, kind in columns
                ),
                "__QRP_ROW_DEFAULT_SORT__": html.escape("|".join(str(value) for value in trade_sort_key(row))),
            },
        )
        for row in rows
    )
    if not body_html:
        body_html = render_asset_template(empty_row_template, {"__QRP_COLUMN_COUNT__": str(len(columns))})

    return render_asset_template(
        table_template,
        {
            "__QRP_TABLE_BODY__": body_html,
            "__QRP_TABLE_HEADER__": header_html,
            "__QRP_TABLE_ID__": html.escape(table_id),
            "__QRP_TABLE_MAX_HEIGHT__": str(int(max_height)),
        },
    )


def make_support_diagnostics_panel(trade_rows):
    rows = support_diagnostic_rows(sort_trade_rows(trade_rows))
    columns = [
        ("trade_id", "Position", "text"),
        ("asset_class", "Asset", "text"),
        ("product_type", "Product", "text"),
        ("model", "Model", "text"),
        ("status", "Status", "text"),
        ("status_message", "Reason", "text"),
    ]
    if not rows:
        rows = [
            {
                "asset_class": "",
                "model": "",
                "product_type": "",
                "status": "supported",
                "status_message": "All positions use fully supported pricing profiles.",
                "trade_id": "all_trades",
            }
        ]
    return {
        "html": sortable_table_html(
            "support-diagnostics",
            rows,
            columns,
            max_height=bounded_chart_height(
                len(rows),
                base_height=150,
                row_height=38,
                min_height=260,
                max_height=520,
            ),
        ),
        "kind": "html",
    }


def make_risk_measures_figure(go, make_subplots, risk_results, trade_rows):
    sorted_results = sorted(risk_results, key=lambda result: trade_id_sort_key(result.trade_id, trade_rows))
    trade_ids = [result.trade_id for result in sorted_results]
    trade_axis = trade_hierarchy_axis(trade_ids, trade_rows)
    trade_labels = trade_hierarchy_labels(trade_ids, trade_rows)
    all_buckets = sorted(
        {bucket for result in sorted_results for bucket in result.bucketed_risk},
        key=risk_bucket_sort_key,
    )
    bucket_axis = risk_bucket_hierarchy_axis(all_buckets)
    z = [[result.bucketed_risk.get(bucket, 0.0) for bucket in all_buckets] for result in sorted_results]
    bucket_customdata = [[[result.trade_id, bucket] for bucket in all_buckets] for result in sorted_results]
    height = bounded_chart_height(
        len(sorted_results),
        base_height=280,
        row_height=26,
        min_height=520,
        max_height=980,
    )

    fig = make_subplots(
        rows=1,
        cols=2,
        shared_yaxes=True,
        specs=[[{"type": "xy"}, {"type": "xy"}]],
        subplot_titles=[
            "Aggregate Sensitivities by Position",
            "Bucketed Rate/Credit Risk",
        ],
    )
    fig.add_trace(
        go.Bar(
            name="PV01",
            orientation="h",
            x=[result.pv01 for result in sorted_results],
            y=trade_axis,
        ),
        row=1,
        col=1,
    )
    fig.add_trace(
        go.Bar(
            name="CS01",
            orientation="h",
            x=[result.cs01 for result in sorted_results],
            y=trade_axis,
        ),
        row=1,
        col=1,
    )
    if all_buckets:
        fig.add_trace(
            go.Heatmap(
                colorbar={"title": "Risk"},
                colorscale=DIVERGING_SCALE,
                customdata=bucket_customdata,
                hovertemplate="<b>%{customdata[0]}</b><br>Bucket: %{customdata[1]}<br>Risk: %{z:,.2f}<extra></extra>",
                x=bucket_axis,
                y=trade_axis,
                zmid=0.0,
                z=z,
            ),
            row=1,
            col=2,
        )
    apply_finance_layout(fig, barmode="group", height=height, showlegend=True)
    fig.update_layout(
        margin={
            "b": 86,
            "l": categorical_left_margin(trade_labels, min_margin=180, max_margin=320, char_width=4),
            "r": 38,
            "t": 82,
        }
    )
    fig.update_xaxes(title_text="Sensitivity", row=1, col=1)
    fig.update_xaxes(
        **diagonal_category_axis(title_text="Risk Bucket", multicategory=True),
        row=1,
        col=2,
    )
    fig.update_yaxes(autorange="reversed", automargin=True, row=1, col=1, type="multicategory")
    fig.update_yaxes(
        autorange="reversed",
        automargin=True,
        row=1,
        col=2,
        showticklabels=False,
        type="multicategory",
    )
    return fig


def make_historical_stress_figure(go, make_subplots, stress_results, trade_rows):
    stress_sorted = sorted(stress_results, key=lambda result: result.total_pnl)
    scenario_names = [result.scenario_name for result in stress_sorted]
    scenario_pnls = [result.total_pnl for result in stress_sorted]
    matrix_results = sorted(stress_results, key=lambda result: scenario_sort_key(result.scenario_name))
    matrix_scenario_names = [result.scenario_name for result in matrix_results]
    matrix_scenario_axis = scenario_hierarchy_axis(matrix_scenario_names)
    trade_ids = sorted(
        {trade_id for result in stress_results for trade_id in result.trade_pnls},
        key=lambda trade_id: trade_id_sort_key(trade_id, trade_rows),
    )
    trade_axis = trade_hierarchy_axis(trade_ids, trade_rows)
    trade_labels = trade_hierarchy_labels(trade_ids, trade_rows)
    stress_matrix = [[result.trade_pnls.get(trade_id, 0.0) for result in matrix_results] for trade_id in trade_ids]
    stress_customdata = [[[trade_id, result.scenario_name] for result in matrix_results] for trade_id in trade_ids]
    worst = stress_sorted[0]
    worst_trade_pnls = sorted(
        [(trade_id, worst.trade_pnls.get(trade_id, 0.0)) for trade_id in trade_ids],
        key=lambda item: abs(item[1]),
        reverse=True,
    )
    height = bounded_chart_height(
        max(len(scenario_names), len(trade_ids)),
        base_height=760,
        row_height=26,
        min_height=1120,
        max_height=1560,
    )

    fig = make_subplots(
        rows=3,
        cols=1,
        row_heights=[0.26, 0.38, 0.36],
        specs=[[{"type": "xy"}], [{"type": "xy"}], [{"type": "xy"}]],
        subplot_titles=[
            "Stress Scenario Ranking",
            "Position P&L Matrix",
            f"Worst Scenario Position Contribution: {worst.scenario_name}",
        ],
        vertical_spacing=0.13,
    )
    fig.add_trace(
        go.Bar(
            cliponaxis=False,
            constraintext="none",
            customdata=[[result.scenario_name] for result in stress_sorted],
            hovertemplate="<b>%{customdata[0]}</b><br>Total P&L: %{x:,.0f}<extra></extra>",
            marker_color=[NEGATIVE_COLOR if pnl < 0 else POSITIVE_COLOR for pnl in scenario_pnls],
            orientation="h",
            text=[money(pnl) for pnl in scenario_pnls],
            textposition="auto",
            x=scenario_pnls,
            y=scenario_names,
        ),
        row=1,
        col=1,
    )
    fig.add_trace(
        go.Heatmap(
            colorbar={"title": "Position P&L", "thickness": 18},
            colorscale=DIVERGING_SCALE,
            customdata=stress_customdata,
            hovertemplate="<b>%{customdata[0]}</b><br>%{customdata[1]}<br>P&L: %{z:,.0f}<extra></extra>",
            x=matrix_scenario_axis,
            y=trade_axis,
            zmid=0.0,
            z=stress_matrix,
        ),
        row=2,
        col=1,
    )
    fig.add_trace(
        go.Bar(
            cliponaxis=False,
            constraintext="none",
            hovertemplate="%{y}<br>Contribution: %{x:,.0f}<extra></extra>",
            marker_color=[NEGATIVE_COLOR if pnl < 0 else POSITIVE_COLOR for _, pnl in worst_trade_pnls],
            orientation="h",
            text=[money(pnl) for _, pnl in worst_trade_pnls],
            textposition="auto",
            x=[pnl for _, pnl in worst_trade_pnls],
            y=[trade_id for trade_id, _ in worst_trade_pnls],
        ),
        row=3,
        col=1,
    )
    apply_finance_layout(fig, height=height)
    fig.update_layout(
        margin={
            "b": 118,
            "l": max(
                categorical_left_margin(scenario_names),
                categorical_left_margin(trade_labels, min_margin=180, max_margin=320, char_width=4),
                categorical_left_margin([trade_id for trade_id, _ in worst_trade_pnls]),
            ),
            "r": 94,
            "t": 108,
        }
    )
    fig.add_vline(
        x=worst.total_pnl,
        line_color=WARNING_COLOR,
        line_dash="dot",
        line_width=2,
        row=3,
        col=1,
    )
    fig.add_annotation(
        showarrow=False,
        text=f"Total: {money(worst.total_pnl)}",
        x=worst.total_pnl,
        xref="x3",
        xanchor="right" if worst.total_pnl < 0 else "left",
        xshift=-8 if worst.total_pnl < 0 else 8,
        y=1.06,
        yref="y3 domain",
    )
    fig.update_xaxes(
        range=padded_number_range(scenario_pnls),
        title_text="Total P&L",
        row=1,
        col=1,
    )
    fig.update_xaxes(
        **diagonal_category_axis(title_text="Scenario", multicategory=True),
        row=2,
        col=1,
    )
    fig.update_xaxes(
        range=padded_number_range([pnl for _, pnl in worst_trade_pnls] + [worst.total_pnl]),
        title_text="Position Contribution",
        row=3,
        col=1,
    )
    fig.update_yaxes(autorange="reversed", automargin=True, row=1, col=1)
    fig.update_yaxes(autorange="reversed", automargin=True, row=2, col=1, type="multicategory")
    fig.update_yaxes(autorange="reversed", automargin=True, row=3, col=1)
    return fig


def make_performance_review_figure(go, make_subplots, performance_history):
    benchmark = performance_history["benchmark_name"]
    dates = performance_history["dates"]
    default_period = performance_history.get("default_period", PERFORMANCE_DEFAULT_PERIOD)
    default_start = performance_history.get("default_start_date") or dates[0]
    max_drawdown = performance_history["max_portfolio_drawdown"]
    calendar = performance_history["calendar_year_returns"]
    year_labels = calendar["labels"]
    year_dates = calendar["dates"]
    fig = make_subplots(
        rows=4,
        cols=1,
        shared_xaxes=True,
        row_heights=[0.38, 0.20, 0.26, 0.16],
        subplot_titles=[
            "Portfolio vs Benchmark Total Return",
            "Active Return vs Benchmark",
            f"Drawdown Over Time ({default_period} Portfolio Max {pct(max_drawdown['amount'])})",
            "Calendar-Year / YoY Return",
        ],
        vertical_spacing=0.08,
    )
    fig.add_trace(
        go.Scatter(
            mode="lines+markers",
            name="Portfolio",
            x=dates,
            y=performance_history["portfolio_values"],
            line={"color": POSITIVE_COLOR, "width": 3},
        ),
        row=1,
        col=1,
    )
    fig.add_trace(
        go.Scatter(
            mode="lines",
            name=benchmark,
            x=dates,
            y=performance_history["benchmark_values"],
            line={"color": DASHBOARD_COLORWAY[1], "dash": "dash", "width": 2.5},
        ),
        row=1,
        col=1,
    )
    fig.add_trace(
        go.Bar(
            name="Active Return",
            x=dates,
            y=[value * 100.0 for value in performance_history["active_returns"]],
            marker_color=[
                POSITIVE_COLOR if value >= 0.0 else NEGATIVE_COLOR for value in performance_history["active_returns"]
            ],
        ),
        row=2,
        col=1,
    )
    fig.add_trace(
        go.Bar(
            name="Portfolio Drawdown",
            x=dates,
            y=[value * 100.0 for value in performance_history["portfolio_drawdowns"]],
            marker_color=NEGATIVE_COLOR,
        ),
        row=3,
        col=1,
    )
    fig.add_trace(
        go.Scatter(
            mode="lines",
            name=f"{benchmark} Drawdown",
            x=dates,
            y=[value * 100.0 for value in performance_history["benchmark_drawdowns"]],
            line={"color": DASHBOARD_COLORWAY[1], "dash": "dot", "width": 2},
        ),
        row=3,
        col=1,
    )
    fig.add_trace(
        go.Bar(
            customdata=year_labels,
            marker_color=POSITIVE_COLOR,
            name="Portfolio YoY",
            hovertemplate="<b>%{customdata}</b><br>Portfolio: %{y:.2f}%<extra></extra>",
            x=year_dates,
            y=[value * 100.0 for value in calendar["portfolio"]],
        ),
        row=4,
        col=1,
    )
    fig.add_trace(
        go.Bar(
            customdata=year_labels,
            marker_color=DASHBOARD_COLORWAY[1],
            name=f"{benchmark} YoY",
            hovertemplate="<b>%{customdata}</b><br>Benchmark: %{y:.2f}%<extra></extra>",
            x=year_dates,
            y=[value * 100.0 for value in calendar["benchmark"]],
        ),
        row=4,
        col=1,
    )
    apply_finance_layout(fig, height=1080, showlegend=True, barmode="group")
    fig.update_layout(
        margin={"b": 80, "l": 76, "r": 40, "t": 116},
        hovermode="x unified",
        legend={"orientation": "h", "y": -0.09},
    )
    fig.update_xaxes(
        range=[default_start, dates[-1]],
        type="date",
    )
    fig.update_xaxes(
        rangeselector={
            "buttons": [
                {"count": 1, "label": "1Y", "step": "year", "stepmode": "backward"},
                {"count": 2, "label": "2Y", "step": "year", "stepmode": "backward"},
                {"count": 5, "label": "5Y", "step": "year", "stepmode": "backward"},
                {"count": 10, "label": "10Y", "step": "year", "stepmode": "backward"},
                {"label": "Inception", "step": "all"},
            ],
            "bgcolor": "rgba(255,255,255,0.66)",
            "font": {"color": "#16231d", "family": FINANCE_FONT, "size": 10},
        },
        row=1,
        col=1,
    )
    fig.update_xaxes(
        title_text="Date",
        row=4,
        col=1,
    )
    fig.update_yaxes(title_text="Total Return Index", row=1, col=1)
    fig.update_yaxes(title_text="Active Return %", row=2, col=1)
    fig.update_yaxes(title_text="Drawdown %", row=3, col=1)
    fig.update_yaxes(title_text="YoY Return %", row=4, col=1)
    return fig


def make_monte_carlo_figure(go, make_subplots, mc_results, market_as_of=None):
    titles = []
    for label, result, _ in mc_results:
        end_date = horizon_date(market_as_of, result.horizon_days)
        horizon_label = format_date(end_date) if end_date else f"{result.horizon_days:g}d"
        titles.append(f"{label} P&L Distribution to {horizon_label}")
    for label, result, _ in mc_results:
        end_date = horizon_date(market_as_of, result.horizon_days)
        horizon_label = format_date(end_date) if end_date else f"{result.horizon_days:g}d"
        titles.append(f"{label} Future Path Fan to {horizon_label}")

    fig = make_subplots(
        rows=2,
        cols=len(mc_results),
        subplot_titles=titles,
    )
    for col, (label, result, mean_pnl) in enumerate(mc_results, start=1):
        pnls = list(result.portfolio_pnls)
        base = result.base_portfolio_value
        horizon_days = result.horizon_days
        x_axis = horizon_axis(market_as_of, horizon_days)
        uses_dates = parse_valuation_date(market_as_of) is not None
        var_threshold = -result.var_95
        es_threshold = -result.expected_shortfall_95

        fig.add_trace(
            go.Histogram(name=f"{label} P&L", x=pnls, nbinsx=35, marker_color=POSITIVE_COLOR),
            row=1,
            col=col,
        )
        fig.add_vline(x=var_threshold, line_dash="dash", line_color=NEGATIVE_COLOR, row=1, col=col)
        fig.add_vline(x=es_threshold, line_dash="dot", line_color="#7c2d12", row=1, col=col)
        fig.add_vline(x=mean_pnl, line_dash="solid", line_color=WARNING_COLOR, row=1, col=col)

        selected_pnls = pnls[: min(40, len(pnls))]
        for path_index, pnl in enumerate(selected_pnls):
            path_hover = (
                f"path={path_index}<br>date=%{{x|%Y-%m-%d}}<br>value=%{{y:,.2f}}<extra></extra>"
                if uses_dates
                else f"path={path_index}<br>day=%{{x}}<br>value=%{{y:,.2f}}<extra></extra>"
            )
            fig.add_trace(
                go.Scatter(
                    hovertemplate=path_hover,
                    line={"color": "rgba(15, 118, 110, 0.18)", "width": 1},
                    mode="lines",
                    name=f"{label} path",
                    showlegend=False,
                    x=x_axis,
                    y=[base, base + pnl],
                ),
                row=2,
                col=col,
            )
        fig.add_trace(
            go.Scatter(
                line={"color": NEGATIVE_COLOR, "width": 3},
                mode="lines+markers",
                name=f"{label} mean",
                x=x_axis,
                y=[base, base + mean_pnl],
            ),
            row=2,
            col=col,
        )
        end_date = horizon_date(market_as_of, horizon_days)
        horizon_label = format_date(end_date) if end_date else f"{horizon_days:g}d"
        fig.update_xaxes(title_text=f"Horizon P&L ending {horizon_label}", row=1, col=col)
        if uses_dates:
            fig.update_xaxes(title_text="Date", type="date", row=2, col=col)
        else:
            fig.update_xaxes(title_text="Simulation day", row=2, col=col)
    apply_finance_layout(fig, height=780)
    return fig


def explain_component_amount(result, component_id, fallback):
    for component in getattr(result, "components", []):
        if getattr(component, "component_id", "") == component_id:
            return component.amount
    return fallback


def explain_result_residual(result):
    if hasattr(result, "residual"):
        return number_attr(result, "residual")
    return (
        number_attr(result, "total_pnl")
        - number_attr(result, "carry_pnl")
        - number_attr(result, "cash_pnl")
        - number_attr(result, "market_move_pnl")
    )


def make_pnl_explain_figure(go, pnl_results, trade_rows):
    sorted_results = sorted(pnl_results, key=lambda result: trade_id_sort_key(result.trade_id, trade_rows))
    trade_ids = [result.trade_id for result in sorted_results]
    trade_axis = trade_hierarchy_axis(trade_ids, trade_rows)
    trade_labels = trade_hierarchy_labels(trade_ids, trade_rows)
    height = bounded_chart_height(
        len(sorted_results),
        base_height=260,
        row_height=27,
        min_height=520,
        max_height=1060,
    )
    fig = go.Figure()
    fig.add_trace(
        go.Bar(
            name="Carry",
            orientation="h",
            x=[
                explain_component_amount(result, "carry", number_attr(result, "carry_pnl")) for result in sorted_results
            ],
            y=trade_axis,
            marker_color=WARNING_COLOR,
        )
    )
    fig.add_trace(
        go.Bar(
            name="Cash",
            orientation="h",
            x=[explain_component_amount(result, "cash", number_attr(result, "cash_pnl")) for result in sorted_results],
            y=trade_axis,
            marker_color=DASHBOARD_COLORWAY[1],
        )
    )
    fig.add_trace(
        go.Bar(
            name="Market Move",
            orientation="h",
            x=[
                explain_component_amount(result, "market_move", number_attr(result, "market_move_pnl"))
                for result in sorted_results
            ],
            y=trade_axis,
            marker_color=POSITIVE_COLOR,
        )
    )
    fig.add_trace(
        go.Bar(
            name="Residual",
            orientation="h",
            x=[
                explain_component_amount(result, "residual", explain_result_residual(result))
                for result in sorted_results
            ],
            y=trade_axis,
            marker_color=NEGATIVE_COLOR,
        )
    )
    apply_finance_layout(
        fig,
        barmode="relative",
        height=height,
        showlegend=True,
        title="P&L Explain by Position",
    )
    fig.update_layout(
        margin={
            "b": 86,
            "l": categorical_left_margin(trade_labels, min_margin=180, max_margin=320, char_width=4),
            "r": 38,
            "t": 82,
        }
    )
    fig.update_xaxes(title_text="P&L")
    fig.update_yaxes(autorange="reversed", automargin=True, type="multicategory")
    return fig


def make_factor_scenario_figure(go, make_subplots, factors, scenarios):
    factors_sorted = sorted(factors, key=factor_sort_key)
    scenarios_sorted = sorted(scenarios, key=lambda scenario: scenario_sort_key(scenario.name))
    factor_ids = [factor.factor_id for factor in factors_sorted]
    factor_axis = factor_hierarchy_axis(factors_sorted)
    scenario_names = [scenario.name for scenario in scenarios_sorted]
    scenario_axis = scenario_hierarchy_axis(scenario_names)
    z = [[scenario.factor_shocks.get(factor_id, 0.0) for factor_id in factor_ids] for scenario in scenarios_sorted]
    factor_customdata = [[[scenario.name, factor_id] for factor_id in factor_ids] for scenario in scenarios_sorted]
    factor_type_counts = defaultdict(int)
    for factor in factors_sorted:
        factor_type_counts[enum_label(factor.factor_type)] += 1
    factor_type_names = sorted(factor_type_counts)
    height = bounded_chart_height(
        max(len(scenario_names), len(factor_ids)),
        base_height=640,
        row_height=16,
        min_height=1180,
        max_height=1520,
    )

    fig = make_subplots(
        rows=2,
        cols=1,
        row_heights=[0.62, 0.38],
        specs=[[{"type": "xy"}], [{"type": "xy"}]],
        subplot_titles=["Scenario Factor Shock Matrix", "Factor Coverage by Type"],
        vertical_spacing=0.24,
    )
    fig.add_trace(
        go.Heatmap(
            colorbar={"title": "Shock"},
            colorscale=DIVERGING_SCALE,
            customdata=factor_customdata,
            hovertemplate="<b>%{customdata[0]}</b><br>%{customdata[1]}<br>Shock: %{z:.6f}<extra></extra>",
            x=factor_axis,
            y=scenario_axis,
            zmid=0.0,
            z=z,
        ),
        row=1,
        col=1,
    )
    fig.add_trace(
        go.Bar(
            x=factor_type_names,
            y=[factor_type_counts[name] for name in factor_type_names],
            marker_color=POSITIVE_COLOR,
        ),
        row=2,
        col=1,
    )
    apply_finance_layout(fig, height=height)
    fig.update_layout(
        margin={
            "b": 156,
            "l": categorical_left_margin(scenario_hierarchy_labels(scenario_names)),
            "r": 42,
            "t": 82,
        }
    )
    fig.update_xaxes(
        **diagonal_category_axis(multicategory=True),
        row=1,
        col=1,
    )
    fig.update_yaxes(autorange="reversed", automargin=True, row=1, col=1, type="multicategory")
    fig.update_xaxes(
        **diagonal_category_axis(title_text="Factor Type"),
        row=2,
        col=1,
    )
    fig.update_yaxes(title_text="Factors", row=2, col=1)
    return fig


def portfolio_label(portfolio):
    return getattr(portfolio, "portfolio_name", "") or getattr(portfolio, "portfolio_id", "Portfolio")


def portfolio_description(portfolio):
    description = getattr(portfolio, "description", "")
    if description:
        return description
    return "Portfolio analytics generated from current valuation, stress, risk, P&L explain, and Monte Carlo outputs."


def make_dashboard_cards(
    portfolio,
    valuation_results,
    total_npv,
    mc_results,
    performance_history,
):
    trade_rows = build_trade_rows(portfolio, valuation_results)
    mc_card = mc_results[0][1] if mc_results else None
    asset_class_count = len(set(row["asset_class"] for row in trade_rows))

    return [
        {
            "label": "Portfolio NPV",
            "value": money(total_npv),
            "note": f"{len(trade_rows)} positions across {asset_class_count} asset classes",
        },
        {
            "label": "2Y Return",
            "value": pct(performance_history["portfolio_return"]),
            "note": "Default performance lookback",
        },
        {
            "label": "2Y Benchmark",
            "value": pct(performance_history["benchmark_return"]),
            "note": performance_history["benchmark_name"],
        },
        {
            "label": "2Y Active Return",
            "value": pct(performance_history["active_return"]),
            "note": "Portfolio return minus benchmark return",
        },
        {
            "label": "2Y Max Drawdown",
            "value": pct(performance_history["max_portfolio_drawdown"]["amount"]),
            "note": performance_history["max_portfolio_drawdown"]["date"],
        },
        {
            "label": "Monte Carlo VaR / ES",
            "value": f"{money(mc_card.var_95)} / {money(mc_card.expected_shortfall_95)}" if mc_card else "n/a",
            "note": mc_results[0][0] if mc_results else "No Monte Carlo results",
        },
    ]


def make_dashboard_figures(
    go,
    make_subplots,
    portfolio,
    valuation_results,
    stress_results,
    risk_results,
    pnl_results,
    mc_results,
    factors,
    scenarios,
    market_as_of,
    performance_history,
):
    trade_rows = build_trade_rows(portfolio, valuation_results)
    return [
        (
            "Portfolio Performance Review",
            make_performance_review_figure(go, make_subplots, performance_history),
            (
                "Model total-return history versus the policy benchmark, with 1Y, 2Y, 5Y, 10Y, "
                "inception, and calendar-year views."
            ),
        ),
        (
            "Portfolio Allocation",
            make_portfolio_allocation_figure(go, make_subplots, trade_rows),
            (
                "Use Asset, Book, or Position hierarchy; funded bond and note bars show premium "
                "or discount to par while tables keep raw NPV."
            ),
        ),
        (
            "Support Diagnostics",
            make_support_diagnostics_panel(trade_rows),
            (
                "Pricing support exceptions are listed by position so unsupported or partially "
                "supported instruments are explicit."
            ),
        ),
        (
            "Position Inventory",
            make_trade_inventory_figure(go, trade_rows),
            (
                "Rows are portfolio positions; asset class and instrument type are shown "
                "separately for sorting and grouping."
            ),
        ),
        (
            "Risk Exposures by Position",
            make_risk_measures_figure(go, make_subplots, risk_results, trade_rows),
            (
                "Rows are positions, grouped by asset class and instrument type; rate and credit "
                "buckets are shown by tenor group."
            ),
        ),
        (
            "Scenario Stress P&L",
            make_historical_stress_figure(go, make_subplots, stress_results, trade_rows),
            "Stress scenarios are one-step shocks and P&L explainers, not portfolio performance through time.",
        ),
        (
            "Monte Carlo VaR and Future Path Fan",
            make_monte_carlo_figure(go, make_subplots, mc_results, market_as_of),
            "Simulated portfolio P&L distribution and path fan for the selected risk horizon.",
        ),
        (
            "P&L Explain by Position",
            make_pnl_explain_figure(go, pnl_results, trade_rows),
            "P&L components are shown by position with asset class and instrument type as grouping levels.",
        ),
        (
            "Risk Factor Scenario Coverage",
            make_factor_scenario_figure(go, make_subplots, factors, scenarios),
            "Scenario factors are grouped by risk-factor family so market-data coverage and shock breadth are visible.",
        ),
    ]


def make_dashboard_view(
    go,
    make_subplots,
    analytics,
    factors,
    scenarios,
    market_as_of,
):
    portfolio = analytics["portfolio"]
    valuation_results = analytics["valuation_results"]
    total_npv = analytics["total_npv"]
    stress_results = analytics["stress_results"]
    risk_results = analytics["risk_results"]
    pnl_results = analytics["pnl_results"]
    mc_results = analytics["mc_results"]
    trade_rows = build_trade_rows(portfolio, valuation_results)
    performance_history = build_performance_history(portfolio, trade_rows, market_as_of)
    supported_count = sum(1 for row in trade_rows if row["status"] == "supported")
    asset_class_count = len(set(row["asset_class"] for row in trade_rows))
    summary = f"{len(trade_rows)} positions | {supported_count} supported | {asset_class_count} asset classes"

    return {
        "cards": make_dashboard_cards(
            portfolio,
            valuation_results,
            total_npv,
            mc_results,
            performance_history,
        ),
        "description": portfolio_description(portfolio),
        "figures": make_dashboard_figures(
            go,
            make_subplots,
            portfolio,
            valuation_results,
            stress_results,
            risk_results,
            pnl_results,
            mc_results,
            factors,
            scenarios,
            market_as_of,
            performance_history,
        ),
        "id": getattr(portfolio, "portfolio_id", portfolio_label(portfolio)),
        "label": portfolio_label(portfolio),
        "summary": summary,
    }


def create_plotly_dashboard(
    portfolio,
    valuation_results,
    total_npv,
    stress_results,
    risk_results,
    pnl_results,
    mc_results,
    factors,
    scenarios,
    output_path,
    optimization_result=None,
    market_valuation_date=None,
    generated_at=None,
    portfolio_views=None,
):
    modules = optional_plotly_modules()
    if modules is None:
        return None
    go, make_subplots, pio = modules

    generated_at = generated_at or dashboard_generated_at()
    market_as_of = parse_valuation_date(market_valuation_date)
    market_as_of_label = format_date(market_as_of)
    generated_at_label = format_timestamp(generated_at)
    analytics_views = [
        {
            "mc_results": mc_results,
            "pnl_results": pnl_results,
            "portfolio": portfolio,
            "risk_results": risk_results,
            "stress_results": stress_results,
            "total_npv": total_npv,
            "valuation_results": valuation_results,
        }
    ]
    analytics_views.extend(portfolio_views or [])
    views = [
        make_dashboard_view(
            go,
            make_subplots,
            analytics,
            factors,
            scenarios,
            market_as_of,
        )
        for analytics in analytics_views
    ]

    return render_dashboard_html(
        "Quant Risk Platform Demo Dashboard",
        views,
        output_path,
        pio,
        {
            "generated_at": generated_at_label,
            "market_as_of": market_as_of_label,
        },
    )


def build_demo_dashboard_portfolio_views(
    demo_platform, primary_portfolio_id, market, market_path, factors, bindings, scenarios
):
    views = []
    seen = {primary_portfolio_id}
    portfolio_dir = demo_platform.project_root / "data" / "portfolios"
    dashboard_monte_carlo_cases = [demo_platform.MONTE_CARLO_CASES[0]]
    for portfolio_file in DASHBOARD_PORTFOLIOS:
        portfolio = demo_platform.qrp.load_portfolio(str(portfolio_dir / portfolio_file))
        if portfolio.portfolio_id in seen:
            continue
        views.append(
            demo_platform.compute_portfolio_analytics(
                portfolio,
                market,
                market_path,
                factors,
                bindings,
                scenarios,
                dashboard_monte_carlo_cases,
            )
        )
        seen.add(portfolio.portfolio_id)
    return views


def generate_demo_dashboard(output_path=None, open_browser=True):
    import demo_platform

    project_root = demo_platform.project_root
    market_path = project_root / "data" / "market" / "demo_market.json"
    portfolio_path = project_root / "data" / "portfolios" / "demo_portfolio.json"
    scenario_path = project_root / "data" / "scenarios" / "demo_scenarios.json"
    output_path = Path(output_path) if output_path else project_root / "reports" / "demo_risk_dashboard.html"

    market = demo_platform.qrp.load_market(str(market_path))
    portfolio = demo_platform.qrp.load_portfolio(str(portfolio_path))
    _, factors, bindings, scenarios = demo_platform.load_factor_scenario_set(scenario_path)
    analytics = demo_platform.compute_portfolio_analytics(
        portfolio,
        market,
        market_path,
        factors,
        bindings,
        scenarios,
    )
    portfolio_views = build_demo_dashboard_portfolio_views(
        demo_platform,
        portfolio.portfolio_id,
        market,
        market_path,
        factors,
        bindings,
        scenarios,
    )
    dashboard_path = create_plotly_dashboard(
        portfolio=portfolio,
        valuation_results=analytics["valuation_results"],
        total_npv=analytics["total_npv"],
        stress_results=analytics["stress_results"],
        risk_results=analytics["risk_results"],
        pnl_results=analytics["pnl_results"],
        mc_results=analytics["mc_results"],
        factors=factors,
        market_valuation_date=market.valuation_date,
        scenarios=scenarios,
        output_path=output_path,
        portfolio_views=portfolio_views,
    )
    if dashboard_path is None:
        print("Dashboard was not generated, so there is no webpage to open.")
        return None

    print(f"Dashboard written to: {dashboard_path}")
    if open_browser:
        opened = webbrowser.open(dashboard_path.resolve().as_uri())
        if opened:
            print("Dashboard opened in your default web browser.")
        else:
            print("Dashboard written, but the browser did not confirm that it opened.")
    return dashboard_path


def parse_args():
    parser = argparse.ArgumentParser(description="Generate the Quant Risk Platform Plotly demo dashboard.")
    parser.add_argument(
        "--output",
        type=Path,
        default=None,
        help="Dashboard HTML output path. Defaults to reports/demo_risk_dashboard.html.",
    )
    parser.add_argument(
        "--no-open",
        action="store_true",
        help="Write the dashboard without opening the default web browser.",
    )
    return parser.parse_args()


if __name__ == "__main__":
    args = parse_args()
    raise SystemExit(0 if generate_demo_dashboard(args.output, open_browser=not args.no_open) else 1)
