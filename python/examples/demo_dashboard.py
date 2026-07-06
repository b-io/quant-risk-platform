"""Creates the Plotly HTML dashboard used by the demo platform workflow."""

import html
import json
import math
from collections import defaultdict
from datetime import date, datetime, timedelta


FINANCE_FONT = '"IBM Plex Sans", "Aptos", "Bahnschrift", sans-serif'
MONO_FONT = '"IBM Plex Mono", "Cascadia Mono", "SFMono-Regular", monospace'
CATEGORY_TICKANGLE = -35
CATEGORY_TICKFONT = {"family": FINANCE_FONT, "size": 9}

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
        print(
            f"Skipped: Plotly dashboard dependencies are not available in this Python environment ({exc})."
        )
        print("Install the optional dashboard dependencies with:")
        print("  uv sync --project python --extra dashboard")
        print("Then run the demo with the uv-managed interpreter:")
        print("  uv run --project python python python/examples/demo_platform.py")
        return None
    return go, make_subplots, pio


def dashboard_style_options():
    return "\n".join(
        f'<option value="{html.escape(key)}">{html.escape(preset["label"])}</option>'
        for key, preset in DASHBOARD_STYLE_PRESETS.items()
    )


def active_style_preset():
    return DASHBOARD_STYLE_PRESETS[DEFAULT_STYLE_PRESET]


def style_color(index):
    colorway = active_style_preset()["colorway"]
    return colorway[index % len(colorway)]


def stable_color_index(value, modulo):
    if modulo <= 0:
        return 0
    return (
        sum((index + 1) * ord(char) for index, char in enumerate(str(value))) % modulo
    )


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
    mixed = [
        round(base * (1.0 - clamped) + mix * clamped)
        for base, mix in zip(base_rgb, mix_rgb)
    ]
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
    mix_color, weight = PRODUCT_COLOR_VARIANTS[
        stable_color_index(product_node_id, len(PRODUCT_COLOR_VARIANTS))
    ]
    return mix_hex(asset_color, mix_color, weight)


def trade_family_color(product_color, trade_node_id):
    mix_color, weight = TRADE_COLOR_VARIANTS[
        stable_color_index(trade_node_id, len(TRADE_COLOR_VARIANTS))
    ]
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
        product_color = product_family_color(
            asset_color, product_node_id_from_trade_node(node)
        )
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
    if any(
        token in normalized
        for token in ("error", "missing", "not_supported", "unsupported")
    ):
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
    extras = sorted(
        status for status in status_counts if status not in SUPPORT_STATUS_ORDER
    )
    return known + extras


def sort_text(value):
    return str(value or "").casefold()


def split_identifier(value):
    return [
        token
        for token in str(value or "").replace(":", "_").replace("-", "_").split("_")
        if token
    ]


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
    return [
        f"{row.get('asset_class', '')} / {row.get('product_type', '')} / {row.get('trade_id', '')}"
        for row in rows
    ]


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
    if tokens[0] == "FX" or (
        tokens[0] == "USD" and len(tokens) > 1 and tokens[1] in {"STRENGTH", "WEAKNESS"}
    ):
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
        [
            axis_tick_label(name, max_line_length=16, max_lines=2)
            for name in scenario_names
        ],
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
        tuple(
            sort_text(token) for token in split_identifier(short_factor_id(factor_id))
        ),
    )


def factor_hierarchy_axis(factors):
    return [
        [factor_group_label(factor.factor_id) for factor in factors],
        [factor_subgroup_label(factor.factor_id) for factor in factors],
        [
            axis_tick_label(
                short_factor_id(factor.factor_id), max_line_length=14, max_lines=2
            )
            for factor in factors
        ],
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
    label = (
        f"{getattr(portfolio, 'portfolio_id', '')} {portfolio_label(portfolio)}".lower()
    )
    return "demo" in label and "portfolio" in label


def balance_demo_exposures(portfolio, rows):
    if not uses_balanced_demo_exposure(portfolio):
        return rows

    asset_totals = defaultdict(float)
    for row in rows:
        asset_totals[row["asset_class"]] += max(0.0, float(row["raw_exposure"] or 0.0))
    positive_assets = {
        asset_class: total for asset_class, total in asset_totals.items() if total > 0.0
    }
    if len(positive_assets) < 2:
        return rows

    target_total = sum(positive_assets.values())
    target_asset_exposure = target_total / len(positive_assets)
    for row in rows:
        raw_exposure = max(0.0, float(row["raw_exposure"] or 0.0))
        asset_total = positive_assets.get(row["asset_class"], 0.0)
        row["exposure"] = (
            raw_exposure * target_asset_exposure / asset_total
            if asset_total > 0.0
            else raw_exposure
        )
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
                "model": getattr(valuation, "model_name", "")
                or valuation.tags.get("model", ""),
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
        "colors": [
            semantic_node_color(segment_ids[label], index)
            for index, label in enumerate(labels)
        ],
        "ids": [segment_ids[label] for label in labels],
        "labels": labels,
        "values": [totals[label] for label in labels],
    }


def treemap_node_colors(node_ids):
    return [
        semantic_node_color(node_id, index) for index, node_id in enumerate(node_ids)
    ]


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
    clean = {
        key: max(0.0, float(value or 0.0))
        for key, value in weights.items()
        if float(value or 0.0) > 0.0
    }
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
    label = (
        f"{getattr(portfolio, 'portfolio_id', '')} {portfolio_label(portfolio)}".lower()
    )
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
    daily_noise = (
        model["vol"]
        / math.sqrt(252.0)
        * stable_wave(f"{asset_class}:{index}", index, scale=0.22)
    )
    return daily_drift + model["beta"] * common + daily_noise


def weighted_daily_return(weights, index, sample_count):
    return sum(
        weight * asset_daily_return(asset_class, index, sample_count)
        for asset_class, weight in weights.items()
    )


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
        "active_return": cumulative_return(period_portfolio)
        - cumulative_return(period_benchmark),
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
        label: performance_period_summary(
            dates, portfolio_values, benchmark_values, lookback_days
        )
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
        "calendar_year_returns": calendar_year_returns(
            dates, portfolio_values, benchmark_values
        ),
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


