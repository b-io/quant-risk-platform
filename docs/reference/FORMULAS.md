# Formula Reference

This chapter records shared notation and formulas used across the documentation.
Asset-class chapters may derive or specialize these expressions, but the common
notation should remain aligned with this file.

Use dollar-delimited LaTeX in Markdown:

- inline formulas use `$...$`;
- displayed formulas use `$$...$$` on separate lines.

## Valuation Function

A platform valuation can be written abstractly as

$$
PV =
f(
\text{trade state},
\text{market state},
\text{model state},
\text{valuation context}
).
$$

This expression is an implementation contract: reproducible valuation requires
the persisted run to reconstruct each argument of $f$.

## Discounting

For deterministic cashflows $CF_i$ paid at dates $T_i$, the present value is

$$
PV = \sum_{i=1}^{n} CF_i D(t,T_i),
$$

where $D(t,T_i)$ is the discount factor observed or constructed at valuation
date $t$.

With a continuously compounded zero rate $z(t,T)$,

$$
D(t,T) = \exp\left[-z(t,T)(T-t)\right].
$$

With a simple rate $R(t,T)$ over year fraction $\tau(t,T)$,

$$
D(t,T) = \frac{1}{1 + R(t,T)\tau(t,T)}.
$$

## Forward Rates

The simply compounded forward rate implied by discount factors over
$[T_1,T_2]$ is

$$
F(t;T_1,T_2)
=
\frac{1}{\tau(T_1,T_2)}
\left(
\frac{D(t,T_1)}{D(t,T_2)}
- 1
\right).
$$

For an overnight-indexed or floating-rate coupon with notional $N$, accrual
factor $\tau_i$, and fixing or projected rate $L_i$,

$$
CF_i = N \tau_i L_i.
$$

## Par Swap Rate

For fixed payment dates $T_i$ with accrual factors $\alpha_i$, the par fixed
rate of a vanilla fixed-for-floating swap can be expressed as

$$
S(t)
=
\frac{D(t,T_0) - D(t,T_n)}
{\sum_{i=1}^{n} \alpha_i D(t,T_i)}.
$$

The exact implementation must use the product schedule, day-count convention,
business-day convention, compounding convention, and projection index.

## Bond Pricing

For a fixed-rate bond with coupon cashflows $C_i$ and principal $N$,

$$
PV =
\sum_{i=1}^{n} C_i D(t,T_i) + N D(t,T_n).
$$

A spread-discounted representation with spread $s$ can be written as

$$
PV(s)
=
\sum_{i=1}^{n} CF_i D(t,T_i) \exp[-s(T_i-t)].
$$

This is a spread measure, not a default model by itself.

## Credit Survival

If $\lambda(u)$ is the hazard rate, the survival probability to $T$ is

$$
Q(t,T)
=
\exp\left(
-\int_t^T \lambda(u)\,du
\right).
$$

A reduced-form defaultable cashflow valuation uses discounting and survival:

$$
PV_{\text{promised}}
=
\sum_{i=1}^{n} CF_i D(t,T_i) Q(t,T_i).
$$

The recovery leg for default over intervals $[T_{i-1},T_i]$ is often
approximated by

$$
PV_{\text{recovery}}
=
R N
\sum_{i=1}^{n}
D(t,T_i)
\left[
Q(t,T_{i-1}) - Q(t,T_i)
\right],
$$

where $R$ is recovery rate and $N$ is notional.

## FX Forward

For domestic discount factor $D_d(t,T)$ and foreign discount factor
$D_f(t,T)$, the no-arbitrage FX forward under a spot quote $S_t$ is

$$
F_{d/f}(t,T)
=
S_t
\frac{D_f(t,T)}{D_d(t,T)}.
$$

The currency-pair convention must define which currency is domestic, which is
foreign, and how spot and forward points are quoted.

## Equity Forward

For equity spot $S_t$, discount factor $D(t,T)$, and present value of discrete
dividends $PV_{\text{div}}(t,T)$,

$$
F(t,T)
=
\frac{S_t - PV_{\text{div}}(t,T)}
{D(t,T)}.
$$

With continuous dividend yield $q$ and funding rate $r$,

$$
F(t,T) = S_t \exp[(r-q)(T-t)].
$$

Borrow and funding conventions should be modeled explicitly when material.

## Commodity Delivery-Period Average

For hourly or daily forward nodes $f_h$ over a delivery period
$[T_1,T_2]$, a simple delivery-period forward is

$$
F(t;T_1,T_2)
=
\frac{1}{N}
\sum_{h \in [T_1,T_2]} f_h.
$$

A weighted version is

$$
F(t;T_1,T_2)
=
\frac{
\sum_{h \in [T_1,T_2]} w_h f_h
}{
\sum_{h \in [T_1,T_2]} w_h
}.
$$

For a peakload product over a peak-hour set $\mathcal{P}(T_1,T_2)$,

$$
F_{\text{peak}}(t;T_1,T_2)
=
\frac{
\sum_{h \in \mathcal{P}(T_1,T_2)} w_h f_h
}{
\sum_{h \in \mathcal{P}(T_1,T_2)} w_h
}.
$$

