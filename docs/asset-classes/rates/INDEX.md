# Rates Documentation Index

This section covers rates-specific curve construction, curve usage, products,
dynamic term-structure modeling, and implementation choices.

Rates should be read before most other asset classes because discount curves and
projection curves are platform-wide dependencies.

## Canonical Files

- `docs/asset-classes/rates/YIELD_CURVE_AND_OIS_CONSTRUCTION.md`
- `docs/asset-classes/rates/YIELD_CURVE_WORKED_EXAMPLE.md`
- `docs/asset-classes/rates/DETAILED_YIELD_CURVE_IMPLEMENTATION.md`
- `docs/asset-classes/rates/RATES_FACTORS_AND_CURVE_MODELS.md`
- `docs/asset-classes/rates/TIME_DEPENDENT_VOLATILITY_AND_MEAN_REVERSION_MODELS.md`

## Support Assets

- `docs/asset-classes/rates/assets/curve_objects_relationships.png`
- `docs/asset-classes/rates/assets/interpolation_tradeoffs.png`

## Recommended Reading Order

1. `docs/asset-classes/rates/YIELD_CURVE_AND_OIS_CONSTRUCTION.md`  
   Discounting, projection, helpers, and curve conventions.

2. `docs/asset-classes/rates/YIELD_CURVE_WORKED_EXAMPLE.md`  
   A concrete rates curve example that connects notation to curve semantics.

3. `docs/asset-classes/rates/DETAILED_YIELD_CURVE_IMPLEMENTATION.md`  
   Curve objects, handles, interpolation, extrapolation, and implementation
   trade-offs.

4. `docs/asset-classes/rates/RATES_FACTORS_AND_CURVE_MODELS.md`  
   Static and dynamic rate-factor models.

5. `docs/asset-classes/rates/TIME_DEPENDENT_VOLATILITY_AND_MEAN_REVERSION_MODELS.md`  
   Time-dependent volatility, mean reversion, calibration, and scenario
   generation.

## Product Coverage Sequence

The rates phase in `docs/implementation/PHASED_BUILD_PLAN.md` should be
implemented in this order:

1. cash and deposits;
2. FRAs;
3. interest-rate futures;
4. vanilla fixed-float swaps;
5. OIS swaps;
6. fixed-rate bonds;
7. floating-rate notes;
8. caps and floors;
9. European swaptions;
10. Bermudan swaptions using LSMC.

## Shared References

- `docs/reference/LEXIS.md` defines rates terms and product vocabulary.
- `docs/reference/FORMULAS.md` contains shared discounting, forward-rate,
  par-swap, bond, option, and risk formulas.
- `docs/reference/SOURCES.md` lists public rates and QuantLib references.

## Maintenance Rule

Rates chapters should keep curve identity, index tenor, day-count convention,
calendar, interpolation, extrapolation, collateral assumption, and projection
logic explicit. These assumptions should be visible in both market-state
construction and pricing diagnostics.
