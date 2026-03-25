# Quant Risk Platform Documentation

This `docs/` tree is the canonical documentation root for the repository.

## Structure

- `docs/design/` — architecture, module boundaries, market-and-curve design, and service-level design rationale.
- `docs/pricing/` — pricing and market-construction documentation: rates curves, credit curves, volatility surfaces, and market-data modeling.
- `docs/risk/` — deterministic and non-linear risk, P&L explain, scenarios, historical stress, VaR, and Monte Carlo.
- `docs/theory/` — mathematical and statistical foundations.
- `docs/roadmap/` — implementation roadmap, review notes, and progress tracking.

## Reading order for new contributors

1. `docs/design/ARCHITECTURE.md`
2. `docs/design/MARKET_AND_CURVES.md`
3. `docs/design/ANALYTICS_SERVICES.md`
4. `docs/pricing/INDEX.md`
5. `docs/risk/INDEX.md`
6. `docs/theory/INDEX.md`
7. `docs/roadmap/STATUS.md`

## Documentation policy

- Keep `docs/design/ARCHITECTURE.md` as the canonical architecture overview.
- Keep implementation-facing pricing notes under `docs/pricing/`.
- Keep implementation-facing risk notes under `docs/risk/`.
- Keep mathematical foundations under `docs/theory/`.
- When adding a major module, update the relevant index file and `docs/roadmap/STATUS.md`.
