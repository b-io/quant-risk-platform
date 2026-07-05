"""Creates the Plotly HTML dashboard used by the demo platform workflow."""

import html
import json
from collections import defaultdict
from datetime import date, datetime, timedelta


FINANCE_FONT = '"IBM Plex Sans", "Aptos", "Bahnschrift", sans-serif'
MONO_FONT = '"IBM Plex Mono", "Cascadia Mono", "SFMono-Regular", monospace'
CATEGORY_TICKANGLE = -35

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
    if float(row.get("npv", 0.0) or 0.0) < 0.0:
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


def trade_exposure_proxy(trade, valuation_result):
    notional = getattr(trade, "notional", None)
    if notional is not None and abs(float(notional)) > 0.0:
        return abs(float(notional))

    quantity = getattr(trade, "quantity", None)
    reference_price = getattr(trade, "reference_price", None)
    if quantity is not None and reference_price is not None:
        return abs(float(quantity) * float(reference_price))

    return abs(float(valuation_result.npv))


def build_trade_rows(portfolio, valuation_results):
    valuation_by_trade = {result.trade_id: result for result in valuation_results}
    rows = []
    for trade in portfolio.trades:
        valuation = valuation_by_trade[trade.id]
        rows.append(
            {
                "asset_class": valuation.tags.get("asset_class", trade.asset_class),
                "book": trade.book,
                "currency": valuation.currency,
                "exposure": trade_exposure_proxy(trade, valuation),
                "model": getattr(valuation, "model_name", "")
                or valuation.tags.get("model", ""),
                "npv": valuation.npv,
                "product_type": valuation.tags.get("product_type", trade.type),
                "status": valuation.tags.get("status", "unknown"),
                "status_message": getattr(valuation, "status_message", ""),
                "strategy": trade.strategy,
                "trade_id": trade.id,
            }
        )
    return sort_trade_rows(rows)


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