def apply_finance_layout(
    fig, height, showlegend=False, barmode=None, title=None, yaxis_title=None
):
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
    style_options = dashboard_style_options()
    style_presets_json = json.dumps(DASHBOARD_STYLE_PRESETS, sort_keys=True)
    portfolio_options = "\n".join(
        f'<option value="{html.escape(view["id"])}">{html.escape(view["label"])}</option>'
        for view in views
    )
    plotly_index = 0
    view_html = []
    for view_index, view in enumerate(views):
        card_html = "\n".join(
            f"""
            <section class="card is-loading">
              <div class="card-label">{html.escape(card["label"])}</div>
              <div class="card-value">{html.escape(card["value"])}</div>
              <div class="card-note">{html.escape(card["note"])}</div>
            </section>
            """
            for card in view["cards"]
        )
        figure_html = []
        for figure in view["figures"]:
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
                panel_body = pio.to_html(
                    fig, full_html=False, include_plotlyjs=include_plotly
                )
            allocation_controls = ""
            if name == "Portfolio Allocation":
                allocation_controls = """
                <div class="allocation-mode" data-allocation-mode aria-label="Allocation hierarchy">
                  <button type="button" data-allocation-view="asset" aria-pressed="true">Asset</button>
                  <button type="button" data-allocation-view="book" aria-pressed="false">Book</button>
                  <button type="button" data-allocation-view="position" aria-pressed="false">Position</button>
                </div>
                """
            figure_html.append(f"""
            <section class="panel is-loading">
              <div class="panel-heading">
                <h2>{html.escape(name)}</h2>
                <div class="panel-actions">
                  {allocation_controls}
                  <button class="panel-reset" type="button" data-panel-reset>Reset slice</button>
                </div>
              </div>
              {f'<p class="panel-note">{html.escape(note)}</p>' if note else ""}
              <div class="panel-filter" data-panel-filter hidden></div>
              <div class="panel-body">
                <div class="panel-loading" aria-hidden="true"><span></span></div>
                {panel_body}
              </div>
            </section>
            """)
        hidden = "" if view_index == 0 else " hidden"
        view_html.append(f"""
        <section class="portfolio-view" data-portfolio-view="{html.escape(view["id"])}"{hidden}>
          <div class="view-heading">
            <div>
              <h2>{html.escape(view["label"])}</h2>
              <p>{html.escape(view["description"])}</p>
            </div>
            <span>{html.escape(view["summary"])}</span>
          </div>
          <div class="cards">{card_html}</div>
          {"".join(figure_html)}
        </section>
        """)
    view_html = "\n".join(view_html)
    initial_theme_script = """
  <script>
    (function () {
      const savedTheme = localStorage.getItem("qrp-dashboard-theme");
      const savedStyle = localStorage.getItem("qrp-dashboard-style");
      const systemTheme = window.matchMedia("(prefers-color-scheme: dark)").matches ? "dark" : "light";
      document.documentElement.dataset.theme = savedTheme || systemTheme;
      document.documentElement.dataset.palette = savedStyle || "institutional";
    }());
  </script>
"""
    theme_script = """
  <script>
    (function () {
      const stylePresets = __STYLE_PRESETS__;
      window.qrpDashboardStylePresets = stylePresets;
      const themeStorageKey = "qrp-dashboard-theme";
      const styleStorageKey = "qrp-dashboard-style";
      const button = document.querySelector("[data-theme-toggle]");
      const label = document.querySelector("[data-theme-label]");
      const portfolioSelect = document.querySelector("[data-portfolio-select]");
      const styleSelect = document.querySelector("[data-style-select]");
      const portfolioStorageKey = "qrp-dashboard-portfolio";
      const contrastPalettes = {
        light: {
          font: "#16231d",
          grid: "rgba(35, 55, 47, 0.13)",
          heatNeutral: "#f5eddc",
          line: "rgba(35, 55, 47, 0.25)",
          paper: "rgba(0,0,0,0)",
          plot: "rgba(0,0,0,0)",
          tableCell: "#fff8eb",
          tableCellFont: "#16231d",
          tableHeader: "#102820",
          tableHeaderFont: "#f4fff9",
          tableMuted: "#647269"
        },
        dark: {
          font: "#e7f4ef",
          grid: "rgba(136, 164, 151, 0.18)",
          heatNeutral: "#102730",
          line: "rgba(136, 164, 151, 0.32)",
          paper: "rgba(0,0,0,0)",
          plot: "rgba(0,0,0,0)",
          tableCell: "#0f1d26",
          tableCellFont: "#e7f4ef",
          tableHeader: "#64d6c2",
          tableHeaderFont: "#061016",
          tableMuted: "#91a69e"
        }
      };

      function activeStyleKey() {
        const key = document.documentElement.dataset.palette || "institutional";
        return stylePresets[key] ? key : "institutional";
      }

      function styleTokens(key) {
        return stylePresets[key] || stylePresets.institutional;
      }

      function plotNodes() {
        return Array.from(document.querySelectorAll(".js-plotly-plot"));
      }

      function scaleForStyle(preset, tokens) {
        const neutral = tokens.heatNeutral || "#f5eddc";
        return [
          [0, preset.danger],
          [0.34, mixHex(preset.danger, neutral, 0.68)],
          [0.5, neutral],
          [0.66, mixHex(preset.accent, neutral, 0.68)],
          [1, preset.accent]
        ];
      }

      function withAlpha(hex, alpha) {
        const clean = String(hex || "").replace("#", "");
        if (clean.length !== 6) return hex;
        const r = parseInt(clean.slice(0, 2), 16);
        const g = parseInt(clean.slice(2, 4), 16);
        const b = parseInt(clean.slice(4, 6), 16);
        return `rgba(${r}, ${g}, ${b}, ${alpha})`;
      }

      function mixHex(baseColor, mixColor, weight) {
        function toRgb(hex) {
          const clean = String(hex || "").replace("#", "");
          if (clean.length !== 6) return null;
          return [
            parseInt(clean.slice(0, 2), 16),
            parseInt(clean.slice(2, 4), 16),
            parseInt(clean.slice(4, 6), 16)
          ];
        }
        const base = toRgb(baseColor);
        const mix = toRgb(mixColor);
        if (!base || !mix || base.some(Number.isNaN) || mix.some(Number.isNaN)) {
          return baseColor;
        }
        const clamped = Math.max(0, Math.min(1, Number(weight || 0)));
        const channel = (index) => Math.round(base[index] * (1 - clamped) + mix[index] * clamped);
        return `#${[0, 1, 2].map((index) => channel(index).toString(16).padStart(2, "0")).join("")}`;
      }

      const productColorVariants = [
        ["#000000", 0.10],
        ["#ffffff", 0.12],
        ["#000000", 0.22],
        ["#ffffff", 0.28],
        ["#000000", 0.32],
        ["#ffffff", 0.38]
      ];
      const tradeColorVariants = [
        ["#000000", 0.04],
        ["#000000", 0.08],
        ["#000000", 0.12],
        ["#000000", 0.16]
      ];

      function stableColorIndex(value, modulo) {
        if (!modulo) return 0;
        return Array.from(String(value || "")).reduce((total, char, index) => {
          return total + ((index + 1) * char.charCodeAt(0));
        }, 0) % modulo;
      }

      function nodeAssetClass(nodeId) {
        const id = String(nodeId || "");
        if (id.startsWith("asset:")) {
          return id.slice("asset:".length);
        }
        if (id.startsWith("product:")) {
          const parts = id.split(":");
          return parts.length > 1 ? parts[1] : "";
        }
        if (id.startsWith("bookAsset:")) {
          const separator = id.lastIndexOf("|");
          return separator >= 0 ? id.slice(separator + 1) : "";
        }
        if (id.startsWith("trade:")) {
          const parts = id.split(":");
          return parts.length > 3 ? parts[1] : "";
        }
        return "";
      }

      function productNodeIdFromTradeNode(nodeId) {
        const parts = String(nodeId || "").split(":");
        if (parts.length < 4 || parts[0] !== "trade") {
          return "";
        }
        return `product:${parts[1]}:${parts[2]}`;
      }

      function tradeNodeIdFromRow(row) {
        return `trade:${row.asset_class}:${row.product_type}:${row.trade_id}`;
      }

      function tradeAxisValues(rows) {
        return [
          rows.map((row) => row.asset_class || ""),
          rows.map((row) => row.product_type || ""),
          rows.map((row) => row.trade_id || "")
        ];
      }

      function tradeCustomData(rows) {
        return rows.map((row) => [
          row.asset_class || "",
          row.product_type || "",
          row.trade_id || ""
        ]);
      }

      function traceTradeIds(trace) {
        const yValues = trace.y || [];
        return Array.isArray(yValues[0]) ? (yValues[yValues.length - 1] || []) : yValues;
      }

      function tradeIdFromPoint(point) {
        if (Array.isArray(point.customdata) && point.customdata.length >= 3) {
          return point.customdata[2];
        }
        if (Array.isArray(point.y)) {
          return point.y[point.y.length - 1];
        }
        return point.y;
      }

      function familyColor(baseColor, nodeId, variants) {
        const [mixColor, weight] = variants[stableColorIndex(nodeId, variants.length)];
        return mixHex(baseColor, mixColor, weight);
      }

      function nodeColor(nodeId, pointIndex, preset) {
        const id = String(nodeId || "");
        if (id === "portfolio") {
          return preset.amber;
        }
        const colorway = preset.colorway || [];
        const assetColors = preset.asset_colors || {};
        const fallback = colorway[pointIndex % Math.max(colorway.length, 1)] || preset.accent;
        const assetColor = assetColors[nodeAssetClass(id)] || fallback;
        if (id.startsWith("product:")) {
          return familyColor(assetColor, id, productColorVariants);
        }
        if (id.startsWith("trade:")) {
          const productColor = familyColor(assetColor, productNodeIdFromTradeNode(id), productColorVariants);
          return familyColor(productColor, id, tradeColorVariants);
        }
        return assetColor;
      }

      function portfolioBarColors(plot, trace, traceIndex, preset) {
        const meta = plot.layout && plot.layout.meta;
        if (!meta || meta.qrpPanel !== "portfolio_allocation") {
          return null;
        }
        if (traceIndex === 2) {
          const rows = meta.tradeRows || [];
          const rowsById = new Map(rows.map((row) => [row.trade_id, row]));
          return traceTradeIds(trace).map((tradeId, pointIndex) => {
            const row = rowsById.get(tradeId);
            if (!row) {
              return preset.colorway[pointIndex % preset.colorway.length] || preset.accent;
            }
            return Number(row.display_npv ?? row.npv ?? 0) < 0 ? preset.danger : nodeColor(tradeNodeIdFromRow(row), pointIndex, preset);
          });
        }
        return null;
      }

      function barColorsForTrace(trace, preset) {
        const name = String(trace.name || "");
        const axisValues = trace.orientation === "h" ? (trace.x || []) : (trace.y || []);
        const categoryValues = trace.orientation === "h" ? (trace.y || []) : (trace.x || []);
        if (name === "Carry") return axisValues.map(() => preset.amber);
        if (name === "Cash") return axisValues.map(() => preset.accent2);
        if (name === "Market Move" || name === "PV01") return axisValues.map(() => preset.accent);
        if (name === "Residual" || name === "CS01") return axisValues.map(() => preset.danger);
        return axisValues.map((value, pointIndex) => {
          const category = String(categoryValues[pointIndex] || "");
          if (category.includes("partial")) return preset.amber;
          return Number(value || 0) < 0 ? preset.danger : preset.accent;
        });
      }

      function treemapColorsForTrace(trace, preset) {
        return (trace.ids || []).map((id, pointIndex) => nodeColor(id, pointIndex, preset));
      }

      function plotTextColor(theme, tokens, preset) {
        return theme === "dark" ? tokens.font : (preset.fill_text || tokens.font);
      }

      function scatterColorsForTrace(trace, preset, theme) {
        const name = String(trace.name || "").toLowerCase();
        const mutedAlpha = theme === "dark" ? 0.34 : 0.22;
        if (name.includes("benchmark") && name.includes("drawdown")) {
          return { line: withAlpha(preset.danger, theme === "dark" ? 0.78 : 0.66), marker: preset.danger };
        }
        if (name.includes("benchmark")) {
          return { line: preset.accent2 || preset.accent, marker: preset.accent2 || preset.accent };
        }
        if (name.includes("drawdown")) {
          return { line: preset.danger, marker: preset.danger };
        }
        if (name.includes("portfolio")) {
          return { line: preset.accent, marker: preset.accent };
        }
        if (name.includes("mean")) {
          return { line: preset.danger, marker: preset.danger };
        }
        return { line: withAlpha(preset.accent, mutedAlpha), marker: preset.accent };
      }

      function relayoutForTheme(plot, theme, styleKey) {
        if (!window.Plotly || !plot.layout) {
          return Promise.resolve();
        }
        const stateKey = `${theme}:${styleKey}`;
        if (plot.dataset.qrpVisualState === stateKey) {
          return Promise.resolve();
        }
        plot.dataset.qrpVisualState = stateKey;
        const tokens = contrastPalettes[theme];
        const preset = styleTokens(styleKey);
        const updates = {
          "colorway": preset.colorway,
          "font.color": tokens.font,
          "hoverlabel.bgcolor": theme === "dark" ? mixHex(tokens.tableCell, "#000000", 0.12) : tokens.tableCell,
          "hoverlabel.bordercolor": theme === "dark" ? withAlpha(preset.accent, 0.42) : withAlpha("#102820", 0.18),
          "hoverlabel.font.color": tokens.font,
          "legend.bgcolor": "rgba(0,0,0,0)",
          "legend.font.color": tokens.font,
          "paper_bgcolor": tokens.paper,
          "plot_bgcolor": tokens.plot
        };

        Object.keys(plot.layout)
          .filter((key) => /^[xy]axis\\d*$/.test(key))
          .forEach((axisName) => {
            updates[`${axisName}.gridcolor`] = tokens.grid;
            updates[`${axisName}.linecolor`] = tokens.line;
            updates[`${axisName}.tickfont.color`] = tokens.font;
            updates[`${axisName}.title.font.color`] = tokens.font;
            updates[`${axisName}.zerolinecolor`] = tokens.line;
            if (plot.layout[axisName] && plot.layout[axisName].rangeselector) {
              updates[`${axisName}.rangeselector.activecolor`] = withAlpha(preset.accent, theme === "dark" ? 0.38 : 0.26);
              updates[`${axisName}.rangeselector.bgcolor`] = theme === "dark"
                ? mixHex(tokens.tableCell, "#000000", 0.10)
                : "rgba(255,255,255,0.72)";
              updates[`${axisName}.rangeselector.bordercolor`] = withAlpha(preset.accent, theme === "dark" ? 0.44 : 0.24);
              updates[`${axisName}.rangeselector.font.color`] = tokens.font;
            }
          });

        (plot.layout.annotations || []).forEach((_, index) => {
          updates[`annotations[${index}].font.color`] = tokens.font;
        });
        (plot.layout.shapes || []).forEach((shape, index) => {
          const dash = shape.line && shape.line.dash;
          updates[`shapes[${index}].line.color`] = dash === "solid" ? preset.amber : preset.danger;
        });

        const operations = [Plotly.relayout(plot, updates)];

        (plot.data || []).forEach((trace, index) => {
          if (trace.type === "table") {
            operations.push(Plotly.restyle(plot, {
              "cells.fill.color": tokens.tableCell,
              "cells.font.color": tokens.tableCellFont,
              "header.fill.color": theme === "dark" ? preset.accent : tokens.tableHeader,
              "header.font.color": tokens.tableHeaderFont
            }, [index]));
          } else if (trace.type === "pie") {
            const ids = trace.ids || trace.customdata || [];
            const colors = ids.length
              ? ids.map((id, pointIndex) => nodeColor(id, pointIndex, preset))
              : preset.colorway;
            operations.push(Plotly.restyle(plot, {
              "marker.colors": [colors],
              "textfont.color": plotTextColor(theme, tokens, preset)
            }, [index]));
          } else if (trace.type === "treemap") {
            operations.push(Plotly.restyle(plot, {
              "marker.colors": [treemapColorsForTrace(trace, preset)],
              "textfont.color": plotTextColor(theme, tokens, preset)
            }, [index]));
          } else if (trace.type === "heatmap") {
            operations.push(Plotly.restyle(plot, {
              "colorscale": [scaleForStyle(preset, tokens)]
            }, [index]));
          } else if (trace.type === "bar") {
            const colors = portfolioBarColors(plot, trace, index, preset) || barColorsForTrace(trace, preset);
            operations.push(Plotly.restyle(plot, {
              "insidetextfont.color": preset.fill_text || "#102820",
              "marker.color": [colors.length ? colors : preset.accent],
              "outsidetextfont.color": tokens.font
            }, [index]));
          } else if (trace.type === "histogram") {
            operations.push(Plotly.restyle(plot, {
              "marker.color": preset.accent
            }, [index]));
          } else if (trace.type === "scatter") {
            const traceColors = scatterColorsForTrace(trace, preset, theme);
            operations.push(Plotly.restyle(plot, {
              "line.color": traceColors.line,
              "marker.color": traceColors.marker
            }, [index]));
          } else if (trace.type === "waterfall") {
            operations.push(Plotly.restyle(plot, {
              "decreasing.marker.color": preset.danger,
              "increasing.marker.color": preset.accent,
              "totals.marker.color": preset.amber
            }, [index]));
          }
        });
        return Promise.all(operations);
      }

      function applyTableTheme(theme, styleKey) {
        const tokens = contrastPalettes[theme] || contrastPalettes.light;
        const preset = styleTokens(styleKey);
        const root = document.documentElement.style;
        const header = theme === "dark" ? mixHex(preset.accent, "#061016", 0.16) : mixHex(preset.accent, "#102820", 0.68);
        const headerFont = theme === "dark" ? "#061016" : tokens.tableHeaderFont;
        const odd = theme === "dark"
          ? mixHex(tokens.tableCell, preset.accent, 0.07)
          : mixHex(tokens.tableCell, preset.accent, 0.05);
        const even = theme === "dark"
          ? mixHex(tokens.tableCell, preset.accent2 || preset.accent, 0.12)
          : mixHex(tokens.tableCell, preset.accent2 || preset.accent, 0.09);
        root.setProperty("--table-header", header);
        root.setProperty("--table-header-font", headerFont);
        root.setProperty("--table-cell", tokens.tableCell);
        root.setProperty("--table-cell-font", tokens.tableCellFont);
        root.setProperty("--table-row-odd", odd);
        root.setProperty("--table-row-even", even);
        root.setProperty("--table-border", theme === "dark" ? withAlpha(preset.accent, 0.20) : withAlpha("#102820", 0.12));
        root.setProperty("--table-muted", tokens.tableMuted || tokens.font);
        root.setProperty("--status-supported-bg", withAlpha(preset.accent, theme === "dark" ? 0.18 : 0.14));
        root.setProperty("--status-supported-fg", theme === "dark" ? mixHex(preset.accent, "#ffffff", 0.18) : preset.accent);
        root.setProperty("--status-partial-bg", withAlpha(preset.amber, theme === "dark" ? 0.22 : 0.18));
        root.setProperty("--status-partial-fg", theme === "dark" ? mixHex(preset.amber, "#ffffff", 0.18) : "#85622d");
        root.setProperty("--status-unsupported-bg", withAlpha(preset.danger, theme === "dark" ? 0.20 : 0.16));
        root.setProperty("--status-unsupported-fg", theme === "dark" ? mixHex(preset.danger, "#ffffff", 0.20) : preset.danger);
      }

      let visualRefreshFrame = 0;
      function scheduleVisualRefresh() {
        if (visualRefreshFrame) {
          cancelAnimationFrame(visualRefreshFrame);
        }
        visualRefreshFrame = requestAnimationFrame(() => {
          visualRefreshFrame = 0;
          const theme = document.documentElement.dataset.theme || "light";
          const styleKey = activeStyleKey();
          plotNodes().forEach((plot) => relayoutForTheme(plot, theme, styleKey));
        });
      }

      function applyStyle(styleKey) {
        const key = stylePresets[styleKey] ? styleKey : "institutional";
        const preset = styleTokens(key);
        window.qrpDashboardActiveStyle = preset;
        document.documentElement.dataset.palette = key;
        document.documentElement.style.setProperty("--accent", preset.accent);
        document.documentElement.style.setProperty("--accent-2", preset.accent2);
        document.documentElement.style.setProperty("--amber", preset.amber);
        document.documentElement.style.setProperty("--danger", preset.danger);
        document.documentElement.style.setProperty("--card-glow", `${preset.accent}26`);
        applyTableTheme(document.documentElement.dataset.theme || "light", key);
        localStorage.setItem(styleStorageKey, key);
        if (styleSelect) {
          styleSelect.value = key;
        }
        scheduleVisualRefresh();
      }

      function applyTheme(theme) {
        document.documentElement.dataset.theme = theme;
        applyTableTheme(theme, activeStyleKey());
        localStorage.setItem(themeStorageKey, theme);
        if (label) {
          label.textContent = theme === "dark" ? "Dark" : "Light";
        }
        if (button) {
          button.setAttribute("aria-label", theme === "dark" ? "Switch to light mode" : "Switch to dark mode");
          button.setAttribute("title", theme === "dark" ? "Switch to light mode" : "Switch to dark mode");
        }
        scheduleVisualRefresh();
      }

      const initialTheme = document.documentElement.dataset.theme || "light";
      const initialStyle = activeStyleKey();
      applyTheme(initialTheme);
      applyStyle(initialStyle);
      if (button) {
        button.addEventListener("click", () => {
          applyTheme(document.documentElement.dataset.theme === "dark" ? "light" : "dark");
        });
      }
      if (styleSelect) {
        styleSelect.addEventListener("change", () => {
          applyStyle(styleSelect.value);
        });
      }
      function activePortfolioView() {
        return document.querySelector(".portfolio-view:not([hidden])");
      }
      function resizeActivePlots() {
        const activeView = activePortfolioView();
        if (!activeView || !window.Plotly) {
          return;
        }
        activeView.querySelectorAll(".js-plotly-plot").forEach((plot) => {
          plot.dataset.qrpVisualState = "";
          Plotly.Plots.resize(plot);
        });
        scheduleVisualRefresh();
      }
      function applyPortfolioView(viewId) {
        const views = Array.from(document.querySelectorAll("[data-portfolio-view]"));
        const selected = views.find((view) => view.dataset.portfolioView === viewId) || views[0];
        if (!selected) {
          return;
        }
        views.forEach((view) => {
          view.hidden = view !== selected;
        });
        if (portfolioSelect) {
          portfolioSelect.value = selected.dataset.portfolioView;
        }
        localStorage.setItem(portfolioStorageKey, selected.dataset.portfolioView);
        window.setTimeout(resizeActivePlots, 40);
      }
      if (portfolioSelect) {
        portfolioSelect.addEventListener("change", () => {
          applyPortfolioView(portfolioSelect.value);
        });
        applyPortfolioView(localStorage.getItem(portfolioStorageKey) || portfolioSelect.value);
      }
      window.addEventListener("load", () => {
        applyTheme(document.documentElement.dataset.theme || "light");
        applyStyle(activeStyleKey());
        resizeActivePlots();
      });
    }());
  </script>
""".replace("__STYLE_PRESETS__", style_presets_json)
    interaction_script = """
  <script>
    (function () {
      function money(value) {
        const numeric = Number(value || 0);
        return `$${numeric.toLocaleString(undefined, { maximumFractionDigits: 0 })}`;
      }

      function compareTableValues(left, right, kind) {
        if (kind === "number") {
          return Number(left || 0) - Number(right || 0);
        }
        return String(left || "").localeCompare(String(right || ""), undefined, {
          numeric: true,
          sensitivity: "base"
        });
      }

      function sortTable(table, columnIndex, kind, direction) {
        const tbody = table.querySelector("tbody");
        if (!tbody) return;
        const multiplier = direction === "descending" ? -1 : 1;
        const rows = Array.from(tbody.querySelectorAll("tr"));
        rows.sort((leftRow, rightRow) => {
          const leftCell = leftRow.children[columnIndex];
          const rightCell = rightRow.children[columnIndex];
          const primary = compareTableValues(
            leftCell && leftCell.dataset.sortValue,
            rightCell && rightCell.dataset.sortValue,
            kind
          );
          if (primary !== 0) return primary * multiplier;
          return String(leftRow.dataset.defaultSort || "").localeCompare(
            String(rightRow.dataset.defaultSort || ""),
            undefined,
            { numeric: true, sensitivity: "base" }
          );
        });
        rows.forEach((row) => tbody.appendChild(row));
      }

      function initializeSortableTables() {
        document.querySelectorAll("[data-sortable-table]").forEach((table) => {
          if (table.dataset.sortReady === "true") return;
          table.dataset.sortReady = "true";
          table.querySelectorAll("thead button[data-sort-column]").forEach((button) => {
            button.addEventListener("click", () => {
              const current = button.getAttribute("aria-sort");
              const direction = current === "ascending" ? "descending" : "ascending";
              table.querySelectorAll("thead button[aria-sort]").forEach((header) => {
                header.setAttribute("aria-sort", "none");
              });
              button.setAttribute("aria-sort", direction);
              sortTable(
                table,
                Number(button.dataset.sortColumn || 0),
                button.dataset.sortKind || "text",
                direction
              );
            });
          });
        });
      }

      function compactLabel(label) {
        const text = String(label || "All portfolio");
        return text.length > 34 ? `${text.slice(0, 31)}...` : text;
      }

      function escapeHtmlText(value) {
        return String(value || "")
          .replaceAll("&", "&amp;")
          .replaceAll("<", "&lt;")
          .replaceAll(">", "&gt;");
      }

      function boldChartTitle(value) {
        return `<b>${escapeHtmlText(value)}</b>`;
      }

      function activePreset() {
        return window.qrpDashboardActiveStyle || {
          accent: "#5faea4",
          amber: "#d5ac63",
          asset_colors: {
            commodity: "#dfbf72",
            credit: "#bba1d9",
            crypto: "#9bc7d8",
            equity: "#e49a9d",
            fx: "#94b9e2",
            inflation: "#e5b08b",
            rates: "#7bc5ba",
            volatility: "#9bcfaa"
          },
          colorway: ["#7bc5ba", "#94b9e2", "#dfbf72", "#e49a9d", "#bba1d9", "#9bcfaa", "#9bc7d8"],
          danger: "#d3777c",
          fill_text: "#14231e"
        };
      }

      function plotLabelTextColor() {
        return (document.documentElement.dataset.theme || "light") === "dark"
          ? "#e7f4ef"
          : (activePreset().fill_text || "#16231d");
      }

      function mixHex(baseColor, mixColor, weight) {
        function toRgb(hex) {
          const clean = String(hex || "").replace("#", "");
          if (clean.length !== 6) return null;
          return [
            parseInt(clean.slice(0, 2), 16),
            parseInt(clean.slice(2, 4), 16),
            parseInt(clean.slice(4, 6), 16)
          ];
        }
        const base = toRgb(baseColor);
        const mix = toRgb(mixColor);
        if (!base || !mix || base.some(Number.isNaN) || mix.some(Number.isNaN)) {
          return baseColor;
        }
        const clamped = Math.max(0, Math.min(1, Number(weight || 0)));
        const channel = (index) => Math.round(base[index] * (1 - clamped) + mix[index] * clamped);
        return `#${[0, 1, 2].map((index) => channel(index).toString(16).padStart(2, "0")).join("")}`;
      }

      const productColorVariants = [
        ["#000000", 0.10],
        ["#ffffff", 0.12],
        ["#000000", 0.22],
        ["#ffffff", 0.28],
        ["#000000", 0.32],
        ["#ffffff", 0.38]
      ];
      const tradeColorVariants = [
        ["#000000", 0.04],
        ["#000000", 0.08],
        ["#000000", 0.12],
        ["#000000", 0.16]
      ];

      function stableColorIndex(value, modulo) {
        if (!modulo) return 0;
        return Array.from(String(value || "")).reduce((total, char, index) => {
          return total + ((index + 1) * char.charCodeAt(0));
        }, 0) % modulo;
      }

      function nodeAssetClass(nodeId) {
        const id = String(nodeId || "");
        if (id.startsWith("asset:")) {
          return id.slice("asset:".length);
        }
        if (id.startsWith("product:")) {
          const parts = id.split(":");
          return parts.length > 1 ? parts[1] : "";
        }
        if (id.startsWith("bookAsset:")) {
          const separator = id.lastIndexOf("|");
          return separator >= 0 ? id.slice(separator + 1) : "";
        }
        if (id.startsWith("trade:")) {
          const parts = id.split(":");
          return parts.length > 3 ? parts[1] : "";
        }
        return "";
      }

      function productNodeIdFromTradeNode(nodeId) {
        const parts = String(nodeId || "").split(":");
        if (parts.length < 4 || parts[0] !== "trade") {
          return "";
        }
        return `product:${parts[1]}:${parts[2]}`;
      }

      function familyColor(baseColor, nodeId, variants) {
        const [mixColor, weight] = variants[stableColorIndex(nodeId, variants.length)];
        return mixHex(baseColor, mixColor, weight);
      }

      function nodeColor(nodeId, pointIndex) {
        const preset = activePreset();
        const id = String(nodeId || "");
        if (id === "portfolio") {
          return preset.amber;
        }
        const colorway = preset.colorway || [];
        const assetColors = preset.asset_colors || {};
        const fallback = colorway[pointIndex % Math.max(colorway.length, 1)] || preset.accent;
        const assetColor = assetColors[nodeAssetClass(id)] || fallback;
        if (id.startsWith("product:")) {
          return familyColor(assetColor, id, productColorVariants);
        }
        if (id.startsWith("trade:")) {
          const productColor = familyColor(assetColor, productNodeIdFromTradeNode(id), productColorVariants);
          return familyColor(productColor, id, tradeColorVariants);
        }
        return assetColor;
      }

      function tradeNodeId(row) {
        return `trade:${row.asset_class}:${row.product_type}:${row.trade_id}`;
      }

      function bookNodeId(row) {
        return `book:${row.book || "Unassigned"}`;
      }

      function bookAssetNodeId(row) {
        return `bookAsset:${row.book || "Unassigned"}|${row.asset_class || ""}`;
      }

      function bookLabel(book) {
        return String(book || "Unassigned").replace(/^BOOK:[^:]+:/, "");
      }

      function tradeMetricValue(row) {
        return Number(row.display_npv ?? row.npv ?? 0);
      }

      function tradeMetricColor(row) {
        return tradeMetricValue(row) < 0 ? activePreset().danger : nodeColor(tradeNodeId(row), 0);
      }

      function tradeAxisValues(rows, mode) {
        if (mode === "book") {
          return [
            rows.map((row) => bookLabel(row.book)),
            rows.map((row) => row.asset_class || ""),
            rows.map((row) => row.trade_id || "")
          ];
        }
        if (mode === "position") {
          return rows.map((row) => row.trade_id || "");
        }
        return [
          rows.map((row) => row.asset_class || ""),
          rows.map((row) => row.product_type || ""),
          rows.map((row) => row.trade_id || "")
        ];
      }

      function tradeCustomData(rows) {
        return rows.map((row) => [
          row.asset_class || "",
          row.product_type || "",
          row.trade_id || "",
          Number(row.npv || 0)
        ]);
      }

      function rowExposure(row) {
        return Number(row.exposure || 0);
      }

      function sumExposure(rows) {
        return rows.reduce((total, row) => total + rowExposure(row), 0);
      }

      function paddedNumberRange(values, minimumSpan) {
        const numericValues = values.map((value) => Number(value || 0));
        let minValue = Math.min(0, ...numericValues);
        let maxValue = Math.max(0, ...numericValues);
        const span = Math.max(maxValue - minValue, minimumSpan || 1);
        const leftPadding = span * 0.16;
        const rightPadding = span * 0.16;
        if (minValue < 0) {
          minValue -= leftPadding;
        } else {
          minValue = 0;
        }
        if (maxValue > 0) {
          maxValue += rightPadding;
        } else {
          maxValue = 0;
        }
        return [minValue, maxValue];
      }

      function sortRows(rows, mode) {
        return rows.slice().sort((a, b) => {
          const left = mode === "book"
            ? `${bookLabel(a.book)}|${a.asset_class}|${a.product_type}|${a.trade_id}`
            : `${a.asset_class}|${a.product_type}|${a.trade_id}`;
          const right = mode === "book"
            ? `${bookLabel(b.book)}|${b.asset_class}|${b.product_type}|${b.trade_id}`
            : `${b.asset_class}|${b.product_type}|${b.trade_id}`;
          return String(left).localeCompare(String(right), undefined, { numeric: true, sensitivity: "base" });
        });
      }

      function buildTreemap(rows, mode, rootLabel) {
        const ids = ["portfolio"];
        const labels = [rootLabel || "All portfolio"];
        const parents = [""];
        const values = [sumExposure(rows)];
        const seen = new Set(["portfolio"]);
        const totals = new Map();

        function addTotal(id, value) {
          totals.set(id, (totals.get(id) || 0) + value);
        }

        rows.forEach((row) => {
          const exposure = rowExposure(row);
          if (mode === "book") {
            addTotal(bookNodeId(row), exposure);
            addTotal(bookAssetNodeId(row), exposure);
          } else if (mode !== "position") {
            addTotal(`asset:${row.asset_class}`, exposure);
            addTotal(`product:${row.asset_class}:${row.product_type}`, exposure);
          }
          addTotal(tradeNodeId(row), exposure);
        });

        function addNode(id, label, parent) {
          if (seen.has(id)) {
            return;
          }
          seen.add(id);
          ids.push(id);
          labels.push(label);
          parents.push(parent);
          values.push(totals.get(id) || 0);
        }

        sortRows(rows, mode).forEach((row) => {
          if (mode === "book") {
            const bookId = bookNodeId(row);
            const bookAssetId = bookAssetNodeId(row);
            addNode(bookId, bookLabel(row.book), "portfolio");
            addNode(bookAssetId, row.asset_class || "Other", bookId);
            addNode(tradeNodeId(row), row.trade_id || "", bookAssetId);
          } else if (mode === "position") {
            addNode(tradeNodeId(row), row.trade_id || "", "portfolio");
          } else {
            const assetId = `asset:${row.asset_class}`;
            const productId = `product:${row.asset_class}:${row.product_type}`;
            addNode(assetId, row.asset_class || "Other", "portfolio");
            addNode(productId, row.product_type || "Other", assetId);
            addNode(tradeNodeId(row), row.trade_id || "", productId);
          }
        });

        return {
          colors: ids.map((id, index) => nodeColor(id, index)),
          ids,
          labels,
          parents,
          values
        };
      }

      function buildDonut(rows, nodeId, mode) {
        const totals = new Map();
        const idsByLabel = new Map();

        function add(label, id, exposure) {
          totals.set(label, (totals.get(label) || 0) + exposure);
          idsByLabel.set(label, id);
        }

        if (mode === "book" && nodeId && nodeId.startsWith("book:")) {
          rows.forEach((row) => {
            add(row.asset_class, bookAssetNodeId(row), rowExposure(row));
          });
        } else if (mode === "book" && nodeId && nodeId.startsWith("bookAsset:")) {
          rows.forEach((row) => {
            add(row.trade_id, tradeNodeId(row), rowExposure(row));
          });
        } else if (mode === "position") {
          rows.forEach((row) => {
            add(row.trade_id, tradeNodeId(row), rowExposure(row));
          });
        } else if (nodeId && nodeId.startsWith("asset:")) {
          const assetClass = nodeId.slice("asset:".length);
          rows.forEach((row) => {
            add(row.product_type, `product:${assetClass}:${row.product_type}`, rowExposure(row));
          });
        } else if (nodeId && nodeId.startsWith("product:")) {
          rows.forEach((row) => {
            add(row.trade_id, tradeNodeId(row), rowExposure(row));
          });
        } else if (nodeId && nodeId.startsWith("trade:") && rows.length > 0) {
          rows.forEach((row) => {
            add(row.trade_id, tradeNodeId(row), rowExposure(row));
          });
        } else if (mode === "book") {
          rows.forEach((row) => {
            add(bookLabel(row.book), bookNodeId(row), rowExposure(row));
          });
        } else {
          rows.forEach((row) => {
            add(row.asset_class, `asset:${row.asset_class}`, rowExposure(row));
          });
        }

        const labels = Array.from(totals.keys()).sort((a, b) => a.localeCompare(b));
        const ids = labels.map((label) => idsByLabel.get(label));
        return {
          colors: ids.map((id, index) => nodeColor(id, index)),
          ids,
          labels,
          values: labels.map((label) => totals.get(label))
        };
      }

      function rowsForNode(rows, id) {
        if (!id || id === "portfolio") {
          return rows.slice();
        }
        if (id.startsWith("book:")) {
          const book = id.slice("book:".length);
          return rows.filter((row) => String(row.book || "Unassigned") === book);
        }
        if (id.startsWith("bookAsset:")) {
          const payload = id.slice("bookAsset:".length);
          const separator = payload.lastIndexOf("|");
          const book = separator >= 0 ? payload.slice(0, separator) : payload;
          const assetClass = separator >= 0 ? payload.slice(separator + 1) : "";
          return rows.filter((row) => String(row.book || "Unassigned") === book && row.asset_class === assetClass);
        }
        if (id.startsWith("asset:")) {
          const assetClass = id.slice("asset:".length);
          return rows.filter((row) => row.asset_class === assetClass);
        }
        if (id.startsWith("product:")) {
          const [, assetClass, ...productParts] = id.split(":");
          const productType = productParts.join(":");
          return rows.filter((row) => row.asset_class === assetClass && row.product_type === productType);
        }
        if (id.startsWith("trade:")) {
          const parts = id.split(":");
          const tradeId = parts.length > 3 ? parts.slice(3).join(":") : id.slice("trade:".length);
          return rows.filter((row) => row.trade_id === tradeId);
        }
        return rows.slice();
      }

      function labelForNode(id, fallback) {
        if (!id || id === "portfolio") {
          return "All portfolio";
        }
        if (id.startsWith("book:")) {
          return `Book: ${bookLabel(id.slice("book:".length))}`;
        }
        if (id.startsWith("bookAsset:")) {
          const payload = id.slice("bookAsset:".length);
          const separator = payload.lastIndexOf("|");
          const book = separator >= 0 ? payload.slice(0, separator) : payload;
          const assetClass = separator >= 0 ? payload.slice(separator + 1) : "";
          return `${bookLabel(book)} / ${assetClass}`;
        }
        if (id.startsWith("asset:")) {
          return `Asset class: ${id.slice("asset:".length)}`;
        }
        if (id.startsWith("product:")) {
          const [, assetClass, ...productParts] = id.split(":");
          return `${assetClass} / ${productParts.join(":")}`;
        }
        if (id.startsWith("trade:")) {
          const parts = id.split(":");
          const tradeId = parts.length > 3 ? parts.slice(3).join(":") : id.slice("trade:".length);
          return `Position: ${tradeId}`;
        }
        return fallback || "Selected slice";
      }

      function pointNodeId(plot, point) {
        const trace = plot.data && plot.data[point.curveNumber];
        const candidates = [
          point.id,
          trace && trace.ids && trace.ids[point.pointNumber],
          trace && trace.customdata && trace.customdata[point.pointNumber],
          point.customdata,
          point.label
        ];
        const value = candidates.find((candidate) => candidate !== undefined && candidate !== null && candidate !== "");
        if (Array.isArray(value)) {
          return value[0] || "portfolio";
        }
        return String(value || "portfolio");
      }

      function nodeIdForVisibleLabel(rows, label, mode) {
        const normalized = String(label || "").trim();
        if (!normalized || normalized === "All portfolio") {
          return "portfolio";
        }

        const tradeMatches = rows.filter((row) => row.trade_id === normalized);
        if (tradeMatches.length === 1) {
          return tradeNodeId(tradeMatches[0]);
        }

        if (mode === "book") {
          const bookMatches = Array.from(
            new Set(rows.filter((row) => bookLabel(row.book) === normalized).map((row) => bookNodeId(row)))
          );
          if (bookMatches.length === 1) {
            return bookMatches[0];
          }
          const bookAssetMatches = Array.from(
            new Set(rows.filter((row) => row.asset_class === normalized).map((row) => bookAssetNodeId(row)))
          );
          if (bookAssetMatches.length === 1) {
            return bookAssetMatches[0];
          }
        }

        const assetMatches = Array.from(
          new Set(rows.filter((row) => row.asset_class === normalized).map((row) => row.asset_class))
        );
        if (assetMatches.length === 1) {
          return `asset:${assetMatches[0]}`;
        }

        const productMatches = Array.from(
          new Set(
            rows
              .filter((row) => row.product_type === normalized)
              .map((row) => `product:${row.asset_class}:${row.product_type}`)
          )
        );
        return productMatches.length === 1 ? productMatches[0] : "";
      }

      function sliceLabel(slice) {
        if (!slice) {
          return "";
        }
        const textNode = slice.querySelector("text");
        if (!textNode) {
          return "";
        }
        const tspans = Array.from(textNode.querySelectorAll("tspan"))
          .map((item) => item.textContent.trim())
          .filter(Boolean);
        if (tspans.length > 0) {
          return tspans[0];
        }
        return textNode.textContent.trim().replace(/[\\s$,.%\\d-]+$/u, "").trim();
      }

      function closestSliceElement(target, boundary) {
        let current = target;
        while (current && current !== boundary) {
          const className = String(current.getAttribute && current.getAttribute("class") || "");
          if (current.tagName && current.tagName.toLowerCase() === "g" && className.includes("slice")) {
            return current;
          }
          current = current.parentNode;
        }
        return null;
      }

      function currentTreeLevel(plot) {
        const trace = plot.data && plot.data[0];
        let level = trace && trace.level;
        if (Array.isArray(level)) {
          level = level[0];
        }
        return level ? String(level) : "portfolio";
      }

      function restyleTouchesLevel(eventData) {
        const update = Array.isArray(eventData) ? eventData[0] : eventData;
        return !!update && Object.prototype.hasOwnProperty.call(update, "level");
      }

      function setFilterLabel(plot, label, rows) {
        const panel = plot.closest(".panel");
        const filter = panel ? panel.querySelector("[data-panel-filter]") : null;
        if (!filter) {
          return;
        }
        filter.hidden = false;
        filter.textContent = `${label} - ${rows.length} position${rows.length === 1 ? "" : "s"} - ${money(sumExposure(rows))} exposure`;
      }

      function updateLinkedCharts(plot, rows, label, nodeId, options) {
        const settings = options || {};
        const mode = settings.mode || "asset";
        const sortedRows = sortRows(rows, mode);
        const tradeAxis = tradeAxisValues(sortedRows, mode);
        const tradeData = tradeCustomData(sortedRows);
        const npvs = sortedRows.map(tradeMetricValue);
        const donut = buildDonut(sortedRows, nodeId, mode);
        const treemap = buildTreemap(sortedRows, mode, label);
        const npvRange = paddedNumberRange(npvs, 1);

        const updates = [
          Plotly.restyle(plot, {
            "ids": [treemap.ids],
            "labels": [treemap.labels],
            "marker.colors": [treemap.colors],
            "parents": [treemap.parents],
            "textfont.color": plotLabelTextColor(),
            "values": [treemap.values]
          }, [0]),
          Plotly.restyle(plot, {
            "customdata": [donut.ids],
            "ids": [donut.ids],
            "labels": [donut.labels],
            "marker.colors": [donut.colors],
            "textfont.color": plotLabelTextColor(),
            "values": [donut.values]
          }, [1]),
          Plotly.restyle(plot, {
            "customdata": [tradeData],
            "marker.color": [sortedRows.map(tradeMetricColor)],
            "text": [npvs.map(money)],
            "x": [npvs],
            "y": [tradeAxis]
          }, [2])
        ];

        if (settings.updateTreeLevel === false) {
          updates.shift();
        }

        updates.push(Plotly.relayout(plot, {
          "annotations[1].text": boldChartTitle(`Allocation - ${compactLabel(label)}`),
          "annotations[2].text": boldChartTitle(`NPV - ${compactLabel(label)}`),
          "xaxis.range": npvRange,
          "yaxis.autorange": "reversed",
          "yaxis.type": Array.isArray(tradeAxis[0]) ? "multicategory" : "category"
        }));

        setFilterLabel(plot, label, rows);
        return Promise.all(updates);
      }

      function initializePortfolioAllocation(plot) {
        if (!plot || !plot.layout || !plot.layout.meta || plot.layout.meta.qrpPanel !== "portfolio_allocation") {
          return;
        }
        if (plot.dataset.qrpLinked === "true") {
          if (typeof plot.qrpAttachSliceFallbacks === "function") {
            plot.qrpAttachSliceFallbacks();
          }
          return;
        }
        plot.dataset.qrpLinked = "true";

        const rows = (plot.layout.meta.tradeRows || []).slice();
        let activeRows = rows.slice();
        let activeMode = "asset";
        let syncingTreeLevel = false;
        const panel = plot.closest(".panel");
        const resetButton = panel ? panel.querySelector("[data-panel-reset]") : null;
        const modeButtons = panel ? Array.from(panel.querySelectorAll("[data-allocation-view]")) : [];
        if (panel) {
          panel.classList.add("is-linked");
        }

        function setActiveMode(mode) {
          activeMode = ["asset", "book", "position"].includes(mode) ? mode : "asset";
          modeButtons.forEach((button) => {
            button.setAttribute("aria-pressed", button.dataset.allocationView === activeMode ? "true" : "false");
          });
        }

        function applySelection(selectedRows, label, nodeId, options) {
          const settings = options || {};
          settings.mode = settings.mode || activeMode;
          const nextRows = selectedRows.length > 0 ? selectedRows.slice() : rows.slice();
          const nextLabel = selectedRows.length > 0 ? label : "All portfolio";
          const nextNodeId = selectedRows.length > 0 ? nodeId : "portfolio";
          activeRows = nextRows;
          if (settings.updateTreeLevel !== false) {
            syncingTreeLevel = true;
          }
          const updatePromise = updateLinkedCharts(plot, nextRows, nextLabel, nextNodeId, settings);
          Promise.resolve(updatePromise).finally(() => {
            attachSliceFallbacks();
            window.setTimeout(() => {
              syncingTreeLevel = false;
            }, 0);
          });
          return updatePromise;
        }

        function reset() {
          return applySelection(rows, "All portfolio", "portfolio");
        }

        function attachSliceFallbacks() {
          function bindSliceEvents(layer, handler) {
            ["pointerup", "mouseup", "click"].forEach((eventName) => {
              layer.addEventListener(eventName, handler, true);
            });
          }

          const treemapLayer = plot.querySelector(".treemaplayer");
          if (treemapLayer && treemapLayer.dataset.qrpClickFallback !== "true") {
            treemapLayer.dataset.qrpClickFallback = "true";
            bindSliceEvents(treemapLayer, (event) => {
              const slice = closestSliceElement(event.target, treemapLayer);
              const label = sliceLabel(slice);
              const id = nodeIdForVisibleLabel(rows, label, activeMode);
              if (!id) {
                return;
              }
              event.preventDefault();
              event.stopPropagation();
              if (event.stopImmediatePropagation) {
                event.stopImmediatePropagation();
              }
              const selectedRows = id === "portfolio" ? rows : rowsForNode(rows, id);
              window.setTimeout(() => {
                applySelection(selectedRows, labelForNode(id, label), id);
              }, 0);
            });
          }

          const pieLayer = plot.querySelector(".pielayer");
          if (pieLayer && pieLayer.dataset.qrpClickFallback !== "true") {
            pieLayer.dataset.qrpClickFallback = "true";
            bindSliceEvents(pieLayer, (event) => {
              const slice = closestSliceElement(event.target, pieLayer);
              const label = sliceLabel(slice);
              const id = nodeIdForVisibleLabel(activeRows, label, activeMode);
              if (!id) {
                return;
              }
              event.preventDefault();
              event.stopPropagation();
              if (event.stopImmediatePropagation) {
                event.stopImmediatePropagation();
              }
              const selectedRows = id === "portfolio" ? rows : rowsForNode(rows, id);
              window.setTimeout(() => {
                applySelection(selectedRows, labelForNode(id, label), id);
              }, 0);
            });
          }
        }
        plot.qrpAttachSliceFallbacks = attachSliceFallbacks;

        plot.on("plotly_click", (eventData) => {
          const point = eventData.points && eventData.points[0];
          if (!point) {
            return;
          }

          if (point.curveNumber === 0) {
            const id = pointNodeId(plot, point);
            const selectedRows = id === "portfolio" ? rows : rowsForNode(rows, id);
            window.setTimeout(() => {
              applySelection(selectedRows, labelForNode(id, point.label), id);
            }, 0);
            return;
          }

          if (point.curveNumber === 1) {
            const id = pointNodeId(plot, point);
            const selectedRows = id === "portfolio" ? rows : rowsForNode(rows, id);
            window.setTimeout(() => {
              applySelection(selectedRows, labelForNode(id, point.label), id);
            }, 0);
            return;
          }

          if (point.curveNumber === 2) {
            const tradeId = tradeIdFromPoint(point);
            const selectedRows = activeRows.filter((row) => row.trade_id === tradeId);
            window.setTimeout(() => {
              applySelection(selectedRows, `Position: ${tradeId}`, selectedRows[0] ? tradeNodeId(selectedRows[0]) : `trade:${tradeId}`);
            }, 0);
          }
        });

        plot.on("plotly_restyle", (eventData) => {
          if (syncingTreeLevel) {
            return;
          }
          if (!restyleTouchesLevel(eventData)) {
            return;
          }
          const id = currentTreeLevel(plot);
          const selectedRows = id === "portfolio" ? rows : rowsForNode(rows, id);
          window.setTimeout(() => {
            applySelection(selectedRows, labelForNode(id), id, { updateTreeLevel: false });
          }, 0);
        });

        plot.on("plotly_doubleclick", () => {
          reset();
          return false;
        });

        if (resetButton) {
          resetButton.addEventListener("click", reset);
        }
        modeButtons.forEach((button) => {
          button.addEventListener("click", () => {
            setActiveMode(button.dataset.allocationView);
            reset();
          });
        });
        setActiveMode("asset");
        setFilterLabel(plot, "All portfolio", rows);
        reset();
        window.setTimeout(attachSliceFallbacks, 0);
      }

      function markLoaded(element) {
        if (!element) {
          return;
        }
        element.classList.remove("is-loading");
        element.classList.add("is-loaded");
      }

      function initializeLoadingStates() {
        document.querySelectorAll(".card.is-loading").forEach((card, index) => {
          window.setTimeout(() => markLoaded(card), 120 + index * 45);
        });
        document.querySelectorAll(".panel.is-loading").forEach((panel) => {
          const plot = panel.querySelector(".js-plotly-plot");
          if (!plot) {
            window.setTimeout(() => markLoaded(panel), 120);
            return;
          }
          const complete = () => markLoaded(panel);
          if (typeof plot.on === "function") {
            plot.on("plotly_afterplot", complete);
          }
          if (plot.querySelector(".main-svg")) {
            window.setTimeout(complete, 160);
          }
          window.setTimeout(complete, 2500);
        });
      }

      function initializeLinkedPanels() {
        Array.from(document.querySelectorAll(".js-plotly-plot")).forEach(initializePortfolioAllocation);
      }

      if (document.readyState === "loading") {
        document.addEventListener("DOMContentLoaded", () => {
          initializeLinkedPanels();
          initializeLoadingStates();
          initializeSortableTables();
        });
      } else {
        initializeLinkedPanels();
        initializeLoadingStates();
        initializeSortableTables();
      }
      window.addEventListener("load", initializeLinkedPanels);
      window.addEventListener("load", initializeLoadingStates);
      window.addEventListener("load", initializeSortableTables);
      [250, 1000, 2500].forEach((delay) => {
        window.setTimeout(initializeLinkedPanels, delay);
      });
    }());
  </script>
"""
    document = f"""<!doctype html>
<html lang="en" data-theme="light">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>{html.escape(title)}</title>
{initial_theme_script}
  <style>
    @import url("https://fonts.googleapis.com/css2?family=IBM+Plex+Mono:wght@500;600&family=IBM+Plex+Sans:wght@400;500;600;700&display=swap");

    :root {{
      color-scheme: light;
      --accent: #5faea4;
      --accent-2: #7aa7d9;
      --amber: #d5ac63;
      --bg: #f6efe2;
      --bg-grid: rgba(24, 45, 37, 0.055);
      --card-glow: rgba(15, 118, 110, 0.12);
      --danger: #d3777c;
      --hero: #102820;
      --ink: #16231d;
      --line: rgba(31, 53, 45, 0.17);
      --muted: #647269;
      --panel: rgba(255, 250, 239, 0.84);
      --panel-strong: rgba(255, 253, 248, 0.94);
      --shadow: rgba(23, 31, 27, 0.13);
      --status-partial-bg: rgba(213, 172, 99, 0.18);
      --status-partial-fg: #85622d;
      --status-supported-bg: rgba(95, 174, 164, 0.14);
      --status-supported-fg: #5faea4;
      --status-unsupported-bg: rgba(211, 119, 124, 0.16);
      --status-unsupported-fg: #d3777c;
      --table-border: rgba(16, 40, 32, 0.12);
      --table-cell: #fff8eb;
      --table-cell-font: #16231d;
      --table-header: #102820;
      --table-header-font: #f4fff9;
      --table-muted: #647269;
      --table-row-even: #f0ebdf;
      --table-row-odd: #fff8eb;
      --toggle-bg: rgba(255, 250, 239, 0.72);
    }}
    :root[data-theme="dark"] {{
      color-scheme: dark;
      --accent: #83d5cc;
      --accent-2: #9dbfed;
      --amber: #e2c174;
      --bg: #061016;
      --bg-grid: rgba(123, 169, 151, 0.07);
      --card-glow: rgba(100, 214, 194, 0.16);
      --danger: #e79296;
      --hero: #e7f4ef;
      --ink: #e7f4ef;
      --line: rgba(136, 164, 151, 0.22);
      --muted: #91a69e;
      --panel: rgba(9, 22, 30, 0.82);
      --panel-strong: rgba(12, 29, 39, 0.92);
      --shadow: rgba(0, 0, 0, 0.42);
      --status-partial-bg: rgba(226, 193, 116, 0.22);
      --status-partial-fg: #f1d997;
      --status-supported-bg: rgba(131, 213, 204, 0.18);
      --status-supported-fg: #a8eee6;
      --status-unsupported-bg: rgba(231, 146, 150, 0.20);
      --status-unsupported-fg: #f2b4b8;
      --table-border: rgba(136, 164, 151, 0.22);
      --table-cell: #0f1d26;
      --table-cell-font: #e7f4ef;
      --table-header: #83d5cc;
      --table-header-font: #061016;
      --table-muted: #91a69e;
      --table-row-even: #142832;
      --table-row-odd: #0f1d26;
      --toggle-bg: rgba(12, 29, 39, 0.74);
    }}
    * {{
      box-sizing: border-box;
    }}
    body {{
      background:
        radial-gradient(circle at 12% 4%, color-mix(in srgb, var(--accent) 22%, transparent), transparent 28rem),
        radial-gradient(circle at 84% 12%, color-mix(in srgb, var(--accent-2) 18%, transparent), transparent 32rem),
        linear-gradient(135deg, var(--bg) 0%, color-mix(in srgb, var(--bg) 88%, var(--accent) 12%) 100%);
      color: var(--ink);
      font-family: "IBM Plex Sans", "Aptos", "Bahnschrift", sans-serif;
      margin: 0;
      min-height: 100vh;
      padding: 0;
      transition: background 260ms ease, color 220ms ease;
    }}
    body::before {{
      background:
        linear-gradient(var(--bg-grid) 1px, transparent 1px),
        linear-gradient(90deg, var(--bg-grid) 1px, transparent 1px);
      background-size: 42px 42px;
      content: "";
      inset: 0;
      mask-image: linear-gradient(to bottom, black, transparent 76%);
      pointer-events: none;
      position: fixed;
      z-index: -1;
    }}
    .dashboard-shell {{
      margin: 0 auto;
      max-width: 1480px;
      padding: clamp(18px, 3vw, 36px);
    }}
    .hero {{
      border-bottom: 1px solid var(--line);
      margin-bottom: 24px;
      padding: 10px 0 22px;
      position: relative;
    }}
    .topbar {{
      align-items: center;
      display: flex;
      gap: 16px;
      justify-content: space-between;
      margin-bottom: 34px;
    }}
    .brand {{
      align-items: center;
      display: flex;
      gap: 12px;
      min-width: 0;
    }}
    .brand-mark {{
      align-items: center;
      background:
        linear-gradient(135deg, var(--hero), color-mix(in srgb, var(--accent) 62%, var(--hero) 38%));
      border: 1px solid color-mix(in srgb, var(--accent) 42%, transparent);
      border-radius: 14px;
      box-shadow: 0 14px 34px var(--card-glow);
      color: var(--bg);
      display: inline-flex;
      font-family: "IBM Plex Mono", "Cascadia Mono", monospace;
      font-size: 0.92rem;
      font-weight: 700;
      height: 42px;
      justify-content: center;
      letter-spacing: 0.08em;
      width: 56px;
    }}
    :root[data-theme="dark"] .brand-mark {{
      color: #061016;
    }}
    .brand-copy {{
      min-width: 0;
    }}
    .eyebrow {{
      color: var(--accent);
      font-size: 0.78rem;
      font-weight: 700;
      letter-spacing: 0.16em;
      text-transform: uppercase;
    }}
    .desk-label {{
      color: var(--muted);
      font-size: 0.92rem;
      margin-top: 2px;
    }}
    .theme-toggle {{
      align-items: center;
      backdrop-filter: blur(18px);
      background: var(--toggle-bg);
      border: 1px solid var(--line);
      border-radius: 999px;
      box-shadow: 0 16px 36px var(--shadow);
      color: var(--ink);
      cursor: pointer;
      display: inline-flex;
      font: 700 0.88rem "IBM Plex Sans", "Aptos", sans-serif;
      gap: 9px;
      min-height: 42px;
      padding: 9px 14px;
      transition: transform 180ms ease, border-color 180ms ease, background 180ms ease;
      white-space: nowrap;
    }}
    .theme-toggle:hover {{
      border-color: color-mix(in srgb, var(--accent) 55%, var(--line));
      transform: translateY(-1px);
    }}
    .dashboard-controls {{
      align-items: center;
      display: flex;
      flex-wrap: wrap;
      gap: 10px;
      justify-content: flex-end;
    }}
    .style-picker {{
      align-items: center;
      backdrop-filter: blur(18px);
      background: var(--toggle-bg);
      border: 1px solid var(--line);
      border-radius: 999px;
      box-shadow: 0 16px 36px var(--shadow);
      color: var(--muted);
      display: inline-flex;
      font: 700 0.78rem "IBM Plex Sans", "Aptos", sans-serif;
      gap: 8px;
      letter-spacing: 0.08em;
      min-height: 42px;
      padding: 7px 8px 7px 13px;
      text-transform: uppercase;
    }}
    .style-picker select {{
      appearance: none;
      background:
        linear-gradient(135deg, color-mix(in srgb, var(--accent) 16%, transparent), transparent),
        var(--panel);
      border: 1px solid color-mix(in srgb, var(--accent) 34%, var(--line));
      border-radius: 999px;
      color: var(--ink);
      cursor: pointer;
      font: 700 0.86rem "IBM Plex Sans", "Aptos", sans-serif;
      min-width: 138px;
      outline: none;
      padding: 7px 30px 7px 12px;
    }}
    .portfolio-picker select {{
      min-width: 240px;
    }}
    .style-picker::after {{
      border-left: 4px solid transparent;
      border-right: 4px solid transparent;
      border-top: 5px solid var(--accent);
      content: "";
      height: 0;
      margin-left: -32px;
      pointer-events: none;
      width: 0;
    }}
    .theme-icon {{
      align-items: center;
      background: color-mix(in srgb, var(--accent) 13%, transparent);
      border: 1px solid color-mix(in srgb, var(--accent) 30%, transparent);
      border-radius: 999px;
      color: var(--accent);
      display: inline-flex;
      font-size: 1rem;
      height: 24px;
      justify-content: center;
      line-height: 1;
      width: 24px;
    }}
    .theme-icon-moon {{
      display: none;
    }}
    :root[data-theme="dark"] .theme-icon-sun {{
      display: none;
    }}
    :root[data-theme="dark"] .theme-icon-moon {{
      display: inline-flex;
    }}
    h1 {{
      color: var(--hero);
      font-size: clamp(2.25rem, 5.8vw, 6.2rem);
      letter-spacing: -0.065em;
      line-height: 0.88;
      margin: 0;
      max-width: 1020px;
    }}
    h2 {{
      color: var(--accent);
      font-size: 0.92rem;
      font-weight: 800;
      letter-spacing: 0.13em;
      margin: 0 0 16px;
      text-transform: uppercase;
    }}
    .subtitle {{
      color: var(--muted);
      font-size: clamp(1rem, 1.8vw, 1.22rem);
      line-height: 1.6;
      max-width: 980px;
    }}
    .market-ribbon {{
      align-items: center;
      color: var(--muted);
      display: flex;
      flex-wrap: wrap;
      font-family: "IBM Plex Mono", "Cascadia Mono", monospace;
      font-size: 0.78rem;
      gap: 10px;
      letter-spacing: 0.08em;
      margin-top: 22px;
      text-transform: uppercase;
    }}
    .market-ribbon span {{
      border: 1px solid var(--line);
      border-radius: 999px;
      background: var(--panel);
      padding: 6px 10px;
    }}
    .cards {{
      display: grid;
      gap: 14px;
      grid-template-columns: repeat(6, minmax(0, 1fr));
      margin: 28px 0;
    }}
    .portfolio-view[hidden] {{
      display: none;
    }}
    .view-heading {{
      align-items: flex-end;
      border-bottom: 1px solid var(--line);
      display: flex;
      gap: 18px;
      justify-content: space-between;
      margin: 28px 0 6px;
      padding: 0 2px 16px;
    }}
    .view-heading h2 {{
      margin-bottom: 8px;
    }}
    .view-heading p {{
      color: var(--muted);
      line-height: 1.55;
      margin: 0;
      max-width: 860px;
    }}
    .view-heading span {{
      color: var(--accent);
      font-family: "IBM Plex Mono", "Cascadia Mono", monospace;
      font-size: 0.82rem;
      font-weight: 700;
      white-space: nowrap;
    }}
    .card, .panel {{
      backdrop-filter: blur(18px);
      background: var(--panel);
      border: 1px solid var(--line);
      border-radius: 24px;
      box-shadow: 0 22px 64px var(--shadow);
      position: relative;
      transition: background 220ms ease, border-color 220ms ease, box-shadow 220ms ease;
    }}
    .card::before, .panel::before {{
      background: linear-gradient(135deg, color-mix(in srgb, var(--accent) 26%, transparent), transparent 42%);
      border-radius: inherit;
      content: "";
      inset: 0;
      opacity: 0.32;
      pointer-events: none;
      position: absolute;
    }}
    .card {{
      overflow: hidden;
      min-width: 0;
      padding: 18px 16px;
    }}
    .card-label {{
      color: var(--muted);
      font-size: 0.76rem;
      font-weight: 700;
      letter-spacing: 0.12em;
      text-transform: uppercase;
    }}
    .card-value {{
      color: var(--ink);
      font-family: "IBM Plex Mono", "Cascadia Mono", monospace;
      font-size: 1.55rem;
      font-weight: 700;
      margin-top: 8px;
      overflow-wrap: anywhere;
    }}
    .card-note {{
      color: var(--muted);
      font-size: 0.84rem;
      line-height: 1.35;
      margin-top: 6px;
    }}
    .panel {{
      background: var(--panel-strong);
      margin: 24px 0;
      overflow: hidden;
      padding: clamp(16px, 2vw, 22px);
    }}
    .panel-heading {{
      align-items: center;
      display: flex;
      gap: 14px;
      justify-content: space-between;
      position: relative;
      z-index: 1;
    }}
    .panel-actions {{
      align-items: center;
      display: flex;
      flex-wrap: wrap;
      gap: 10px;
      justify-content: flex-end;
    }}
    .allocation-mode {{
      background: color-mix(in srgb, var(--accent) 9%, transparent);
      border: 1px solid color-mix(in srgb, var(--accent) 26%, var(--line));
      border-radius: 999px;
      display: none;
      gap: 2px;
      padding: 3px;
    }}
    .panel.is-linked .allocation-mode {{
      display: inline-flex;
    }}
    .allocation-mode button {{
      background: transparent;
      border: 0;
      border-radius: 999px;
      color: var(--muted);
      cursor: pointer;
      font: 700 0.74rem "IBM Plex Sans", "Aptos", sans-serif;
      letter-spacing: 0.06em;
      padding: 6px 9px;
      text-transform: uppercase;
    }}
    .allocation-mode button[aria-pressed="true"] {{
      background: color-mix(in srgb, var(--accent) 23%, transparent);
      color: var(--accent);
    }}
    .panel-reset {{
      background: color-mix(in srgb, var(--accent) 12%, transparent);
      border: 1px solid color-mix(in srgb, var(--accent) 32%, var(--line));
      border-radius: 999px;
      color: var(--accent);
      cursor: pointer;
      display: none;
      font: 700 0.78rem "IBM Plex Sans", "Aptos", sans-serif;
      letter-spacing: 0.08em;
      padding: 7px 11px;
      text-transform: uppercase;
    }}
    .panel.is-linked .panel-reset {{
      display: inline-flex;
    }}
    .panel-filter {{
      background: color-mix(in srgb, var(--accent) 9%, transparent);
      border: 1px solid color-mix(in srgb, var(--accent) 26%, var(--line));
      border-radius: 999px;
      color: var(--muted);
      display: inline-flex;
      font-family: "IBM Plex Mono", "Cascadia Mono", monospace;
      font-size: 0.78rem;
      letter-spacing: 0.03em;
      margin: -4px 0 10px;
      padding: 7px 11px;
      position: relative;
      z-index: 1;
    }}
    .panel-filter[hidden] {{
      display: none;
    }}
    .panel-note {{
      color: var(--muted);
      font-size: 0.9rem;
      line-height: 1.45;
      margin: -4px 0 14px;
      max-width: 980px;
      position: relative;
      z-index: 1;
    }}
    .panel .js-plotly-plot {{
      border-radius: 18px;
    }}
    .panel-body {{
      min-height: 160px;
      position: relative;
      z-index: 1;
    }}
    .panel-loading {{
      align-items: center;
      background: linear-gradient(90deg, transparent, color-mix(in srgb, var(--accent) 13%, transparent), transparent);
      border: 1px solid color-mix(in srgb, var(--accent) 18%, var(--line));
      border-radius: 18px;
      display: none;
      inset: 8px;
      justify-content: center;
      pointer-events: none;
      position: absolute;
      z-index: 5;
    }}
    .panel.is-loading .panel-loading {{
      display: flex;
    }}
    .panel-loading span {{
      animation: qrpSpin 900ms linear infinite;
      border: 3px solid color-mix(in srgb, var(--accent) 20%, transparent);
      border-radius: 50%;
      border-top-color: var(--accent);
      height: 28px;
      width: 28px;
    }}
    .card.is-loading::after {{
      background: linear-gradient(90deg, transparent, color-mix(in srgb, var(--accent) 18%, transparent), transparent);
      content: "";
      inset: 0;
      opacity: 0.8;
      pointer-events: none;
      position: absolute;
      transform: translateX(-100%);
    }}
    .card.is-loading::after {{
      animation: qrpSweep 780ms ease-out forwards;
    }}
    @keyframes qrpSpin {{
      to {{
        transform: rotate(360deg);
      }}
    }}
    @keyframes qrpSweep {{
      to {{
        transform: translateX(100%);
      }}
    }}
    .data-table-wrap {{
      max-height: var(--table-max-height, 620px);
      overflow-x: hidden;
      overflow-y: auto;
      position: relative;
      scrollbar-color: var(--table-muted) transparent;
      z-index: 1;
    }}
    .data-table {{
      border-collapse: collapse;
      font-size: 0.86rem;
      table-layout: fixed;
      width: 100%;
    }}
    .data-table th {{
      background: var(--table-header);
      color: var(--table-header-font);
      position: sticky;
      top: 0;
      z-index: 2;
    }}
    .data-table th button {{
      align-items: center;
      background: transparent;
      border: 0;
      color: inherit;
      cursor: pointer;
      display: inline-flex;
      font: 700 0.82rem "IBM Plex Sans", "Aptos", sans-serif;
      gap: 7px;
      justify-content: space-between;
      min-width: 0;
      padding: 10px 9px;
      text-align: left;
      width: 100%;
    }}
    .data-table th button span:first-child {{
      min-width: 0;
      overflow-wrap: anywhere;
    }}
    .data-table td {{
      border: 1px solid var(--table-border);
      color: var(--table-cell-font);
      overflow-wrap: anywhere;
      padding: 9px;
      white-space: normal;
      word-break: normal;
    }}
    .data-table td[data-kind="number"] {{
      white-space: nowrap;
    }}
    .data-table[data-table-id="support-diagnostics"] th:nth-child(1),
    .data-table[data-table-id="support-diagnostics"] td:nth-child(1) {{
      width: 14%;
    }}
    .data-table[data-table-id="support-diagnostics"] th:nth-child(2),
    .data-table[data-table-id="support-diagnostics"] td:nth-child(2) {{
      width: 8%;
    }}
    .data-table[data-table-id="support-diagnostics"] th:nth-child(3),
    .data-table[data-table-id="support-diagnostics"] td:nth-child(3) {{
      width: 13%;
    }}
    .data-table[data-table-id="support-diagnostics"] th:nth-child(4),
    .data-table[data-table-id="support-diagnostics"] td:nth-child(4) {{
      width: 24%;
    }}
    .data-table[data-table-id="support-diagnostics"] th:nth-child(5),
    .data-table[data-table-id="support-diagnostics"] td:nth-child(5) {{
      width: 13%;
    }}
    .data-table[data-table-id="support-diagnostics"] th:nth-child(6),
    .data-table[data-table-id="support-diagnostics"] td:nth-child(6) {{
      width: 28%;
    }}
    .data-table[data-table-id="trade-inventory"] th:nth-child(1),
    .data-table[data-table-id="trade-inventory"] td:nth-child(1) {{
      width: 17%;
    }}
    .data-table[data-table-id="trade-inventory"] th:nth-child(2),
    .data-table[data-table-id="trade-inventory"] td:nth-child(2) {{
      width: 8%;
    }}
    .data-table[data-table-id="trade-inventory"] th:nth-child(3),
    .data-table[data-table-id="trade-inventory"] td:nth-child(3) {{
      width: 16%;
    }}
    .data-table[data-table-id="trade-inventory"] th:nth-child(4),
    .data-table[data-table-id="trade-inventory"] td:nth-child(4) {{
      width: 18%;
    }}
    .data-table[data-table-id="trade-inventory"] th:nth-child(5),
    .data-table[data-table-id="trade-inventory"] td:nth-child(5) {{
      width: 13%;
    }}
    .data-table[data-table-id="trade-inventory"] th:nth-child(6),
    .data-table[data-table-id="trade-inventory"] td:nth-child(6),
    .data-table[data-table-id="trade-inventory"] th:nth-child(7),
    .data-table[data-table-id="trade-inventory"] td:nth-child(7) {{
      width: 8%;
    }}
    .data-table[data-table-id="trade-inventory"] th:nth-child(8),
    .data-table[data-table-id="trade-inventory"] td:nth-child(8) {{
      width: 12%;
    }}
    .data-table tbody tr:nth-child(odd) {{
      background: var(--table-row-odd);
    }}
    .data-table tbody tr:nth-child(even) {{
      background: var(--table-row-even);
    }}
    .sort-arrow::after {{
      color: var(--accent);
      content: "\\2195";
      font-size: 0.78rem;
    }}
    .data-table th button[aria-sort="ascending"] .sort-arrow::after {{
      content: "\\2191";
    }}
    .data-table th button[aria-sort="descending"] .sort-arrow::after {{
      content: "\\2193";
    }}
    .status-pill {{
      align-items: center;
      border-radius: 999px;
      display: inline-flex;
      font-size: 0.78rem;
      font-weight: 700;
      justify-content: center;
      line-height: 1.15;
      padding: 3px 8px;
      white-space: normal;
    }}
    .status-pill-supported {{
      background: var(--status-supported-bg);
      color: var(--status-supported-fg);
    }}
    .status-pill-partially-supported {{
      background: var(--status-partial-bg);
      color: var(--status-partial-fg);
    }}
    .status-pill-unsupported, .status-pill-failed {{
      background: var(--status-unsupported-bg);
      color: var(--status-unsupported-fg);
    }}
    .footnote {{
      color: var(--muted);
      font-size: 0.9rem;
      line-height: 1.45;
      margin-top: 28px;
    }}
    @media (max-width: 1320px) {{
      .cards {{
        grid-template-columns: repeat(3, minmax(0, 1fr));
      }}
    }}
    @media (max-width: 860px) {{
      .cards {{
        grid-template-columns: repeat(2, minmax(0, 1fr));
      }}
    }}
    @media (max-width: 720px) {{
      .topbar {{
        align-items: flex-start;
        flex-direction: column;
      }}
      .theme-toggle {{
        align-self: flex-end;
      }}
      .view-heading {{
        align-items: flex-start;
        flex-direction: column;
      }}
      .view-heading span {{
        white-space: normal;
      }}
      .dashboard-shell {{
        padding: 16px;
      }}
      .card-value {{
        font-size: 1.42rem;
      }}
    }}
    @media (max-width: 520px) {{
      .cards {{
        grid-template-columns: 1fr;
      }}
    }}
  </style>
</head>
<body>
  <div class="dashboard-shell">
    <header class="hero">
      <div class="topbar">
        <div class="brand">
          <span class="brand-mark">QRP</span>
          <div class="brand-copy">
            <div class="eyebrow">Risk cockpit</div>
            <div class="desk-label">Portfolio manager and risk manager view</div>
          </div>
        </div>
        <div class="dashboard-controls">
          <label class="style-picker portfolio-picker">
            <span>Portfolio</span>
            <select data-portfolio-select aria-label="Dashboard portfolio">
              {portfolio_options}
            </select>
          </label>
          <label class="style-picker">
            <span>Style</span>
            <select data-style-select aria-label="Dashboard visual style">
              {style_options}
            </select>
          </label>
          <button class="theme-toggle" type="button" data-theme-toggle aria-label="Switch theme">
            <span class="theme-icon theme-icon-sun" aria-hidden="true">&#9728;</span>
            <span class="theme-icon theme-icon-moon" aria-hidden="true">&#9790;</span>
            <span data-theme-label>Light</span>
          </button>
        </div>
      </div>
      <h1>{html.escape(title)}</h1>
      <p class="subtitle">
        Interactive portfolio performance and risk dashboard generated from model return history,
        allocation, valuation, stress, factor exposure, P&amp;L explain, and Monte Carlo outputs.
        Built as a modern cockpit for performance review, loss diagnostics, and risk investigations.
      </p>
      <div class="market-ribbon">
        <span>Market as of {html.escape(market_as_of)}</span>
        <span>Generated {html.escape(generated_at)}</span>
        <span>Scenario revaluation</span>
        <span>VaR / ES</span>
        <span>Stress paths</span>
        <span>P&amp;L explain</span>
      </div>
    </header>
    <main>
      {view_html}
      <p class="footnote">
        Performance history is a deterministic model series generated from asset-class return assumptions and policy
        benchmark weights; the chart supports common lookback windows from 1Y through inception plus calendar-year
        returns. Stress scenarios are one-step revaluations as of {html.escape(market_as_of)}; they are not chronological
        portfolio performance.
      </p>
    </main>
  </div>
{theme_script}
{interaction_script}
</body>
</html>
"""
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
    npv_axis_labels = [
        f"{row['asset_class']} / {row['product_type']} / {row['trade_id']}"
        for row in trade_rows
    ]
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
            "l": categorical_left_margin(
                npv_axis_labels, min_margin=180, max_margin=300, char_width=4
            ),
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
    fig.update_xaxes(
        range=padded_number_range(npv_values), title_text="Display NPV", row=2, col=1
    )
    fig.update_yaxes(
        autorange="reversed", automargin=True, row=2, col=1, type="multicategory"
    )
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


