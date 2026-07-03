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

## Product Coverage Sequence

The credit phase in `docs/implementation/PHASED_BUILD_PLAN.md` should be
implemented in this order:

1. credit bonds with spread discounting;
2. single-name CDS;
3. CDS indices;
4. credit curve bootstrapping from CDS spreads;
5. bond CS01 and spread duration;
6. CDS options and credit index options;
7. CDO and tranche products only after CDS and index infrastructure are stable.

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