def stress_performance_path(total_npv, stress_results):
    scenario_names = ["Current"]
    equity_values = [total_npv]
    cumulative_pnl = 0.0
    for result in stress_results:
        scenario_names.append(result.scenario_name)
        cumulative_pnl += result.total_pnl
        equity_values.append(total_npv + cumulative_pnl)

    peaks = []
    drawdowns = []
    peak = equity_values[0]
    for value in equity_values:
        peak = max(peak, value)
        peaks.append(peak)
        denominator = abs(peak) if abs(peak) > 1e-12 else 1.0
        drawdowns.append((value - peak) / denominator)

    max_drawdown = min(drawdowns) if drawdowns else 0.0
    max_drawdown_amount = (
        min(value - peak for value, peak in zip(equity_values, peaks)) if peaks else 0.0
    )
    return scenario_names, equity_values, drawdowns, max_drawdown, max_drawdown_amount


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
            <section class="card">
              <div class="card-label">{html.escape(card["label"])}</div>
              <div class="card-value">{html.escape(card["value"])}</div>
              <div class="card-note">{html.escape(card["note"])}</div>
            </section>
            """
            for card in view["cards"]
        )
        figure_html = []
        for name, fig in view["figures"]:
            if isinstance(fig, dict) and fig.get("kind") == "html":
                panel_body = fig["html"]
            else:
                include_plotly = "cdn" if plotly_index == 0 else False
                plotly_index += 1
                panel_body = pio.to_html(
                    fig, full_html=False, include_plotlyjs=include_plotly
                )
            figure_html.append(f"""
            <section class="panel">
              <div class="panel-heading">
                <h2>{html.escape(name)}</h2>
                <button class="panel-reset" type="button" data-panel-reset>Reset slice</button>
              </div>
              <div class="panel-filter" data-panel-filter hidden></div>
              {panel_body}
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
          tableHeaderFont: "#f4fff9"
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
          tableHeaderFont: "#061016"
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

      function supportStatusColor(status, preset) {
        const normalized = String(status || "").toLowerCase();
        if (normalized.includes("partial")) return preset.amber;
        if (["error", "missing", "not_supported", "unsupported"].some((token) => normalized.includes(token))) {
          return preset.danger;
        }
        if (normalized.includes("supported")) return preset.accent;
        return preset.accent2 || (preset.colorway && preset.colorway[1]) || preset.accent;
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
        if (trace.meta && trace.meta.qrpStatus) {
          return (trace.x || []).map(() => supportStatusColor(trace.meta.qrpStatus, preset));
        }
        if (traceIndex === 2) {
          const rows = meta.tradeRows || [];
          const rowsById = new Map(rows.map((row) => [row.trade_id, row]));
          return traceTradeIds(trace).map((tradeId, pointIndex) => {
            const row = rowsById.get(tradeId);
            if (!row) {
              return preset.colorway[pointIndex % preset.colorway.length] || preset.accent;
            }
            return Number(row.npv || 0) < 0 ? preset.danger : nodeColor(tradeNodeIdFromRow(row), pointIndex, preset);
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
            const colors = trace.meta && trace.meta.qrpSupportDonut
              ? (trace.customdata || []).map((status) => supportStatusColor(status, preset))
              : (ids.length ? ids.map((id, pointIndex) => nodeColor(id, pointIndex, preset)) : preset.colorway);
            operations.push(Plotly.restyle(plot, {
              "marker.colors": [colors],
              "textfont.color": preset.fill_text || tokens.font
            }, [index]));
          } else if (trace.type === "treemap") {
            operations.push(Plotly.restyle(plot, {
              "marker.colors": [treemapColorsForTrace(trace, preset)],
              "textfont.color": preset.fill_text || tokens.font
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
            const traceName = String(trace.name || "");
            operations.push(Plotly.restyle(plot, {
              "line.color": traceName.includes("mean") ? preset.danger : withAlpha(preset.accent, 0.20),
              "marker.color": traceName.includes("mean") ? preset.danger : preset.accent
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
        localStorage.setItem(styleStorageKey, key);
        if (styleSelect) {
          styleSelect.value = key;
        }
        scheduleVisualRefresh();
      }

      function applyTheme(theme) {
        document.documentElement.dataset.theme = theme;
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

      function statusColor(status) {
        const preset = activePreset();
        const normalized = String(status || "").toLowerCase();
        if (normalized.includes("partial")) return preset.amber;
        if (["error", "missing", "not_supported", "unsupported"].some((token) => normalized.includes(token))) {
          return preset.danger;
        }
        if (normalized.includes("supported")) return preset.accent;
          return preset.accent2 || preset.colorway[1] || preset.accent;
      }

      function statusLabel(status) {
        return String(status || "unknown")
          .replaceAll("_", " ")
          .replace(/\\b\\w/g, (char) => char.toUpperCase());
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

      function tradeMetricColor(row) {
        return Number(row.npv || 0) < 0 ? activePreset().danger : nodeColor(tradeNodeId(row), 0);
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

      function buildDonut(rows, nodeId) {
        const totals = new Map();
        const idsByLabel = new Map();

        function add(label, id, exposure) {
          totals.set(label, (totals.get(label) || 0) + exposure);
          idsByLabel.set(label, id);
        }

        if (nodeId && nodeId.startsWith("asset:")) {
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

      function statusCounts(rows) {
        const counts = new Map();
        rows.forEach((row) => {
          counts.set(row.status, (counts.get(row.status) || 0) + 1);
        });
        return counts;
      }

      function supportStatuses(plot, counts) {
        const configured = plot.layout && plot.layout.meta && plot.layout.meta.supportStatuses;
        if (Array.isArray(configured) && configured.length > 0) {
          return configured.slice();
        }
        return Array.from(counts.keys()).sort((a, b) => a.localeCompare(b));
      }

      function supportStatusTraceIndex(plot) {
        const index = plot.layout && plot.layout.meta && plot.layout.meta.supportStatusTraceIndex;
        return Number.isInteger(index) ? index : 3;
      }

      function rowsForNode(rows, id) {
        if (!id || id === "portfolio") {
          return rows.slice();
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
          return `Trade: ${tradeId}`;
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
        filter.textContent = `${label} - ${rows.length} trade${rows.length === 1 ? "" : "s"} - ${money(sumExposure(rows))} exposure`;
      }

      function updateLinkedCharts(plot, rows, label, nodeId, options) {
        const settings = options || {};
        const sortedRows = rows.slice().sort((a, b) => {
          return String(`${a.asset_class}|${a.product_type}|${a.trade_id}`).localeCompare(
            String(`${b.asset_class}|${b.product_type}|${b.trade_id}`),
            undefined,
            { numeric: true, sensitivity: "base" }
          );
        });
        const tradeAxis = tradeAxisValues(sortedRows);
        const tradeData = tradeCustomData(sortedRows);
        const npvs = sortedRows.map((row) => Number(row.npv || 0));
        const statusCountMap = statusCounts(rows);
        const statusNames = supportStatuses(plot, statusCountMap);
        const statusValues = statusNames.map((status) => statusCountMap.get(status) || 0);
        const donut = buildDonut(rows, nodeId);
        const npvRange = paddedNumberRange(npvs, 1);
        const supportTrace = supportStatusTraceIndex(plot);
        const treeLevel = nodeId && !nodeId.startsWith("status:") && nodeId !== "portfolio" ? nodeId : "";

        const updates = [
          Plotly.restyle(plot, {
            "level": [treeLevel]
          }, [0]),
          Plotly.restyle(plot, {
            "customdata": [donut.ids],
            "ids": [donut.ids],
            "labels": [donut.labels],
            "marker.colors": [donut.colors],
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

        updates.push(Plotly.restyle(plot, {
          "customdata": [statusNames],
          "labels": [statusNames.map(statusLabel)],
          "marker.colors": [statusNames.map(statusColor)],
          "values": [statusValues]
        }, [supportTrace]));

        if (settings.updateTreeLevel === false) {
          updates.shift();
        }

        updates.push(Plotly.relayout(plot, {
          "annotations[1].text": boldChartTitle(`Allocation - ${compactLabel(label)}`),
          "annotations[2].text": boldChartTitle(`NPV - ${compactLabel(label)}`),
          "annotations[3].text": boldChartTitle(`Support - ${compactLabel(label)}`),
          "xaxis.range": npvRange,
          "yaxis.autorange": "reversed"
        }));

        setFilterLabel(plot, label, rows);
        return Promise.all(updates);
      }

      function initializePortfolioAllocation(plot) {
        if (!plot || !plot.layout || !plot.layout.meta || plot.layout.meta.qrpPanel !== "portfolio_allocation") {
          return;
        }
        if (plot.dataset.qrpLinked === "true") {
          return;
        }
        plot.dataset.qrpLinked = "true";

        const rows = (plot.layout.meta.tradeRows || []).slice();
        let activeRows = rows.slice();
        let syncingTreeLevel = false;
        const panel = plot.closest(".panel");
        const resetButton = panel ? panel.querySelector("[data-panel-reset]") : null;
        if (panel) {
          panel.classList.add("is-linked");
        }

        function applySelection(selectedRows, label, nodeId, options) {
          const settings = options || {};
          const nextRows = selectedRows.length > 0 ? selectedRows.slice() : rows.slice();
          const nextLabel = selectedRows.length > 0 ? label : "All portfolio";
          const nextNodeId = selectedRows.length > 0 ? nodeId : "portfolio";
          activeRows = nextRows;
          if (settings.updateTreeLevel !== false) {
            syncingTreeLevel = true;
          }
          const updatePromise = updateLinkedCharts(plot, nextRows, nextLabel, nextNodeId, settings);
          Promise.resolve(updatePromise).finally(() => {
            window.setTimeout(() => {
              syncingTreeLevel = false;
            }, 0);
          });
          return updatePromise;
        }

        function reset() {
          return applySelection(rows, "All portfolio", "portfolio");
        }

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
              applySelection(selectedRows, `Trade: ${tradeId}`, selectedRows[0] ? tradeNodeId(selectedRows[0]) : `trade:${tradeId}`);
            }, 0);
            return;
          }

          const supportTrace = supportStatusTraceIndex(plot);
          if (point.curveNumber === supportTrace) {
            const trace = plot.data && plot.data[supportTrace];
            const status = trace && trace.customdata
              ? trace.customdata[point.pointNumber]
              : "";
            const selectedRows = activeRows.filter((row) => row.status === status);
            window.setTimeout(() => {
              applySelection(selectedRows, `Pricing status: ${statusLabel(status)}`, `status:${status}`);
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
        setFilterLabel(plot, "All portfolio", rows);
      }

      function initializeLinkedPanels() {
        Array.from(document.querySelectorAll(".js-plotly-plot")).forEach(initializePortfolioAllocation);
      }

      if (document.readyState === "loading") {
        document.addEventListener("DOMContentLoaded", () => {
          initializeLinkedPanels();
          initializeSortableTables();
        });
      } else {
        initializeLinkedPanels();
        initializeSortableTables();
      }
      window.addEventListener("load", initializeLinkedPanels);
      window.addEventListener("load", initializeSortableTables);
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
      gap: 18px;
      grid-template-columns: repeat(auto-fit, minmax(230px, 1fr));
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
      padding: 20px 22px;
    }}
    .card-label {{
      color: var(--muted);
      font-size: 0.82rem;
      font-weight: 700;
      letter-spacing: 0.12em;
      text-transform: uppercase;
    }}
    .card-value {{
      color: var(--ink);
      font-family: "IBM Plex Mono", "Cascadia Mono", monospace;
      font-size: clamp(1.55rem, 3vw, 2.15rem);
      font-weight: 700;
      margin-top: 8px;
    }}
    .card-note {{
      color: var(--muted);
      font-size: 0.9rem;
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
    .panel .js-plotly-plot {{
      border-radius: 18px;
    }}
    .data-table-wrap {{
      max-height: var(--table-max-height, 620px);
      overflow-x: hidden;
      overflow-y: auto;
      position: relative;
      z-index: 1;
    }}
    .data-table {{
      border-collapse: collapse;
      font-size: 0.86rem;
      table-layout: fixed;
      width: 100%;
    }}
    .data-table th {{
      background: #102820;
      color: #f4fff9;
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
      border: 1px solid rgba(16, 40, 32, 0.11);
      color: var(--ink);
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
      background: rgba(255, 248, 235, 0.72);
    }}
    .data-table tbody tr:nth-child(even) {{
      background: rgba(241, 232, 214, 0.58);
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
      background: color-mix(in srgb, var(--accent) 16%, transparent);
      color: var(--accent);
    }}
    .status-pill-partially-supported {{
      background: color-mix(in srgb, var(--amber) 22%, transparent);
      color: #85622d;
    }}
    .status-pill-unsupported, .status-pill-failed {{
      background: color-mix(in srgb, var(--danger) 18%, transparent);
      color: var(--danger);
    }}
    .footnote {{
      color: var(--muted);
      font-size: 0.9rem;
      line-height: 1.45;
      margin-top: 28px;
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
        Interactive portfolio-risk dashboard generated from valuation, stress, factor exposure,
        P&amp;L explain, and Monte Carlo outputs. Built as a modern market cockpit for loss diagnostics,
        allocation review, and next-step risk investigations.
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
        Historical stress paths are scenario-ordered revaluations applied to the current portfolio as of
        {html.escape(market_as_of)}; they are not chronological observations. Monte Carlo path fans run from the
        market as-of date to each simulated horizon date.
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
    npv_values = [row["npv"] for row in trade_rows]
    donut = allocation_donut_segments(trade_rows)
    status_counts = aggregate_rows(
        [{**row, "count": 1.0} for row in trade_rows], "status", "count"
    )
    status_statuses = support_statuses_from_counts(status_counts)
    status_values = [status_counts.get(status, 0.0) for status in status_statuses]
    chart_height = bounded_chart_height(
        len(trade_rows),
        base_height=640,
        row_height=28,
        min_height=980,
        max_height=1420,
    )
    row_heights = [0.32, 0.52, 0.16] if len(trade_rows) > 22 else [0.38, 0.44, 0.18]

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
        rows=3,
        cols=2,
        column_widths=[0.52, 0.48],
        horizontal_spacing=0.08,
        row_heights=row_heights,
        specs=[
            [{"type": "domain"}, {"type": "domain"}],
            [{"type": "xy", "colspan": 2}, None],
            [{"type": "domain", "colspan": 2}, None],
        ],
        subplot_titles=[
            "Exposure Map",
            "Allocation Donut",
            "NPV by Trade",
            "Pricing Support Mix",
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
                [row["asset_class"], row["product_type"], row["trade_id"]]
                for row in trade_rows
            ],
            hovertemplate=(
                "<b>%{customdata[2]}</b><br>"
                "Asset: %{customdata[0]}<br>"
                "Product: %{customdata[1]}<br>"
                "NPV: %{x:,.2f}<extra></extra>"
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
    fig.add_trace(
        go.Pie(
            customdata=status_statuses,
            hole=0.58,
            hovertemplate="<b>%{label}</b><br>Trades: %{value:.0f}<br>Share: %{percent}<extra></extra>",
            labels=[support_status_label(status) for status in status_statuses],
            marker={
                "colors": [support_status_color(status) for status in status_statuses],
                "line": {"color": "rgba(255,255,255,0.38)", "width": 2},
            },
            meta={"qrpSupportDonut": True},
            name="Pricing Support",
            sort=False,
            textfont={"color": fill_text_color, "family": FINANCE_FONT, "size": 12},
            textinfo="label+percent",
            values=status_values,
        ),
        row=3,
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
            "supportStatusTraceIndex": 3,
            "supportStatuses": status_statuses,
            "tradeRows": trade_rows,
        }
    )
    fig.update_xaxes(
        range=padded_number_range(npv_values), title_text="NPV", row=2, col=1
    )
    fig.update_yaxes(
        autorange="reversed", automargin=True, row=2, col=1, type="multicategory"
    )
    return fig


def make_trade_inventory_figure(go, trade_rows):
    columns = [
        ("trade_id", "Trade", "text"),
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
        ("trade_id", "Trade", "text"),
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
                "status_message": "All trades use fully supported pricing profiles.",
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
    all_buckets = sorted(
        {bucket for result in sorted_results for bucket in result.bucketed_risk}
    )
    z = [
        [result.bucketed_risk.get(bucket, 0.0) for bucket in all_buckets]
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
        specs=[[{"type": "xy"}, {"type": "xy"}]],
        subplot_titles=[
            "Aggregate Sensitivities by Trade",
            "Bucketed Rate/Credit Risk",
        ],
    )
    fig.add_trace(
        go.Bar(
            name="PV01",
            orientation="h",
            x=[result.pv01 for result in sorted_results],
            y=trade_ids,
        ),
        row=1,
        col=1,
    )
    fig.add_trace(
        go.Bar(
            name="CS01",
            orientation="h",
            x=[result.cs01 for result in sorted_results],
            y=trade_ids,
        ),
        row=1,
        col=1,
    )
    if all_buckets:
        fig.add_trace(
            go.Heatmap(
                colorbar={"title": "Risk"},
                colorscale=DIVERGING_SCALE,
                x=all_buckets,
                y=trade_ids,
                zmid=0.0,
                z=z,
            ),
            row=1,
            col=2,
        )
    apply_finance_layout(fig, barmode="group", height=height, showlegend=True)
    fig.update_layout(
        margin={"b": 86, "l": categorical_left_margin(trade_ids), "r": 38, "t": 82}
    )
    fig.update_xaxes(title_text="Sensitivity", row=1, col=1)
    fig.update_yaxes(autorange="reversed", automargin=True, row=1, col=1)
    fig.update_yaxes(autorange="reversed", automargin=True, row=1, col=2)
    return fig


def make_historical_stress_figure(go, make_subplots, stress_results, trade_rows):
    stress_sorted = sorted(stress_results, key=lambda result: result.total_pnl)
    scenario_names = [result.scenario_name for result in stress_sorted]
    scenario_pnls = [result.total_pnl for result in stress_sorted]
    trade_ids = sorted(
        {trade_id for result in stress_results for trade_id in result.trade_pnls},
        key=lambda trade_id: trade_id_sort_key(trade_id, trade_rows),
    )
    stress_matrix = [
        [result.trade_pnls.get(trade_id, 0.0) for result in stress_sorted]
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
            "Trade P&L Matrix",
            f"Worst Scenario Contribution: {worst.scenario_name}",
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
            colorbar={"title": "Trade P&L", "thickness": 18},
            colorscale=DIVERGING_SCALE,
            hovertemplate="<b>%{y}</b><br>%{x}<br>P&L: %{z:,.0f}<extra></extra>",
            x=scenario_names,
            y=trade_ids,
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
        automargin=True,
        tickangle=CATEGORY_TICKANGLE,
        tickfont={"family": FINANCE_FONT, "size": 9},
        tickmode="array",
        ticktext=[axis_tick_label(scenario_name) for scenario_name in scenario_names],
        tickvals=scenario_names,
        title_text="Scenario",
        row=2,
        col=1,
    )
    fig.update_xaxes(
        range=padded_number_range(
            [pnl for _, pnl in worst_trade_pnls] + [worst.total_pnl]
        ),
        title_text="Trade Contribution",
        row=3,
        col=1,
    )
    fig.update_yaxes(autorange="reversed", automargin=True, row=1, col=1)
    fig.update_yaxes(autorange="reversed", automargin=True, row=2, col=1)
    fig.update_yaxes(autorange="reversed", automargin=True, row=3, col=1)
    return fig


def make_performance_drawdown_figure(
    go, make_subplots, total_npv, stress_results, market_as_of=None
):
    scenario_names, equity_values, drawdowns, max_drawdown, _ = stress_performance_path(
        total_npv, stress_results
    )
    normalized = [value - equity_values[0] for value in equity_values]
    as_of_label = format_date(market_as_of)

    fig = make_subplots(
        rows=2,
        cols=1,
        shared_xaxes=True,
        subplot_titles=[
            f"Scenario-Ordered Portfolio Performance (As of {as_of_label})",
            f"Scenario-Ordered Drawdown (Max Drawdown {pct(max_drawdown)})",
        ],
    )
    fig.add_trace(
        go.Scatter(
            mode="lines+markers",
            name="Cumulative P&L",
            x=scenario_names,
            y=normalized,
            line={"color": POSITIVE_COLOR, "width": 3},
        ),
        row=1,
        col=1,
    )
    fig.add_trace(
        go.Bar(
            name="Drawdown",
            x=scenario_names,
            y=[value * 100.0 for value in drawdowns],
            marker_color=NEGATIVE_COLOR,
        ),
        row=2,
        col=1,
    )
    apply_finance_layout(fig, height=680)
    fig.update_xaxes(
        title_text="Stress scenario order (not chronological time)", row=2, col=1
    )
    fig.update_yaxes(title_text="Cumulative P&L", row=1, col=1)
    fig.update_yaxes(title_text="Drawdown %", row=2, col=1)
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
            y=trade_ids,
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
            y=trade_ids,
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
            y=trade_ids,
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
            y=trade_ids,
            marker_color=NEGATIVE_COLOR,
        )
    )
    apply_finance_layout(
        fig,
        barmode="relative",
        height=height,
        showlegend=True,
        title="P&L Explain by Trade",
    )
    fig.update_layout(
        margin={"b": 86, "l": categorical_left_margin(trade_ids), "r": 38, "t": 82}
    )
    fig.update_xaxes(title_text="P&L")
    fig.update_yaxes(autorange="reversed", automargin=True)
    return fig


def make_factor_scenario_figure(go, make_subplots, factors, scenarios):
    factor_ids = [factor.factor_id for factor in factors]
    scenario_names = [scenario.name for scenario in scenarios]
    z = [
        [scenario.factor_shocks.get(factor_id, 0.0) for factor_id in factor_ids]
        for scenario in scenarios
    ]
    factor_type_counts = defaultdict(int)
    for factor in factors:
        factor_type_counts[enum_label(factor.factor_type)] += 1
    height = bounded_chart_height(
        max(len(scenario_names), len(factor_type_counts)),
        base_height=420,
        row_height=20,
        min_height=620,
        max_height=1000,
    )

    fig = make_subplots(
        rows=1,
        cols=2,
        specs=[[{"type": "xy"}, {"type": "xy"}]],
        subplot_titles=["Scenario Factor Shock Matrix", "Factor Coverage by Type"],
    )
    fig.add_trace(
        go.Heatmap(
            colorbar={"title": "Shock"},
            colorscale=DIVERGING_SCALE,
            x=[short_factor_id(factor_id) for factor_id in factor_ids],
            y=scenario_names,
            zmid=0.0,
            z=z,
        ),
        row=1,
        col=1,
    )
    fig.add_trace(
        go.Bar(
            x=list(factor_type_counts),
            y=list(factor_type_counts.values()),
            marker_color=POSITIVE_COLOR,
        ),
        row=1,
        col=2,
    )
    apply_finance_layout(fig, height=height)
    fig.update_layout(
        margin={
            "b": 110,
            "l": categorical_left_margin(scenario_names),
            "r": 42,
            "t": 82,
        }
    )
    fig.update_xaxes(tickangle=CATEGORY_TICKANGLE, row=1, col=1)
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
    stress_results,
    mc_results,
    optimization_result=None,
):
    trade_rows = build_trade_rows(portfolio, valuation_results)
    gross_exposure = sum(row["exposure"] for row in trade_rows)
    worst_stress = min(stress_results, key=lambda result: result.total_pnl)
    _, _, _, max_drawdown, max_drawdown_amount = stress_performance_path(
        total_npv, stress_results
    )
    mc_card = mc_results[0][1] if mc_results else None
    optimization_note = "not run"
    if optimization_result:
        optimization_note = optimization_result.get("status", "unknown")

    return [
        {
            "label": "Portfolio NPV",
            "value": money(total_npv),
            "note": f"{len(trade_rows)} trades across {len(set(row['asset_class'] for row in trade_rows))} asset classes",
        },
        {
            "label": "Gross Exposure Proxy",
            "value": money(gross_exposure),
            "note": "Uses notional, equity market value, or abs(NPV) fallback",
        },
        {
            "label": "Worst Historical Stress",
            "value": money(worst_stress.total_pnl),
            "note": worst_stress.scenario_name,
        },
        {
            "label": "Stress-Path Max Drawdown",
            "value": pct(max_drawdown),
            "note": money(max_drawdown_amount),
        },
        {
            "label": "Monte Carlo VaR / ES",
            "value": f"{money(mc_card.var_95)} / {money(mc_card.expected_shortfall_95)}"
            if mc_card
            else "n/a",
            "note": mc_results[0][0] if mc_results else "No Monte Carlo results",
        },
        {
            "label": "Optimization Worker",
            "value": optimization_note,
            "note": "Optional CVXPY dependency check",
        },
    ]


def make_dashboard_figures(
    go,
    make_subplots,
    portfolio,
    valuation_results,
    total_npv,
    stress_results,
    risk_results,
    pnl_results,
    mc_results,
    factors,
    scenarios,
    market_as_of,
):
    trade_rows = build_trade_rows(portfolio, valuation_results)
    return [
        (
            "Portfolio Allocation",
            make_portfolio_allocation_figure(go, make_subplots, trade_rows),
        ),
        ("Support Diagnostics", make_support_diagnostics_panel(trade_rows)),
        ("Trade Inventory", make_trade_inventory_figure(go, trade_rows)),
        (
            "Risk Measures",
            make_risk_measures_figure(go, make_subplots, risk_results, trade_rows),
        ),
        (
            "Historical Stress",
            make_historical_stress_figure(
                go, make_subplots, stress_results, trade_rows
            ),
        ),
        (
            "Performance and Drawdown",
            make_performance_drawdown_figure(
                go, make_subplots, total_npv, stress_results, market_as_of
            ),
        ),
        (
            "Monte Carlo VaR and Future Path Fan",
            make_monte_carlo_figure(go, make_subplots, mc_results, market_as_of),
        ),
        ("P&L Explain", make_pnl_explain_figure(go, pnl_results, trade_rows)),
        (
            "Risk Factor Scenario Coverage",
            make_factor_scenario_figure(go, make_subplots, factors, scenarios),
        ),
    ]


def make_dashboard_view(
    go,
    make_subplots,
    analytics,
    factors,
    scenarios,
    market_as_of,
    optimization_result=None,
):
    portfolio = analytics["portfolio"]
    valuation_results = analytics["valuation_results"]
    total_npv = analytics["total_npv"]
    stress_results = analytics["stress_results"]
    risk_results = analytics["risk_results"]
    pnl_results = analytics["pnl_results"]
    mc_results = analytics["mc_results"]
    trade_rows = build_trade_rows(portfolio, valuation_results)
    supported_count = sum(1 for row in trade_rows if row["status"] == "supported")
    asset_class_count = len(set(row["asset_class"] for row in trade_rows))
    summary = f"{len(trade_rows)} trades | {supported_count} supported | {asset_class_count} asset classes"

    return {
        "cards": make_dashboard_cards(
            portfolio,
            valuation_results,
            total_npv,
            stress_results,
            mc_results,
            optimization_result,
        ),
        "description": portfolio_description(portfolio),
        "figures": make_dashboard_figures(
            go,
            make_subplots,
            portfolio,
            valuation_results,
            total_npv,
            stress_results,
            risk_results,
            pnl_results,
            mc_results,
            factors,
            scenarios,
            market_as_of,
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
            optimization_result,
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