def cell_display(row, key, kind):
    value = row.get(key, "")
    if key == "status":
        return (
            f'<span class="status-pill status-pill-{html.escape(str(value).replace("_", "-"))}">'
            f"{html.escape(support_status_label(value))}</span>"
        )
    if kind == "number":
        return html.escape(money(float(value or 0.0)))
    return html.escape(str(value or ""))


def sortable_table_html(table_id, rows, columns, *, max_height):
    header_html = "".join(
        f"""
        <th>
          <button type="button" data-sort-column="{index}" data-sort-kind="{html.escape(kind)}" aria-sort="none">
            <span>{html.escape(label)}</span><span class="sort-arrow" aria-hidden="true"></span>
          </button>
        </th>
        """
        for index, (_, label, kind) in enumerate(columns)
    )
    body_html = "\n".join(
        f"""
        <tr data-default-sort="{
            html.escape("|".join(str(value) for value in trade_sort_key(row)))
        }">
          {
            "".join(
                f'<td data-kind="{html.escape(kind)}" data-sort-value="{html.escape(sortable_value(row.get(key), kind))}">{cell_display(row, key, kind)}</td>'
                for key, _, kind in columns
            )
        }
        </tr>
        """
        for row in rows
    )
    if not body_html:
        body_html = f'<tr><td colspan="{len(columns)}">No rows</td></tr>'

    return f"""
    <div class="data-table-wrap" style="--table-max-height: {int(max_height)}px">
      <table class="data-table" data-sortable-table data-table-id="{html.escape(table_id)}">
        <thead><tr>{header_html}</tr></thead>
        <tbody>{body_html}</tbody>
      </table>
    </div>
    """


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
    sorted_results = sorted(
        risk_results, key=lambda result: trade_id_sort_key(result.trade_id, trade_rows)
    )
    trade_ids = [result.trade_id for result in sorted_results]
    trade_axis = trade_hierarchy_axis(trade_ids, trade_rows)
    trade_labels = trade_hierarchy_labels(trade_ids, trade_rows)
    all_buckets = sorted(
        {bucket for result in sorted_results for bucket in result.bucketed_risk},
        key=risk_bucket_sort_key,
    )
    bucket_axis = risk_bucket_hierarchy_axis(all_buckets)
    z = [
        [result.bucketed_risk.get(bucket, 0.0) for bucket in all_buckets]
        for result in sorted_results
    ]
    bucket_customdata = [
        [[result.trade_id, bucket] for bucket in all_buckets]
        for result in sorted_results
    ]
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
            "l": categorical_left_margin(
                trade_labels, min_margin=180, max_margin=320, char_width=4
            ),
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
    fig.update_yaxes(
        autorange="reversed", automargin=True, row=1, col=1, type="multicategory"
    )
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
    matrix_results = sorted(
        stress_results, key=lambda result: scenario_sort_key(result.scenario_name)
    )
    matrix_scenario_names = [result.scenario_name for result in matrix_results]
    matrix_scenario_axis = scenario_hierarchy_axis(matrix_scenario_names)
    trade_ids = sorted(
        {trade_id for result in stress_results for trade_id in result.trade_pnls},
        key=lambda trade_id: trade_id_sort_key(trade_id, trade_rows),
    )
    trade_axis = trade_hierarchy_axis(trade_ids, trade_rows)
    trade_labels = trade_hierarchy_labels(trade_ids, trade_rows)
    stress_matrix = [
        [result.trade_pnls.get(trade_id, 0.0) for result in matrix_results]
        for trade_id in trade_ids
    ]
    stress_customdata = [
        [[trade_id, result.scenario_name] for result in matrix_results]
        for trade_id in trade_ids
    ]
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
            marker_color=[
                NEGATIVE_COLOR if pnl < 0 else POSITIVE_COLOR for pnl in scenario_pnls
            ],
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
            marker_color=[
                NEGATIVE_COLOR if pnl < 0 else POSITIVE_COLOR
                for _, pnl in worst_trade_pnls
            ],
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
                categorical_left_margin(
                    trade_labels, min_margin=180, max_margin=320, char_width=4
                ),
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
        range=padded_number_range(
            [pnl for _, pnl in worst_trade_pnls] + [worst.total_pnl]
        ),
        title_text="Position Contribution",
        row=3,
        col=1,
    )
    fig.update_yaxes(autorange="reversed", automargin=True, row=1, col=1)
    fig.update_yaxes(
        autorange="reversed", automargin=True, row=2, col=1, type="multicategory"
    )
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
                POSITIVE_COLOR if value >= 0.0 else NEGATIVE_COLOR
                for value in performance_history["active_returns"]
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
        horizon_label = (
            format_date(end_date) if end_date else f"{result.horizon_days:g}d"
        )
        titles.append(f"{label} P&L Distribution to {horizon_label}")
    for label, result, _ in mc_results:
        end_date = horizon_date(market_as_of, result.horizon_days)
        horizon_label = (
            format_date(end_date) if end_date else f"{result.horizon_days:g}d"
        )
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
            go.Histogram(
                name=f"{label} P&L", x=pnls, nbinsx=35, marker_color=POSITIVE_COLOR
            ),
            row=1,
            col=col,
        )
        fig.add_vline(
            x=var_threshold, line_dash="dash", line_color=NEGATIVE_COLOR, row=1, col=col
        )
        fig.add_vline(
            x=es_threshold, line_dash="dot", line_color="#7c2d12", row=1, col=col
        )
        fig.add_vline(
            x=mean_pnl, line_dash="solid", line_color=WARNING_COLOR, row=1, col=col
        )

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
        fig.update_xaxes(
            title_text=f"Horizon P&L ending {horizon_label}", row=1, col=col
        )
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
    sorted_results = sorted(
        pnl_results, key=lambda result: trade_id_sort_key(result.trade_id, trade_rows)
    )
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
                explain_component_amount(
                    result, "carry", number_attr(result, "carry_pnl")
                )
                for result in sorted_results
            ],
            y=trade_axis,
            marker_color=WARNING_COLOR,
        )
    )
    fig.add_trace(
        go.Bar(
            name="Cash",
            orientation="h",
            x=[
                explain_component_amount(
                    result, "cash", number_attr(result, "cash_pnl")
                )
                for result in sorted_results
            ],
            y=trade_axis,
            marker_color=DASHBOARD_COLORWAY[1],
        )
    )
    fig.add_trace(
        go.Bar(
            name="Market Move",
            orientation="h",
            x=[
                explain_component_amount(
                    result, "market_move", number_attr(result, "market_move_pnl")
                )
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
                explain_component_amount(
                    result, "residual", explain_result_residual(result)
                )
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
            "l": categorical_left_margin(
                trade_labels, min_margin=180, max_margin=320, char_width=4
            ),
            "r": 38,
            "t": 82,
        }
    )
    fig.update_xaxes(title_text="P&L")
    fig.update_yaxes(autorange="reversed", automargin=True, type="multicategory")
    return fig


def make_factor_scenario_figure(go, make_subplots, factors, scenarios):
    factors_sorted = sorted(factors, key=factor_sort_key)
    scenarios_sorted = sorted(
        scenarios, key=lambda scenario: scenario_sort_key(scenario.name)
    )
    factor_ids = [factor.factor_id for factor in factors_sorted]
    factor_axis = factor_hierarchy_axis(factors_sorted)
    scenario_names = [scenario.name for scenario in scenarios_sorted]
    scenario_axis = scenario_hierarchy_axis(scenario_names)
    z = [
        [scenario.factor_shocks.get(factor_id, 0.0) for factor_id in factor_ids]
        for scenario in scenarios_sorted
    ]
    factor_customdata = [
        [[scenario.name, factor_id] for factor_id in factor_ids]
        for scenario in scenarios_sorted
    ]
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
    fig.update_yaxes(
        autorange="reversed", automargin=True, row=1, col=1, type="multicategory"
    )
    fig.update_xaxes(
        **diagonal_category_axis(title_text="Factor Type"),
        row=2,
        col=1,
    )
    fig.update_yaxes(title_text="Factors", row=2, col=1)
    return fig


def portfolio_label(portfolio):
    return getattr(portfolio, "portfolio_name", "") or getattr(
        portfolio, "portfolio_id", "Portfolio"
    )


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

    return [
        {
            "label": "Portfolio NPV",
            "value": money(total_npv),
            "note": f"{len(trade_rows)} positions across {len(set(row['asset_class'] for row in trade_rows))} asset classes",
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
            "value": f"{money(mc_card.var_95)} / {money(mc_card.expected_shortfall_95)}"
            if mc_card
            else "n/a",
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
            "Model total-return history versus the policy benchmark, with 1Y, 2Y, 5Y, 10Y, inception, and calendar-year views.",
        ),
        (
            "Portfolio Allocation",
            make_portfolio_allocation_figure(go, make_subplots, trade_rows),
            "Use Asset, Book, or Position hierarchy; funded bond and note bars show premium or discount to par while tables keep raw NPV.",
        ),
        (
            "Support Diagnostics",
            make_support_diagnostics_panel(trade_rows),
            "Pricing support exceptions are listed by position so unsupported or partially supported instruments are explicit.",
        ),
        (
            "Position Inventory",
            make_trade_inventory_figure(go, trade_rows),
            "Rows are portfolio positions; asset class and instrument type are shown separately for sorting and grouping.",
        ),
        (
            "Risk Exposures by Position",
            make_risk_measures_figure(go, make_subplots, risk_results, trade_rows),
            "Rows are positions, grouped by asset class and instrument type; rate and credit buckets are shown by tenor group.",
        ),
        (
            "Scenario Stress P&L",
            make_historical_stress_figure(
                go, make_subplots, stress_results, trade_rows
            ),
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
