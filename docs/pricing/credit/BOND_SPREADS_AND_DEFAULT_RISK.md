# Bond Spreads, CDS Spreads, and Default-Risk Measures

## Why this chapter matters

Credit products sit at the intersection of cash-bond mathematics, relative-value spread measures, and reduced-form
default modeling. A production platform must keep these layers distinct:

- cash-bond pricing and yield conventions,
- relative-value spread measures such as credit spread, Z-spread, and asset-swap spread,
- protection pricing through CDS spreads, hazard rates, survival probabilities, and recovery assumptions,
- risk metrics such as CS01 and jump-to-default exposure.

This chapter explains the static concepts that are needed before any dynamic credit simulation or portfolio aggregation is
discussed.

## Notation used in this chapter

Unless stated otherwise:

- $t=0$ is the valuation date.
- $T_i$ is the $i$-th cashflow date.
- $CF_i$ is the deterministic contractual cashflow at $T_i$.
- $D(0,T_i)$ is the discount factor from today to $T_i$.
- $P$ is the clean price unless explicitly stated otherwise.
- $AI$ is accrued interest and $P^{dirty}=P^{clean}+AI$.
- $y$ is yield-to-maturity under the relevant compounding convention.
- $s$ denotes a generic spread.
- $Q(0,T)$ is survival probability to time $T$.
- $\lambda(t)$ is the hazard rate or default intensity.
- $R$ is the recovery rate and $LGD=1-R$ is loss given default.
- $s_{CDS}$ is the quoted CDS premium.

## 1. Bond price, yield, and spread

### 1.1 Present-value view of a fixed-rate bond

A fixed-rate bond with coupons $C_i$ and principal $N$ at maturity $T_n$ has dirty price
$$
P^{dirty} = \sum_{i=1}^{n} C_i D(0,T_i) + N D(0,T_n)
$$
If the bond is quoted clean, then
$$
P^{dirty} = P^{clean} + AI
$$
The clean price removes accrued coupon so that market moves are easier to compare across dates.

### 1.2 Yield-to-maturity

Yield-to-maturity compresses the entire term structure into a single internal rate $y$ solving
$$
P^{dirty} = \sum_{i=1}^{n} \frac{C_i}{(1+y/m)^{mT_i}} + \frac{N}{(1+y/m)^{mT_n}}
$$
under a stated compounding frequency $m$. The yield is a convenient quoting convention, but it is not a primitive market
object. Two bonds with different coupons, maturities, and optionality can have the same yield while having different risk
profiles.

### 1.3 What practitioners mean by spread

A spread is the excess yield or excess discounting required relative to a benchmark curve. A very simple benchmark-relative
definition is
$$
\text{credit spread} \approx y_{bond} - y_{benchmark}
$$
This approximation is easy to communicate but can be misleading when coupon structures differ materially from the benchmark.
More robust spread measures use the full benchmark zero curve, not a single benchmark yield.

## 2. Credit spread, Z-spread, and asset-swap spread

### 2.1 Credit spread in the simplest sense

For a plain cash bond, a desk may refer informally to the bond spread as the difference between the bond yield and a
benchmark government or swap yield. This is useful for quick market commentary, but it is not stable across coupons or
cashflow profiles.

### 2.2 Z-spread

The Z-spread is the constant spread $z$ added to every point of a benchmark zero curve such that the benchmark-discounted
cashflows reproduce the observed bond price:
$$
P^{dirty} = \sum_{i=1}^{n} CF_i e^{-(r_i+z)T_i}
$$
where $r_i$ is the benchmark zero rate for maturity $T_i$. The Z-spread is therefore a full-curve relative-value
measure. It is widely used because it is easy to compute and compare across bonds.

The conceptual limitation is important: Z-spread treats the bond as if each contractual cashflow simply received an equal
spread over the benchmark curve. It does not explicitly model stochastic default timing or recovery. As a result, it is
useful as a relative-value measure but not a full reduced-form default model.

