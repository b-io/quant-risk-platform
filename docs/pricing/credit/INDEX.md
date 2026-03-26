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
