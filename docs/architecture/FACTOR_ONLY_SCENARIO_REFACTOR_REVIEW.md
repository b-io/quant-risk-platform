# Factor-Only Scenario Architecture

## Target Model

Scenarios are represented as factor definitions, factor bindings, and factor shocks. The platform should have one canonical
shock path across stress testing, Monte Carlo, P&L explain, and deterministic revaluation.

```text
scenario.factor_shocks
    -> factor resolver
    -> quote-level shocked values
    -> market state update
    -> curve and instrument revaluation
```

This keeps the economic source of truth explicit: factors describe what moved, bindings describe where the move applies,
and shocked market states describe what is priced.

## Canonical Identifiers

Factor identifiers use the `RF:` prefix and uppercase factor families. Examples:

- `RF:COM:WTI:FWD:3M`
- `RF:COM:WTI:SPOT`
- `RF:COMVOL:TTF:1Y:ATM`
- `RF:CREDIT:CDX_IG:SPREAD:5Y`
- `RF:CREDIT:CDX_IG:RECOVERY:SPOT`
- `RF:CREDITVOL:ITRAXX_MAIN:6M:100`
- `RF:EQ:AAPL:SPOT`
- `RF:EQ:AAPL:DIVYLD:1Y`
- `RF:EQ:SPX:FUT:6M`
- `RF:EQVOL:SPX:3M:ATM`
- `RF:FX:EURUSD:SPOT`
- `RF:FX:EURUSD:FWDPTS_6M`
- `RF:FXVOL:EURUSD:1M:25D_RR`
- `RF:RATES:USD:OIS:5Y`
- `RF:RATESVOL:USD:SWAPTION:1Y:5Y`

New factor families should be added through the canonical factor helpers and tests before they are used in scenario JSON,
Python examples, or analytics services.

## Scenario Application Rules

- Apply scenario shocks through factor bindings only.
- Reject scenario factors that have no binding.
- Reject bindings that target missing market quotes.
- Use shock measures consistently: absolute, basis points, log return, relative, and vol points.
- Keep historical and hypothetical stress scenarios as factor vectors, not special-case runtime semantics.

## Analytics Integration

All risk services should consume the same shocked market states:

- stress testing revalues the portfolio under named factor scenarios;
- P&L explain revalues sequential market states by factor bucket;
- Monte Carlo generates factor paths and resolves them through the same binding layer;
- VaR and ES attribution aggregate results by factor, trade, book, currency, and asset class.

## Acceptance Criteria

- Active scenario JSON uses canonical factor identifiers only.
- C++ tests reject malformed or non-canonical factor identifiers.
- Python demos exercise all supported factor families through the same scenario model.
- Script and documentation examples use the same canonical parameter names across PowerShell and Bash.
