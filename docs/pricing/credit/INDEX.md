# Credit Pricing and Credit Curves

This section covers credit-specific market objects and pricing inputs.

## Contents

- `docs/pricing/credit/CREDIT_CURVE_CONSTRUCTION.md` — spread curves, hazard-rate curves, survival probabilities,
  recovery assumptions, and the intended `CreditCurveBuilder` path.

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
- how credit factors connect to CS01, stress, and P&L explain.
