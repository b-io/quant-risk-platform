    (function () {
      const stylePresetNode = document.getElementById("qrp-style-presets");
      const stylePresets = JSON.parse(stylePresetNode ? stylePresetNode.textContent : "{}");
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
            return Number(row.display_npv ?? row.npv ?? 0) < 0
              ? preset.danger
              : nodeColor(tradeNodeIdFromRow(row), pointIndex, preset);
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
              updates[`${axisName}.rangeselector.activecolor`] = withAlpha(
                preset.accent,
                theme === "dark" ? 0.38 : 0.26
              );
              updates[`${axisName}.rangeselector.bgcolor`] = theme === "dark"
                ? mixHex(tokens.tableCell, "#000000", 0.10)
                : "rgba(255,255,255,0.72)";
              updates[`${axisName}.rangeselector.bordercolor`] = withAlpha(
                preset.accent,
                theme === "dark" ? 0.44 : 0.24
              );
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
        const header = theme === "dark"
          ? mixHex(preset.accent, "#061016", 0.16)
          : mixHex(preset.accent, "#102820", 0.68);
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
        root.setProperty(
          "--table-border",
          theme === "dark" ? withAlpha(preset.accent, 0.20) : withAlpha("#102820", 0.12)
        );
        root.setProperty("--table-muted", tokens.tableMuted || tokens.font);
        root.setProperty("--status-supported-bg", withAlpha(preset.accent, theme === "dark" ? 0.18 : 0.14));
        root.setProperty(
          "--status-supported-fg",
          theme === "dark" ? mixHex(preset.accent, "#ffffff", 0.18) : preset.accent
        );
        root.setProperty("--status-partial-bg", withAlpha(preset.amber, theme === "dark" ? 0.22 : 0.18));
        root.setProperty("--status-partial-fg", theme === "dark" ? mixHex(preset.amber, "#ffffff", 0.18) : "#85622d");
        root.setProperty("--status-unsupported-bg", withAlpha(preset.danger, theme === "dark" ? 0.20 : 0.16));
        root.setProperty(
          "--status-unsupported-fg",
          theme === "dark" ? mixHex(preset.danger, "#ffffff", 0.20) : preset.danger
        );
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
      function refreshDashboardVisuals() {
        applyTableTheme(document.documentElement.dataset.theme || "light", activeStyleKey());
        resizeActivePlots();
        scheduleVisualRefresh();
      }
      window.qrpDashboardRefreshVisuals = refreshDashboardVisuals;
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
        document.dispatchEvent(new CustomEvent("qrp:portfolio-view-changed", {
          detail: { viewId: selected.dataset.portfolioView }
        }));
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

      function findAll(root, selector) {
        const scope = root || document;
        const matches = Array.from(scope.querySelectorAll(selector));
        if (scope.matches && scope.matches(selector)) {
          matches.unshift(scope);
        }
        return matches;
      }

      function hydratePanel(panel) {
        const lazy = panel && panel.querySelector("[data-lazy-panel]");
        if (!lazy || lazy.dataset.hydrated === "true") {
          return;
        }
        const template = lazy.querySelector("template[data-lazy-template]");
        if (!template) {
          lazy.dataset.hydrated = "true";
          return;
        }
        const fragment = template.content.cloneNode(true);
        const deferredScripts = [];
        fragment.querySelectorAll("script").forEach((script, index) => {
          const marker = document.createComment(`qrp-lazy-script-${index}`);
          deferredScripts.push({
            attributes: Array.from(script.attributes),
            marker,
            text: script.textContent
          });
          script.replaceWith(marker);
        });
        lazy.replaceChildren(fragment);
        deferredScripts.forEach(({ attributes, marker, text }) => {
          const executable = document.createElement("script");
          attributes.forEach((attribute) => {
            executable.setAttribute(attribute.name, attribute.value);
          });
          executable.textContent = text;
          marker.replaceWith(executable);
        });
        lazy.dataset.hydrated = "true";
      }

      function initializeSortableTables(root = document) {
        findAll(root, "[data-sortable-table]").forEach((table) => {
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

      function initializePanelContents(panel) {
        if (!panel) {
          return;
        }
        hydratePanel(panel);
        initializeLinkedPanels(panel);
        initializeLoadingStates(panel);
        initializeSortableTables(panel);
        if (typeof window.qrpDashboardRefreshVisuals === "function") {
          window.setTimeout(window.qrpDashboardRefreshVisuals, 80);
        }
        window.setTimeout(() => {
          initializeLinkedPanels(panel);
          initializeLoadingStates(panel);
        }, 240);
      }

      function activateTab(button, options = {}) {
        if (!button) {
          return;
        }
        const view = button.closest(".portfolio-view");
        if (!view) {
          return;
        }
        const panelId = button.dataset.dashboardTab;
        const panel = panelId ? document.getElementById(panelId) : null;
        if (!panel || panel.closest(".portfolio-view") !== view) {
          return;
        }
        view.querySelectorAll("[data-dashboard-tab]").forEach((tab) => {
          tab.setAttribute("aria-selected", tab === button ? "true" : "false");
        });
        view.querySelectorAll("[data-dashboard-panel]").forEach((candidate) => {
          candidate.hidden = candidate !== panel;
        });
        initializePanelContents(panel);
        if (options.focus) {
          button.focus();
        }
      }

      function initializeDashboardTabs(root = document) {
        findAll(root, "[data-dashboard-tabs]").forEach((tablist) => {
          if (tablist.dataset.tabsReady !== "true") {
            tablist.dataset.tabsReady = "true";
            const tabs = Array.from(tablist.querySelectorAll("[data-dashboard-tab]"));
            tabs.forEach((tab, index) => {
              tab.addEventListener("click", () => activateTab(tab));
              tab.addEventListener("keydown", (event) => {
                if (!["ArrowLeft", "ArrowRight", "Home", "End"].includes(event.key)) {
                  return;
                }
                event.preventDefault();
                const lastIndex = tabs.length - 1;
                let nextIndex = index;
                if (event.key === "ArrowLeft") {
                  nextIndex = index === 0 ? lastIndex : index - 1;
                } else if (event.key === "ArrowRight") {
                  nextIndex = index === lastIndex ? 0 : index + 1;
                } else if (event.key === "Home") {
                  nextIndex = 0;
                } else if (event.key === "End") {
                  nextIndex = lastIndex;
                }
                activateTab(tabs[nextIndex], { focus: true });
              });
            });
          }
          const view = tablist.closest(".portfolio-view");
          if (view && !view.hidden) {
            activateTab(tablist.querySelector('[aria-selected="true"]') || tablist.querySelector("[data-dashboard-tab]"));
          }
        });
      }

      function activateVisiblePortfolioTab() {
        const view = document.querySelector(".portfolio-view:not([hidden])");
        if (!view) {
          return;
        }
        initializeDashboardTabs(view);
        const selectedTab = view.querySelector('[data-dashboard-tab][aria-selected="true"]')
          || view.querySelector("[data-dashboard-tab]");
        activateTab(selectedTab);
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
        const countLabel = `${rows.length} position${rows.length === 1 ? "" : "s"}`;
        filter.textContent = `${label} - ${countLabel} - ${money(sumExposure(rows))} exposure`;
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
              const selectedNodeId = selectedRows[0] ? tradeNodeId(selectedRows[0]) : `trade:${tradeId}`;
              applySelection(selectedRows, `Position: ${tradeId}`, selectedNodeId);
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

      function initializeLoadingStates(root = document) {
        findAll(root, ".card.is-loading").forEach((card, index) => {
          window.setTimeout(() => markLoaded(card), 120 + index * 45);
        });
        findAll(root, ".panel.is-loading").forEach((panel) => {
          const lazy = panel.querySelector("[data-lazy-panel]");
          if (panel.hidden || (lazy && lazy.dataset.hydrated !== "true")) {
            return;
          }
          if (panel.dataset.loadingReady === "true") {
            return;
          }
          panel.dataset.loadingReady = "true";
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

      function initializeLinkedPanels(root = document) {
        Array.from(root.querySelectorAll(".js-plotly-plot")).forEach(initializePortfolioAllocation);
      }

      if (document.readyState === "loading") {
        document.addEventListener("DOMContentLoaded", () => {
          initializeDashboardTabs();
          initializeLinkedPanels();
          initializeLoadingStates();
          initializeSortableTables();
        });
      } else {
        initializeDashboardTabs();
        initializeLinkedPanels();
        initializeLoadingStates();
        initializeSortableTables();
      }
      document.addEventListener("qrp:portfolio-view-changed", activateVisiblePortfolioTab);
      window.addEventListener("load", initializeDashboardTabs);
      window.addEventListener("load", initializeLinkedPanels);
      window.addEventListener("load", initializeLoadingStates);
      window.addEventListener("load", initializeSortableTables);
      [250, 1000, 2500].forEach((delay) => {
        window.setTimeout(activateVisiblePortfolioTab, delay);
        window.setTimeout(initializeLinkedPanels, delay);
      });
    }());
