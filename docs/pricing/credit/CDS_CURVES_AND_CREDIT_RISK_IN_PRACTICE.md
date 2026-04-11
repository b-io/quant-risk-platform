# CDS Curves and Credit Risk in Practice

This chapter explains how CDS spreads map to survival curves, how recovery enters calibration, and how a production platform should expose credit analytics for pricing, CS01, stress, and P&L explain.

## Notation used in this chapter

Unless stated otherwise:

- $t=0$ is the valuation date.
- $T$ is a maturity in years from today.
- $T_i$ is the $i$-th CDS payment date.
- $N$ is CDS notional.
- $s$ is the quoted CDS spread.
- $D(0,T)$ is the discount factor from today to maturity $T$.
- $Q(0,T)$ is the survival probability to time $T$.
- $h(t)$ is the hazard rate or default intensity at time $t$.
- $R$ is the recovery rate.
- $LGD = 1-R$ is loss given default.
- $\alpha_i$ is the accrual year fraction for premium period $[T_{i-1},T_i]$.

## 1. What a CDS spread represents

A CDS spread is the premium paid by the protection buyer to insure the reference obligation against default. The contract exchanges:

- a premium leg that is paid until maturity or default,
- a protection leg that pays after a credit event,
- a recovery assumption that determines the loss given default.

Let $PV_{premium}(s)$ be the present value of the premium leg as a function of spread $s$, and let $PV_{protection}$ be the present value of the protection leg. At par spread the contract satisfies

$$
PV_{premium}(s)=PV_{protection}
$$

Where:

- $PV_{premium}(s)$ is the premium-leg present value.
- $PV_{protection}$ is the protection-leg present value.
- $s$ is the quoted running spread.

## 2. Survival probability and default density

Let $Q(0,T)$ be the probability that the reference entity survives from today to time $T$. In an intensity model with hazard rate $h(t)$,

$$
Q(0,T)=\exp\left(-\int_0^T h(t)\,dt\right)
$$

Where:

- $Q(0,T)$ is survival probability to time $T$.
- $h(t)$ is the hazard rate at time $t$.
- $t$ is the integration variable representing time.
- $T$ is the maturity horizon.

The cumulative default probability by time $T$ is

$$
1-Q(0,T)
$$

Let $\tau$ be the random default time. For a short interval $[t,t+dt]$ the conditional default probability is approximately

$$
\mathbb{P}(\tau\in[t,t+dt]\mid\tau>t)\approx h(t)\,dt
$$

Where:

- $\tau$ is default time.
- $dt$ is a small time increment.

A useful density-style identity is

$$
d\mathbb{P}(\tau\le t)\approx h(t)Q(0,t)\,dt
$$

Where:

- $d\mathbb{P}(\tau\le t)$ is the incremental default probability around time $t$.

### Example: constant hazard rate

Let the hazard rate be constant at $h=2\%$. Then

$$
Q(0,3)=e^{-0.02\times 3}=0.941765
$$

So the default probability by year 3 is

$$
1-Q(0,3)=0.058235
$$

## 3. Premium and protection legs

Let the premium leg be approximated by scheduled coupon payments only. Then a common schematic formula is

$$
PV_{premium}(s)\approx N s \sum_{i=1}^{n}\alpha_i D(0,T_i)Q(0,T_i)
$$

Where:

- $n$ is the number of premium-payment dates.
- $\alpha_i$ is the accrual fraction of period $i$.
- $D(0,T_i)$ is the discount factor to payment date $T_i$.
- $Q(0,T_i)$ is survival probability to payment date $T_i$.

Let the protection leg be approximated on the same tenor grid. Then

$$
PV_{protection}\approx N(1-R)\sum_{i=1}^{n} D(0,T_i)\left(Q(0,T_{i-1})-Q(0,T_i)\right)
$$

Where:

- $1-R$ is loss given default.
- $Q(0,T_{i-1})-Q(0,T_i)$ is the default probability over interval $[T_{i-1},T_i]$.

In production pricing, accrual-on-default and exact default-time integration are included. The formulas above are still useful because they make the structure transparent:

- discounting comes from the rates curve,
- default timing comes from the survival curve,
- loss severity comes from recovery.

