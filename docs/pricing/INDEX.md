# Pricing Documentation Index

This section contains the market-construction and pricing-facing documentation.

The pricing notes should be read as a layered system rather than as isolated chapters. The natural dependency is:

$$
\text{market data} \rightarrow \text{curves and surfaces} \rightarrow \text{instrument valuation} \rightarrow \text{risk sensitivities}
$$

A pricing platform is coherent only if this dependency is respected in both the documentation and the code.

## Section structure

- `docs/pricing/market-data/` — normalized market schema, factor identities, and reusable market objects.
- `docs/pricing/rates/` — discount curves, projection curves, rate-model choices, and term-structure implementation.
- `docs/pricing/credit/` — spread measures, survival curves, CDS logic, and default-risk interpretation.
- `docs/pricing/volatility/` — option foundations, Greeks, surfaces, and volatility-model trade-offs.

## Recommended study order

### Phase 1 — start from market state

1. `docs/pricing/market-data/MULTI_ASSET_MARKET_DATA_AND_CURVES.md`  
   Read this first. All pricing chapters assume the existence of a typed and reusable market state.

### Phase 2 — build rates intuition and implementation

2. `docs/pricing/rates/YIELD_CURVE_AND_OIS_CONSTRUCTION.md`  
   This is the core curve-construction chapter.

3. `docs/pricing/rates/YIELD_CURVE_WORKED_EXAMPLE.md`  
   Read this immediately after the construction chapter to make the curve formulas concrete.

4. `docs/pricing/rates/DETAILED_YIELD_CURVE_IMPLEMENTATION.md`  
   This chapter explains implementation details such as interpolation, extrapolation, and object wiring.

5. `docs/pricing/rates/RATES_FACTORS_AND_CURVE_MODELS.md`  
   Read this once the static curve layer is clear.

6. `docs/pricing/rates/TIME_DEPENDENT_VOLATILITY_AND_MEAN_REVERSION_MODELS.md`  
   This is the deeper model-trade-off chapter.

### Phase 3 — extend the same logic to credit

7. `docs/pricing/credit/BOND_SPREADS_AND_DEFAULT_RISK.md`  
   Read this first because credit language is often overloaded.

8. `docs/pricing/credit/CREDIT_CURVE_CONSTRUCTION.md`  
   This is the bridge from spread observations to survival curves.

9. `docs/pricing/credit/CDS_CURVES_AND_CREDIT_RISK_IN_PRACTICE.md`  
   This connects calibrated curves to actual analytics outputs and desk-facing risk language.

### Phase 4 — move to volatility and nonlinear pricing

10. `docs/pricing/volatility/OPTION_PRICING_FOUNDATIONS_AND_PUT_CALL_PARITY.md`
11. `docs/pricing/volatility/OPTION_PRICING_AND_EXERCISE_STYLES.md`
12. `docs/pricing/volatility/GREEKS_AND_NONLINEAR_RISK.md`
13. `docs/pricing/volatility/VOLATILITY_SURFACES.md`
14. `docs/pricing/volatility/VOLATILITY_MODELS_AND_CALIBRATION_TRADEOFFS.md`

The volatility section is last because it assumes comfort with discounting, forwards, carry, and factor mapping.

## Core pricing identities

The rates layer revolves around discounting and forwards:

$$
PV = \sum_{i=1}^{n} CF_i \, D(t,T_i)
$$

$$
F(t;T_1,T_2) = \frac{1}{\tau(T_1,T_2)}\left(\frac{D(t,T_1)}{D(t,T_2)} - 1\right)
$$

The credit layer adds survival weighting:

$$
PV_{\text{credit}} = \mathbb{E}\left[ D(t,\tau) \, \text{payoff}(\tau) \right]
$$

with default timing $\tau$ governed by survival and hazard assumptions.

The volatility layer introduces nonlinear payoffs, so first-order sensitivities become only local approximations:

$$
\Delta PV \approx \sum_k \frac{\partial PV}{\partial x_k}\Delta x_k
$$

and second-order terms such as gamma and cross-gamma start to matter.

## What to understand before moving on to risk

By the end of this section, the reader should be able to explain:

- how raw quotes become typed market inputs,
- why discounting and projection are separate,
- why credit introduces survival modeling rather than just another spread column,
- why option risk cannot be reduced to purely linear factor shocks,
- how reusable curves and surfaces connect pricing and risk.

## Maintenance rule

When adding a new pricing chapter, place it in the most specific subfolder possible and update this index so the study order remains explicit.
