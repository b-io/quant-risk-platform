# P&L Explain in Practice

This chapter describes how day-over-day P&L is decomposed in a production analytics platform.

## Notation used in this chapter

Unless stated otherwise:

- $V_t$ is the portfolio value at the end of day $t$.
- $M_t$ is the market state observed on day $t$.
- $\mathcal{T}_t$ is the trade population on day $t$.
- $CF_t$ is realized cash over day $t$.
- $dt$ is one reporting step such as one business day.

## 1. Actual and theoretical P&L

Let actual P&L be the observed day-over-day value change plus recognized cash. Then

$$
P\&L_{actual}=V_t-V_{t-1}+CF_t
$$

Where:

- $P\&L_{actual}$ is actual or economic P&L over the interval.

A practical explain decomposition is

$$
P\&L_{actual}=P\&L_{carry}+P\&L_{roll}+P\&L_{market}+P\&L_{cash}+P\&L_{trade\ flow}+P\&L_{model}+P\&L_{residual}
$$

Where:

- $P\&L_{carry}$ is accrual-type P&L under frozen markets.
- $P\&L_{roll}$ is mark-to-market change from aging along a non-flat curve or surface.
- $P\&L_{market}$ is P&L from actual market moves.
- $P\&L_{cash}$ is coupons, settlements, and fixings.
- $P\&L_{trade\ flow}$ is P&L from new trades, unwinds, or amendments.
- $P\&L_{model}$ is P&L from model, methodology, or configuration changes.
- $P\&L_{residual}$ is unexplained P&L after all explicit components are booked.

## 2. Carry and roll-down

Carry and roll-down are separated because they have different meanings.

- Carry is the P&L earned simply because time passes while market levels are frozen.
- Roll-down is the extra mark-to-market change caused by sliding to a shorter point on a non-flat curve or surface.

A robust production definition uses full repricing under a frozen market.

Let $V^{frozen}_{t-1\to t}$ be the value of yesterday's continuing portfolio aged by one day and repriced with yesterday's market state held fixed. Then

$$
P\&L_{carry+roll}=V^{frozen}_{t-1\to t}-V_{t-1}
$$

Where:

- $V^{frozen}_{t-1\to t}$ is yesterday's continuing portfolio aged to today under frozen market data.

For a zero-coupon bond with price $P(\tau)=e^{-y(\tau)\tau}$ as a function of remaining maturity $\tau$, a first-order frozen-market decomposition is

$$
\frac{dP}{P}\approx \left(y(\tau)+\tau y'(\tau)\right)dt
$$

Where:

- $y(\tau)$ is the zero rate at remaining maturity $\tau$.
- $y'(\tau)$ is the slope of the zero curve with respect to maturity.

Interpretation:

- $y(\tau)dt$ is carry,
- $\tau y'(\tau)dt$ is roll-down.

### Example: carry and roll-down on an upward-sloping curve

Suppose a zero-coupon bond has price $P=95$, maturity $\tau=5$ years, zero rate $y(5)=4\%$, curve slope $y'(5)=0.20\%$ per year, and one-day horizon $dt=1/252$. Then

$$
\Delta P_{carry}\approx 95\times 0.04\times \frac{1}{252}=0.0151
$$

$$
\Delta P_{roll}\approx 95\times 5\times 0.002\times \frac{1}{252}=0.0038
$$

So frozen-market carry plus roll is about 0.0189.

## 3. Market-move P&L

Let market-move P&L be the revaluation of yesterday's continuing trades under today's market state. Then

$$
P\&L_{market}=V(M_t\ ;\mathcal{T}_{t-1})-V(M_{t-1}\ ;\mathcal{T}_{t-1})
$$

Where:

- $V(M_t\ ;\mathcal{T}_{t-1})$ is the value of yesterday's continuing trade set under today's market state.

A local approximation may split this into delta, gamma, vega, rho, DV01, or CS01 contributions. Official reporting is usually more reliable with full revaluation.

## 4. Cash and fixings

Cash and fixing effects should be isolated explicitly. Typical items are:

- coupon payments,
- premium settlements,
- floating-rate fixings,
- principal redemptions,
- fees or commissions when the report includes them.

These events can change valuation even if the broader market state is unchanged.

## 5. Trade-flow P&L

Let trade-flow P&L be the difference between today's full trade population and yesterday's continuing trade population under the same market state. Then

$$
P\&L_{trade\ flow}=V(M_t\ ;\mathcal{T}_t)-V(M_t\ ;\mathcal{T}_{t-1})
$$

This separates business activity from genuine market performance.

## 6. Model-change P&L

Let $\mathcal{M}_{old}$ and $\mathcal{M}_{new}$ denote two model or methodology configurations. Then

$$
P\&L_{model}=V(M_t\ ;\mathcal{T}_t\ ;\mathcal{M}_{new})-V(M_t\ ;\mathcal{T}_t\ ;\mathcal{M}_{old})
$$

Model-change P&L should be reported explicitly rather than mixed into market P&L.

## 7. Residual control

Let explained P&L be the sum of all explicit components. Then

$$
P\&L_{residual}=P\&L_{actual}-P\&L_{explained}
$$

Persistent residuals usually indicate:

- stale or inconsistent market data,
- booking mismatches,
- missing fixings or accruals,
- model-version differences,
- omitted valuation adjustments,
- approximation error from a local explain.

## 8. Practical implementation flow

A stable daily explain workflow is:

1. freeze the continuing portfolio,
2. archive yesterday's and today's market states,
3. compute frozen-market carry and roll,
4. compute market-move P&L,
5. isolate cash and fixings,
6. isolate trade-flow effects,
7. isolate model-change effects,
8. report the residual with diagnostics and lineage.

## 9. Current platform mapping

The platform implementation follows this workflow through `PnlExplainService`.
For each trade it prices the previous snapshot, the current snapshot, and the
previous market rolled to the current valuation date. Actual P&L is

$$
P\&L_{actual}=V_t-V_{t-1}+CF_t
$$

where `CF_t` is supplied by product-specific realized-cash extractors.

Implemented behavior:

- Carry is the rolled previous-market valuation less previous NPV.
- Market move is either aggregate full revaluation or sequential full
  revaluation by factor-bound quote.
- Deposit maturity principal and simple ACT/360 interest are recognized in the
  cash line when the maturity date is inside the explain interval.
- Every explain row includes explicit carry, roll-down, market move, cash,
  trade activity, FX translation, model-change, and residual components.
- Results and components are stored in `pnl_explain_results` and
  `pnl_explain_components`, with component sequence, type, factor id,
  risk-factor group, model, support status, and tags.

Current boundaries:

- Roll-down is not yet split from frozen-market carry for curve products.
- Trade activity is zero because the current request shape explains one
  continuing portfolio rather than two trade populations.
- FX translation is zero until valuation currency and reporting currency are
  separated.
- Model-change P&L is zero until model-version or configuration snapshots are
  introduced.
- The residual is computed explicitly as actual P&L less the non-residual
  components and is tagged by trade, book, asset class, currency, product type,
  and reconciliation status.