## 4. Back-of-the-envelope spread intuition

Let the hazard rate be roughly constant and ignore discounting detail. Then a useful approximation is

$$
s\approx h(1-R)
$$

Where:

- $s$ is the CDS spread.
- $h$ is the flat hazard rate.
- $1-R$ is loss given default.

This is not a production formula, but it provides immediate intuition:

- higher hazard implies a wider spread,
- lower recovery implies a wider spread.

### Example: hazard from spread

If $s=150$ bp and $R=40\%$, then

$$
h\approx \frac{0.015}{0.60}=0.025
$$

So the implied flat hazard rate is about $2.5\%$ per year.

## 5. Calibration logic

A CDS curve builder typically solves for hazard or survival nodes so that each market CDS quote is repriced. In a piecewise-constant hazard model, the unknowns are hazard levels on successive maturity intervals.

Let $s_k^{mkt}$ be the market spread for maturity $T_k$. Let $h=(h_1,\ldots,h_m)$ be the hazard-parameter vector, and let $s_k^{model}(h)$ be the model-implied spread produced by $h$. A common calibration target is

$$
s_k^{model}(h)=s_k^{mkt}
$$

Where:

- $h=(h_1,\ldots,h_m)$ is the vector of hazard parameters.
- $s_k^{mkt}$ is the observed market quote at tenor $k$.
- $s_k^{model}(h)$ is the model-implied quote.

If exact repricing is not possible or if smoothing is required, the objective may instead minimize weighted quote errors.

## 6. CS01 and credit-risk attribution

Let $V(s)$ be the present value of an instrument as a function of the shocked credit spread input $s$. A parallel one-basis-point CS01 is approximated by

$$
CS01\approx \frac{V(s-1\text{ bp})-V(s+1\text{ bp})}{2}
$$

Where:

- $V(s\pm 1\text{ bp})$ is the repriced value after a one-basis-point spread bump.
- $CS01$ is the change in value for a one-basis-point widening in spread under the chosen sign convention.

A bucketed CS01 shocks only one maturity node at a time. Let $s=(s_1,\ldots,s_m)$ be the spread vector and let $e_k$ be the unit vector for bucket $k$. Then

$$
CS01_k\approx \frac{V(s-1\text{ bp}\,e_k)-V(s+1\text{ bp}\,e_k)}{2}
$$

Where:

- $CS01_k$ is sensitivity to tenor bucket $k$.
- $s=(s_1,\ldots,s_m)$ is the spread vector across tenor buckets.
- $e_k$ is the unit vector that selects only bucket $k$.
- $1\text{ bp}$ is the one-basis-point shock size.

### Example: parallel CS01

Suppose a position is worth 2.40. After a +1 bp spread bump it is worth 2.48. After a -1 bp spread bump it is worth 2.32. Then

$$
CS01\approx \frac{2.32-2.48}{2}=-0.08
$$

Under this sign convention a spread widening loses 0.08 of value.

## 7. Bond spread versus CDS spread

Bond spreads and CDS spreads are related but not identical.

- A cash-bond spread reflects the bond's coupon, maturity, financing, liquidity, and benchmark choice.
- A CDS spread reflects the price of protection under explicit recovery and contract conventions.
- The bond-CDS basis captures the difference between the two representations.

The cleanest interpretation is:

- bond spread is a cash-market relative-value measure,
- CDS spread is a derivative-market protection measure.

## 8. What a reusable credit curve should expose

A production credit curve object should expose at least:

- survival probability $Q(0,T)$,
- cumulative default probability $1-Q(0,T)$,
- hazard rate $h(t)$ or piecewise hazard nodes,
- calibration diagnostics,
- factor identifiers for spread and CS01 aggregation,
- enough metadata to distinguish issuer, seniority, currency, restructuring clause, and recovery assumption.

## 9. Practical implementation notes

The credit stack should preserve the following separation:

1. normalized quote records,
2. convention resolution,
3. discount-curve linkage,
4. credit-curve calibration,
5. pricing and risk services,
6. archival lineage.

That separation is what allows the same calibrated object to feed pricing, CS01, historical stress, VaR, and P&L explain without re-implementing credit logic in each report.