### 2.3 Asset-swap spread

The asset-swap spread answers a different question. It asks what spread over a floating benchmark is earned if the bond
is purchased and its fixed coupons are swapped into floating cashflows. In practical terms, the asset-swap spread is the
fixed spread $s_{ASW}$ satisfying
$$
P^{dirty} + PV_{swap\ package}(s_{ASW}) = \text{par package value}
$$
The exact formula depends on the market convention, clean-versus-dirty treatment, whether the package is par/par or
market/par, and the floating index used. The important intuition is:

- Z-spread is a constant spread over the benchmark zero curve that prices bond cashflows.
- Asset-swap spread is the spread earned on a synthetic floating package made from the bond plus an interest-rate swap.

Because the two measures answer different questions, they can differ materially.

### 2.4 Spread widening versus tightening

For a long cash bond position, spread widening generally lowers price and spread tightening generally raises price. A
first-order linearized approximation is
$$
\Delta P \approx -CS01 \times \Delta s_{bp}
$$
where $\Delta s_{bp}$ is the spread move in basis points and $CS01$ is the PV sensitivity per basis point. This local
approximation is useful for small spread moves, but it becomes unreliable when the bond is deeply distressed, near
maturity, or strongly exposed to jump-to-default risk.

## 3. CDS spread, hazard rate, survival probability, and recovery

### 3.1 CDS premium and protection legs

A CDS is a protection contract. The protection buyer pays a periodic premium until maturity or default, and receives a
protection payment if default occurs. At inception, the quoted CDS spread makes premium-leg PV equal protection-leg PV:
$$
PV_{premium}(s_{CDS}) = PV_{protection}
$$
In discretized form with payment dates $T_i$, accrual fractions $\alpha_i$, and notional $N$, the premium leg is often
written schematically as
$$
PV_{premium} = N s_{CDS} \sum_{i=1}^{n} \alpha_i D(0,T_i) Q(0,T_i)
$$
up to accrual-on-default adjustments, while the protection leg is approximately
$$
PV_{protection} = N(1-R) \sum_{i=1}^{n} D(0,T_i) \left(Q(0,T_{i-1}) - Q(0,T_i)\right)
$$
These formulas show the key structural inputs:

- a discount curve,
- a survival curve,
- a recovery assumption.

### 3.2 Hazard rate and survival probability

The hazard rate $\lambda(t)$ is the instantaneous default intensity conditional on survival up to time $t$. Survival
probability satisfies
$$
Q(0,T)=\exp\left(-\int_0^T \lambda(u)\,du\right)
$$
Under a constant hazard rate $\lambda$, this simplifies to
$$
Q(0,T)=e^{-\lambda T}
$$
and the cumulative default probability by time $T$ is
$$
P(\tau \le T)=1-Q(0,T)
$$
The hazard rate is not itself a probability. Rather, for a short interval $[t,t+\Delta t]$,
$$
P(\tau \in [t,t+\Delta t] \mid \tau>t) \approx \lambda(t)\Delta t
$$
This conditional-intensity view is what makes reduced-form credit models tractable.

### 3.3 Recovery rate

Recovery rate is the fraction of notional recovered after default. Loss given default is
$$
LGD = 1-R
$$
Lower recovery mechanically raises the protection-leg PV and therefore the CDS spread required to balance premium and
protection legs. Recovery is one of the most important modeling assumptions in reduced-form credit, especially when CDS
quotes are sparse and calibration becomes underdetermined.

### 3.4 A useful constant-intensity approximation

If one ignores discounting detail and accrual-on-default corrections, a common back-of-the-envelope approximation is
$$
s_{CDS} \approx \lambda (1-R)
$$
This is not a production pricing formula, but it is extremely useful for intuition. It shows immediately that a higher
hazard rate or lower recovery leads to a wider CDS spread.

## 4. Credit-curve calibration and outputs

### 4.1 What calibration solves for

