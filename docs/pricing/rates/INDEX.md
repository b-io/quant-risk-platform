# Rates Pricing and Curve Construction

This section covers rates-specific curve construction and pricing inputs.

## Contents

- `docs/pricing/rates/YIELD_CURVE_AND_OIS_CONSTRUCTION.md` — discount and projection curves, OIS discounting,
  bootstrap logic, conventions, and rates risk implications.

## Scope

The rates section is the current implementation focus of QRP. It should remain the canonical home for:

- OIS discounting by currency,
- IBOR or term projection curves,
- conventions used by deposits, FRAs, futures, and swaps,
- rates-specific bootstrap choices,
- rates risk diagnostics and scenario foundations.

## What to understand from this section

After reading this section, the reader should be able to explain:

- why discount and projection curves should be separated,
- how rates conventions affect helper construction and pricing,
- why curve objects must be reusable across valuation, risk, and stress,
- how rates curve design drives PV01, bucketed risk, and scenario semantics.
