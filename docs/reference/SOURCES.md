# Sources

This chapter lists public references and source classes useful for the
documentation. It is intentionally project-level: asset-class chapters should
link here instead of maintaining separate bibliographies unless a chapter needs
a narrow, local reference list.

Private preparation notes, company-specific research, and location-specific
extracts belong under the ignored `temp/docs/` workspace unless they are
rewritten as general project material.

## Quantitative Finance

- Brigo and Mercurio, *Interest Rate Models: Theory and Practice*.
- Hull, *Options, Futures, and Other Derivatives*.
- Andersen and Piterbarg, *Interest Rate Modeling*.
- Glasserman, *Monte Carlo Methods in Financial Engineering*.
- Joshi, *The Concepts and Practice of Mathematical Finance*.
- Shreve, *Stochastic Calculus for Finance*.
- Lando, *Credit Risk Modeling*.
- O'Kane, *Modelling Single-name and Multi-name Credit Derivatives*.

## Rates And Curves

- Public central-bank and exchange material on OIS, money-market, swap, and
  futures conventions.
- ISDA definitions and market-standard documentation for rates products.
- QuantLib documentation and source code for term structures, helpers,
  schedules, calendars, indices, and observer semantics.
- Public benchmark administrator documentation for overnight and term-rate
  indices.

## FX

- Public exchange and market-convention documentation for spot, forwards, FX
  swaps, NDFs, and currency-pair quotation conventions.
- ISDA documentation for FX derivatives and settlement conventions.
- Vendor documentation for spot, forward-point, and volatility-surface field
  mapping, subject to license restrictions.

## Credit

- ISDA credit derivatives definitions and CDS standard-model documentation.
- Public index documentation for major CDS index families.
- Public issuer, bond, and exchange documentation for cash-credit conventions.
- Standard references on reduced-form default modeling, hazard-rate
  calibration, survival curves, recovery, CS01, and jump-to-default.

## Equity

- Public exchange documentation for equity, index, option, and futures contract
  specifications.
- Public index provider methodology documents for index construction,
  dividends, corporate actions, and rebalancing.
- Standard references on equity forward pricing, dividend curves, borrow costs,
  American exercise, volatility surfaces, and local or stochastic volatility.

## Commodities

- Public exchange documentation for power, gas, emissions, oil, metals, and
  agricultural futures and options.
- EEX power, gas, and EUA product specifications.
- ICE Dutch TTF Natural Gas Futures and NBP Natural Gas Futures product
  specifications.
- ENTSO-E transparency platform documentation.
- ENTSOG transparency platform documentation.
- AGSI and GIE gas storage documentation.
- National power exchange documentation for day-ahead and intraday auction
  conventions.
- Fleten and Lemming, "Constructing forward price curves in electricity
  markets", *Energy Economics*, 2003.
- Standard references on commodity forward curves, delivery-period averaging,
  convenience yield, storage valuation, stochastic control, and Least Squares
  Monte Carlo for flexible assets.

## Market Data And Vendors

- Bloomberg, Refinitiv/LSEG, exchange, broker, and clearing documentation for
  licensed ticker, RIC, field, and contract mapping.
- Public exchange calendars, settlement price definitions, expiry rules, and
  contract-roll methodologies.
- Internal production systems should map licensed vendor identifiers into
  canonical platform identifiers before pricing, risk, or persistence uses them.

## Implementation

- QuantLib documentation and source code for financial instruments, calendars,
  schedules, term structures, volatility structures, and pricing engines.
- pybind11 documentation for Python bindings.
- CMake documentation for build configuration.
- SQLite or selected storage-engine documentation for persistence semantics,
  transactions, and reproducibility controls.

## Source-Use Rule

Published documentation should distinguish between:

- public market conventions;
- licensed vendor field mappings;
- implementation choices made by this repository;
- assumptions used only for examples or tests.

When a source is licensed, restricted, local, or private, the public document
should describe the canonical platform concept without reproducing proprietary
content.
