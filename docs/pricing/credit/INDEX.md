# Credit Pricing and Credit Curves

This section covers credit-specific market objects and pricing inputs.

Credit should be read after rates. The reason is structural: credit valuation typically reuses discounting machinery from rates and adds survival modeling, recovery assumptions, and default-event semantics.

## Canonical files

- `docs/pricing/credit/BOND_SPREADS_AND_DEFAULT_RISK.md`
- `docs/pricing/credit/CREDIT_CURVE_CONSTRUCTION.md`
- `docs/pricing/credit/CDS_CURVES_AND_CREDIT_RISK_IN_PRACTICE.md`

## Recommended study order

1. `docs/pricing/credit/BOND_SPREADS_AND_DEFAULT_RISK.md`  
   Start here to separate yield, spread, Z-spread, asset-swap spread, CDS spread, and hazard ideas.

2. `docs/pricing/credit/CREDIT_CURVE_CONSTRUCTION.md`  
   Continue with the mapping from market spreads to hazard or survival curves.

3. `docs/pricing/credit/CDS_CURVES_AND_CREDIT_RISK_IN_PRACTICE.md`  
   Finish with the actual analytics implications: premium leg, protection leg, CS01, basis, and risk usage.

## Core credit formulas

If $Q(t,T)$ is the survival probability to $T$, then under a hazard-rate view

$$
Q(t,T) = \exp\left(-\int_t^T \lambda(u)\,du\right)
$$

where $\lambda(u)$ is the hazard rate.

A reduced-form view of a defaultable cashflow often combines discounting and survival weighting:

$$
PV \approx \sum_{i=1}^{n} \mathbb{E}\left[ CF_i \, D(t,T_i) \, \mathbf{1}_{\{\tau > T_i\}} \right]
$$

This makes the main conceptual point clear:

- discounting handles the time value of money,
- survival handles the probability that the promised cashflow is still alive.

## What this section should clarify

By the end of this section, the reader should be able to explain:

- why a bond spread is not automatically a hazard rate,
- why credit discounting and survival are distinct objects,
- how recovery assumptions affect calibration,
- how CS01 and jump-to-default differ from rates risk,
- why credit data needs entity identity and convention metadata, not just numeric spreads.
