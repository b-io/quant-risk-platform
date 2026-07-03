# Risk, Backtesting and Governance

## Risk factor

A variable that drives portfolio value.

Examples:

- German Power Base Cal-2027;
- TTF Q1-2027;
- EUA Dec-2027;
- clean spark spread;
- implied volatility;
- cross-currency FX rates.

## PnL

Profit and loss over a time step:

$$
PnL_{t,t+\Delta t} =
V(t+\Delta t,X_{t+\Delta t}) -
V(t,X_t)
$$

## Sensitivity

Approximate change in value:

$$
\Delta V \approx
\sum_i
\frac{\partial V}{\partial X_i}
\Delta X_i
$$

## Delta

$$
\Delta_i =
\frac{\partial V}{\partial X_i}
$$

## Gamma

$$
\Gamma_{ij} =
\frac{\partial^2 V}{\partial X_i \partial X_j}
$$

## Vega

$$
\text{Vega} =
\frac{\partial V}{\partial \sigma}
$$

## VaR

If losses are positive:

$$
VaR_\alpha = q_\alpha(L)
$$

If using PnL where losses are negative:

$$
VaR_{99\%} = -q_{1\%}(PnL)
$$

## Expected Shortfall

$$
ES_{99\%} = -\mathbb{E}
\left[
PnL
\mid
PnL \leq q_{1\%}(PnL)
\right]
$$

## Stress testing

Apply severe but plausible shocks:

```text
TTF +40%
German power +25%
EUA +15%
Low hydro inflows
Nuclear outage
Correlation breakdown
Liquidity dry-up
```

Revalue:

$$
Loss^{stress} =
V(X)-V(X^{stress})
$$

## Backtesting

Backtesting checks whether models worked historically.

Forecast error:

$$
e_t = y_t - \hat y_t
$$

RMSE:

$$
RMSE =
\sqrt{
\frac{1}{N}
\sum_{t=1}^{N} e_t^2
}
$$

VaR exceedance indicator:

$$
I_t =
\mathbf{1}_{PnL_t < -VaR_t}
$$

Expected exceedance rate for 99% VaR:

$$
\mathbb{E}[I_t] = 1\%
$$

## Model validation

Check:

- input data quality;
- calibration stability;
- pricing accuracy;
- backtesting performance;
- stress behaviour;
- sensitivity reasonableness;
- limits of applicability;
- implementation correctness;
- documentation quality.

## Governance documentation

A model document should include:

- purpose;
- product coverage;
- equations;
- assumptions;
- calibration data;
- parameter estimation;
- validation tests;
- limitations;
- monitoring;
- change log;
- owner;
- approval status.

## Platform Governance

Governance is part of the model lifecycle rather than a document produced after implementation. A pricing or risk model
should preserve:

- assumptions and limits of applicability;
- input data lineage;
- calibration procedure and calibration data;
- validation tests and monitoring thresholds;
- versioned code, configuration, and market data;
- known limitations and fallback behavior.

In platform terms, governance means that a valuation result can be traced back to the trade state, market state, model
state, and valuation context that produced it.
