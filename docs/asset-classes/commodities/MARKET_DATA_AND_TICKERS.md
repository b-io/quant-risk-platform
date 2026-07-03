# Market Data and Tickers

## 1. Key Principle

Vendor tickers are input identifiers, not the platform data model. A robust energy platform maps each vendor, exchange,
or broker identifier into a canonical instrument and risk-factor representation:

```text
vendor_identifier -> canonical_instrument_id -> canonical_factor_id
```

Canonical quote fields include:

```text
commodity
hub_or_zone
load_profile
delivery_start
delivery_end
expiry
currency
unit
source
bid
ask
mid
timestamp
quality_flag
```

The same product may appear through several sources. The canonical model should preserve source lineage while giving
pricing and risk code a stable identifier.

## 2. Vendor and Venue Examples

### Bloomberg-style identifiers

Bloomberg is commonly used for futures chains, option chains, implied volatility, historical prices, curves, Greeks, and
news. Exact identifiers depend on data licenses and vendor configuration.

| Asset | Bloomberg-style example | Meaning |
|---|---|---|
| WTI crude | `CL1 Comdty` | Front WTI future |
| Brent crude | `CO1 Comdty` | Front Brent future, depending on setup |
| Gold | `GC1 Comdty` | Front COMEX gold future |
| Henry Hub gas | `NG1 Comdty` | Front US gas future |
| EUR/USD | `EURUSD Curncy` | FX spot |

For listed options, use the vendor's option-chain service rather than deriving identifiers manually.

### Refinitiv / LSEG identifiers

Refinitiv uses RICs. Examples include:

| Asset | RIC-style example | Meaning |
|---|---|---|
| WTI crude | `CLc1` | Front WTI continuation |
| Brent | `LCOc1` | Front Brent continuation |
| Gold | `GCc1` | Front gold continuation |
| Henry Hub gas | `NGc1` | Front US gas continuation |

Exact European power and gas RICs depend on venue, delivery period, product definition, and data entitlement.

### Exchange and transparency sources

EEX is central for European power derivatives and EU ETS products. ICE is important for Dutch TTF gas, NBP gas, Brent,
and some carbon products. ENTSO-E provides European electricity transparency data, while ENTSOG, AGSI, and GIE provide
gas-flow and storage data.

## 3. Market Data Needed By Curve

### Power curves

| Curve family | Typical data |
|---|---|
| Power baseload | Exchange futures, broker quotes, OTC marks, spot indices |
| Power peakload | Peak futures, broker quotes, peak/off-peak spreads |
| Spot and day-ahead | Exchange spot indices, auction data, vendor feeds |
| Shape and fundamentals | Load, wind, solar, hydro, outages, weather, holidays, interconnector constraints |

### Gas curves

| Curve family | Typical data |
|---|---|
| TTF | ICE Endex futures, broker quotes, spot indices |
| NBP | ICE NBP futures and spot indices |
| Regional gas hubs | Hub futures, broker quotes, spot indices |
| Storage and flows | AGSI/GIE storage levels, ENTSOG pipeline data, LNG data |

### Carbon curves

| Curve family | Typical data |
|---|---|
| EUA | EEX or ICE EUA futures and options |
| Carbon volatility | EUA option chains |
| Clean spark spread | Carbon curve plus gas and power curves |

## 4. Implementation Notes

The quote loader should normalize:

- units such as MWh, therms, barrels, tonnes, and currency per unit;
- delivery-period calendars and daylight-saving rules;
- load profile definitions such as base, peak, off-peak, and custom profiles;
- bid, ask, mid, settlement, and model-derived quote types;
- source quality and override status.

Risk-factor construction should then map quotes into stable factor names. For example:

```text
POWER:<zone>:BASE:CAL-2027
GAS:TTF:BASE:Q1-2027
CARBON:EUA:DEC-2027
SPREAD:CLEAN_SPARK:<zone>:CAL-2027
```

The exact syntax can evolve, but the invariant should not: analytics code should consume canonical identifiers with
explicit delivery periods, units, and source lineage.
