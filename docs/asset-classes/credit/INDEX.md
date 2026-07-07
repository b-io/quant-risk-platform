# Credit Documentation Index

This section covers credit-specific market objects, product conventions, curve
construction, and default-risk interpretation.

Credit should be read after rates because credit valuation usually reuses rates
discounting and adds survival modeling, recovery assumptions, and default-event
semantics.

## Canonical Files

- `docs/asset-classes/credit/BOND_SPREADS_AND_DEFAULT_RISK.md`
- `docs/asset-classes/credit/CREDIT_CURVE_CONSTRUCTION.md`
- `docs/asset-classes/credit/CDS_CURVES_AND_CREDIT_RISK_IN_PRACTICE.md`

## Recommended Reading Order

1. `docs/asset-classes/credit/BOND_SPREADS_AND_DEFAULT_RISK.md`  
   Yield, spread, Z-spread, asset-swap spread, CDS spread, recovery, and default
   risk.

2. `docs/asset-classes/credit/CREDIT_CURVE_CONSTRUCTION.md`  
   Mapping market spreads into hazard or survival curves.

3. `docs/asset-classes/credit/CDS_CURVES_AND_CREDIT_RISK_IN_PRACTICE.md`  
   Premium leg, protection leg, calibration logic, CS01, basis, and analytics
   usage.

## Product Coverage

Credit product coverage is organized around these product families:

1. credit bonds with spread discounting;
2. single-name CDS;
3. CDS indices;
4. credit curve bootstrapping from CDS spreads;
5. bond CS01 and spread duration;
6. CDS options and credit index options;
7. CDO and tranche products only after CDS and index infrastructure are stable.

## Implementation Scope

Product support is implemented for the initial credit stack:

- `CreditBondTrade` prices fixed coupon cashflows with risk-free discounting and
  issuer spread discounting.
- `CdsTrade` and `CdsIndexTrade` price premium and protection legs from
  spread-implied hazard curves and recovery quotes.
- `CdsOptionTrade` and `CreditIndexOptionTrade` price European spread options
  with Black-style spread volatility inputs.
- `RiskResult` exposes CS01 and spread duration, with bucketed credit spread
  risk driven by canonical credit factor bindings.

The first implementation uses deterministic spread and recovery inputs from the
market snapshot. When multiple same-underlier CDS or credit-spread quotes are
available, the pricers interpolate live quote handles into a simple spread or
hazard term structure. CDO, tranche, counterparty-risk, and detailed
accrual-on-default production conventions remain outside the current product
scope.

## Shared References

- `docs/reference/LEXIS.md` defines issuer, reference entity, credit spread,
  hazard rate, survival curve, recovery rate, CDS, and CS01.
- `docs/reference/FORMULAS.md` contains shared credit survival and recovery
  formulas.
- `docs/reference/SOURCES.md` lists public credit convention and modeling
  references.

## Maintenance Rule

Credit chapters should keep issuer identity, reference entity, seniority,
recovery assumption, default event, accrual-on-default convention, calibration
instrument, and curve source explicit.
