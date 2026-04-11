# Rates Pricing and Curve Construction

This section covers rates-specific curve construction, curve usage, dynamic term-structure modeling, and model selection.

Rates documentation should be read in two passes:

- first as **static curve construction and valuation**,
- then as **dynamic modeling and factor interpretation**.

## Canonical files

- `docs/pricing/rates/YIELD_CURVE_AND_OIS_CONSTRUCTION.md`
- `docs/pricing/rates/YIELD_CURVE_WORKED_EXAMPLE.md`
- `docs/pricing/rates/DETAILED_YIELD_CURVE_IMPLEMENTATION.md`
- `docs/pricing/rates/RATES_FACTORS_AND_CURVE_MODELS.md`
- `docs/pricing/rates/TIME_DEPENDENT_VOLATILITY_AND_MEAN_REVERSION_MODELS.md`

## Support assets

- `docs/pricing/rates/assets/curve_objects_relationships.png`
- `docs/pricing/rates/assets/interpolation_tradeoffs.png`

## Recommended study order

1. `docs/pricing/rates/YIELD_CURVE_AND_OIS_CONSTRUCTION.md`  
   Learn discounting, projection, helpers, and conventions.

2. `docs/pricing/rates/YIELD_CURVE_WORKED_EXAMPLE.md`  
   Use this to verify that the formulas and curve semantics are intuitive.

3. `docs/pricing/rates/DETAILED_YIELD_CURVE_IMPLEMENTATION.md`  
   This is where curve objects, handles, interpolation, and implementation trade-offs become concrete.

4. `docs/pricing/rates/RATES_FACTORS_AND_CURVE_MODELS.md`  
   Once the static term structure is clear, place it in the wider modeling landscape.

5. `docs/pricing/rates/TIME_DEPENDENT_VOLATILITY_AND_MEAN_REVERSION_MODELS.md`  
   Finish with the model families used for dynamics and scenario generation.

## Core rates formulas

Discounting is the base layer:

$$
PV = \sum_{i=1}^{n} CF_i \, D(t,T_i)
$$

A continuously compounded zero rate $z(t,T)$ and a discount factor are related by

$$
D(t,T) = e^{-z(t,T)(T-t)}
$$

The forward rate implied by discount factors is

$$
F(t;T_1,T_2) = \frac{1}{\tau(T_1,T_2)}\left(\frac{D(t,T_1)}{D(t,T_2)} - 1\right)
$$

These equations matter operationally because a rates curve object should support the queries that appear in them: discount factors, zero rates, and forwards.

## What this section should make clear

By the end of this section, the reader should understand:

- why OIS discount curves and forward curves should usually be separated,
- how market conventions affect curve helpers and pricing outputs,
- why interpolation choices change both valuation smoothness and risk behavior,
- how a rates curve becomes a reusable dependency for valuation, explain, and scenario analysis,
- where static curve building ends and stochastic rate modeling begins.
