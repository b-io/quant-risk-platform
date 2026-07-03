# Models Index

This section contains model families that are reused across asset classes.
Asset-class chapters describe conventions and product mappings; model chapters
describe mathematical assumptions, calibration choices, numerical methods, and
validation standards.

## Canonical Sections

1. `docs/models/volatility/INDEX.md`  
   Option-pricing foundations, exercise styles, Greeks, volatility surfaces, and
   volatility-model calibration trade-offs.

## Planned Sections

Future model-family chapters should be added here when they are reusable across
asset classes. Natural candidates include:

- LSMC and exercise-policy modeling;
- finite-difference methods;
- stochastic rates;
- stochastic volatility;
- credit default-intensity models;
- multi-factor scenario generation;
- covariance and correlation modeling.

## Maintenance Rule

Put a document in `docs/models/` when the same model family can support several
asset classes. Put it in `docs/asset-classes/` only when the material is mainly
about one asset-class convention or product family.
