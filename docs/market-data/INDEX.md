# Market Data Documentation Index

This section defines the market-state layer used by valuation, risk, explain,
stress, simulation, and historical replay.

Before adding instruments or models, the platform must define how observations
become reusable market objects. The market state is the normalized collection of
quotes, fixings, curves, surfaces, conventions, provenance, validation metadata,
and calibration configuration required to reconstruct pricing inputs.

## Canonical File

- `docs/market-data/MULTI_ASSET_MARKET_DATA_AND_CURVES.md`

## Scope

This section explains:

- raw observations versus normalized platform quotes;
- quote identifiers, source metadata, timestamps, units, and conventions;
- market snapshot versioning and replay;
- curve and surface reconstruction from stored inputs;
- stale quote checks and missing quote diagnostics;
- reuse of one market state across valuation, explain, stress, VaR, and
  simulation.

## Asset-Class Inputs

The market-data foundation should support:

- rates discount curves, projection curves, OIS curves, IBOR tenor curves, and
  later inflation curves;
- FX spot rates, forward points, domestic and foreign discount curves, and FX
  volatility surfaces;
- credit issuer curves, CDS spread curves, hazard curves, recovery rates, and
  bond curves;
- equity spot prices, dividend curves, borrow or funding curves, and equity
  volatility surfaces;
- commodity futures curves, proxy carry or convenience-yield inputs, and
  commodity volatility surfaces.

## Shared References

- `docs/reference/LEXIS.md` defines market state, market snapshot, quote,
  provenance, curve, surface, stale quote, and missing quote.
- `docs/reference/FORMULAS.md` contains shared valuation and curve formulas.
- `docs/reference/SOURCES.md` lists public exchange, vendor, and implementation
  source classes.

## Suggested Next Step

After this section, read `docs/asset-classes/rates/INDEX.md`, because rates
curves are the first concrete example of reusable market objects.

## Maintenance Rule

Market-data documentation should separate observed data, derived objects,
validation diagnostics, and model configuration. Hidden defaults in the market
layer should be avoided because they make valuation and replay ambiguous.