## Curve Fitting

Let $Q$ be a vector of quoted contract prices, $f$ a vector of granular forward
nodes, and $A$ the aggregation matrix mapping nodes to quoted delivery periods:

$$
Q = Af.
$$

A weighted least-squares fit is

$$
\min_f
\sum_i
\omega_i
\left((Af)_i - Q_i^{\text{mid}}\right)^2.
$$

A smoothness-penalized fit is

$$
\min_f
\sum_i
\omega_i
\left((Af)_i - Q_i^{\text{mid}}\right)^2
+
\lambda
\sum_h
\left(f_{h-1} - 2f_h + f_{h+1}\right)^2.
$$

Bid and ask consistency can be written as

$$
Q_i^{\text{bid}} \leq (Af)_i \leq Q_i^{\text{ask}}.
$$

## Spark Spreads

For power price $P_{\text{power}}$, gas price $P_{\text{gas}}$, heat rate $h$,
carbon emissions rate $e$, carbon price $P_{\text{CO2}}$, and variable
operating cost $VOM$, the simplified spark spread is

$$
SS = P_{\text{power}} - h P_{\text{gas}}.
$$

The clean spark spread is

$$
CSS =
P_{\text{power}}
- hP_{\text{gas}}
- eP_{\text{CO2}}
- VOM.
$$

## Black-76

For forward $F$, strike $K$, expiry $T$, volatility $\sigma$, and discount
factor $D(0,T)$,

$$
C =
D(0,T)
\left[
F N(d_1) - K N(d_2)
\right],
$$

$$
P =
D(0,T)
\left[
K N(-d_2) - F N(-d_1)
\right],
$$

where

$$
d_1 =
\frac{
\ln(F/K) + \frac{1}{2}\sigma^2 T
}{
\sigma \sqrt{T}
},
\qquad
d_2 = d_1 - \sigma \sqrt{T}.
$$

Vega under this convention is

$$
\text{Vega}
=
D(0,T) F \phi(d_1)\sqrt{T}.
$$

## Bachelier

For normal volatility $\sigma_N$,

$$
C =
D(0,T)
\left[
(F-K)N(d)
+ \sigma_N \sqrt{T}\phi(d)
\right],
$$

where

$$
d =
\frac{F-K}{\sigma_N \sqrt{T}}.
$$

The corresponding put follows from put-call parity on the forward.

## Put-Call Parity

For a non-dividend-paying equity under textbook assumptions,

$$
C - P = S_0 - K e^{-rT}.
$$

For options on forwards,

$$
C - P = D(0,T)(F-K).
$$

Parity is primarily a consistency condition on forwards, discounting, strike,
expiry, and settlement convention.

## Local Risk Expansion

For pricing function $PV(x)$ depending on market factors $x_i$,

$$
\Delta PV
\approx
\sum_i
\frac{\partial PV}{\partial x_i}\Delta x_i
+
\frac{1}{2}
\sum_i
\frac{\partial^2 PV}{\partial x_i^2}
(\Delta x_i)^2
+
\sum_{i<j}
\frac{\partial^2 PV}{\partial x_i \partial x_j}
\Delta x_i \Delta x_j.
$$

The expansion defines local delta, gamma, and cross-gamma terms. Large moves,
barriers, exercise decisions, discontinuities, and curve reconstruction effects
usually require full revaluation.

## PnL Explain

A practical PnL explain decomposition is

$$
\Delta PV
=
\text{Carry}
+ \text{RollDown}
+ \text{MarketMove}
+ \text{Cash}
+ \text{TradeActivity}
+ \text{FXTranslation}
+ \text{ModelConfigChange}
+ \text{Residual}.
$$

The reconciliation condition is

$$
\Delta PV_{\text{actual}}
-
\sum_j \Delta PV_j
=
\text{Residual}.
$$

The residual should be reported and monitored rather than hidden.

## VaR And Expected Shortfall

For portfolio loss $L$ and confidence level $\alpha$,

$$
VaR_{\alpha}
=
\inf\{l : P(L \leq l) \geq \alpha\}.
$$

Expected Shortfall is

$$
ES_{\alpha}
=
\mathbb{E}[L \mid L \geq VaR_{\alpha}],
$$

with the empirical implementation depending on path ordering, interpolation,
tail convention, and sign convention.

## LSMC Continuation Value

At exercise date $t_m$, Least Squares Monte Carlo approximates continuation
value with basis functions $\phi_k$:

$$
C(t_m, X_{t_m})
\approx
\sum_{k=1}^{K}
\beta_k \phi_k(X_{t_m}).
$$

The exercise rule for payoff $H(t_m, X_{t_m})$ is

$$
\text{exercise at } t_m
\quad \text{if} \quad
H(t_m, X_{t_m}) \geq C(t_m, X_{t_m}).
$$

The implementation must control seed reproducibility, basis selection,
regression sample filtering, path state, discounting, and exercise-policy
serialization.