A CDS curve builder uses quoted CDS spreads across maturities to infer a term structure of hazard rates or survival
probabilities. In a piecewise-constant hazard framework, the calibration solves for hazard nodes on successive tenor
intervals such that each quoted CDS spread is repriced.

### 4.2 What a reusable credit curve should expose

A production credit-curve object should expose at least:

- survival probability $Q(0,T)$,
- cumulative default probability $1-Q(0,T)$,
- hazard rate or hazard segment values,
- risky discounting diagnostics,
- quote-fit diagnostics,
- spread-sensitivity diagnostics such as CS01 by tenor or bucket.

### 4.3 Validation checks

A calibrated credit curve is not acceptable merely because the solver converged. Minimum checks include:

- repricing error against quoted CDS inputs,
- monotonic decreasing survival probabilities,
- non-negative or at least economically sensible hazard nodes,
- stable CS01 and scenario behavior under small quote perturbations,
- sensitivity to recovery assumptions,
- robustness to missing or stale tenors.

## 5. CS01 and jump-to-default

### 5.1 CS01

CS01 is the change in PV for a one-basis-point move in credit spread. A symmetric bump-and-reprice approximation is
$$
CS01 \approx \frac{PV(s-1bp)-PV(s+1bp)}{2}
$$
The precise definition depends on what is shocked:

- parallel shift of a quoted CDS spread curve,
- bucketed tenor shifts,
- issuer-specific spread moves,
- bond spread measures such as Z-spread or asset-swap spread.

A sound platform must document the shocked object explicitly, otherwise CS01 values cannot be reconciled across reports.

### 5.2 Jump-to-default

Spread risk measures small mark-to-market changes under spread moves. Jump-to-default represents the discrete loss if
the reference entity actually defaults. For a simplified exposure with notional $N$ and recovery $R$, the gross loss on
default is approximately
$$
JTD \approx N(1-R)
$$
possibly adjusted for accrued premium, bond price, or legal settlement mechanics. The key conceptual point is that
spread risk and default-event risk are related but not identical:

- spread widening is a continuous mark-to-market effect,
- default is a discrete jump event.

This distinction matters for portfolio stress design, concentration management, and hedging.

## 6. Bond-spread measures versus CDS spread

It is tempting to compare cash-bond and CDS spreads as if they were interchangeable. They are not.

- A bond spread embeds coupon structure, funding, and often liquidity differences.
- A CDS spread prices a protection contract with explicit default and recovery assumptions.
- Basis between cash and CDS can reflect funding, delivery option, counterparty, repo, and liquidity effects.

A robust analytics platform should therefore preserve both representations and allow reconciliation rather than forcing
one measure into the other.

## 7. Practical implementation notes

### 7.1 Canonical data required for cash credit and CDS analytics

At a minimum the system needs:

- issuer and reference-entity identifiers,
- currency and seniority,
- coupon and schedule metadata,
- benchmark curve linkage,
- recovery assumption,
- restructuring clause or convention where relevant,
- market quote timestamps and source lineage.

### 7.2 Why clean labeling matters

A portfolio report that says only “spread” is ambiguous. The report should state clearly whether the quantity is:

- government spread,
- swap spread,
- Z-spread,
- asset-swap spread,
- CDS spread,
- CS01 under a particular curve-shock definition.

### 7.3 Relationship to the rest of the documentation

- `docs/pricing/credit/CREDIT_CURVE_CONSTRUCTION.md` explains how reusable credit curves are built and wired into the
  platform.
- `docs/pricing/credit/CDS_CURVES_AND_CREDIT_RISK_IN_PRACTICE.md` explains CDS cashflows, calibration logic, and credit
  usage patterns.
- `docs/risk/RISK_MEASURES_AND_EXPLAIN.md` and `docs/risk/PNL_EXPLAIN_IN_PRACTICE.md` explain how these quantities feed
  into desk risk, explain, and scenario reporting.
