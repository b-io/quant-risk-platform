# Credit Pricing and Credit Curves

This section covers credit-specific market objects and pricing inputs.

## Contents

- `docs/pricing/credit/CDS_CURVES_AND_CREDIT_RISK_IN_PRACTICE.md` — CDS intuition, premium and protection legs,
  calibration logic, CS01, and practical credit-risk usage.
- `docs/pricing/credit/CREDIT_CURVE_CONSTRUCTION.md` — credit spread curves, hazard-rate calibration, survival
  probabilities, recovery assumptions, and builder design.

## Scope

Credit is intentionally separated from rates because it introduces distinct concepts:

- reference entities and obligations,
- CDS and bond-spread market conventions,
- survival modeling and recovery assumptions,
- CS01 and credit-factor mapping.

## What to understand from this section

After reading this section, the reader should have a clear picture of:

- the difference between discounting and survival modeling,
- the market inputs needed for credit-curve construction,
- the outputs a reusable credit curve should expose,
- how credit factors connect to CS01, stress, P&L explain, and scenario design.
