# Multi-Asset Market Data, Curves, Surfaces, and Pricing Inputs

## 1. Why a risk platform needs a unified market-data model

A production-shaped risk engine should not think only in terms of individual instruments.  
It should think in terms of **market objects** that many instruments depend on.

Examples:

- interest-rate discount curves
- forward curves
- credit spread curves
- default / hazard curves
- FX spot and forward curves
- equity spots and dividend curves
- commodity forward curves
- volatility surfaces
- correlation matrices

A clean platform architecture typically follows this pattern:

$$
\text{Raw market quotes}
\longrightarrow
\text{Normalized factor set}
\longrightarrow
\text{Curves / surfaces / market objects}
\longrightarrow
\text{Pricing and risk analytics}
$$

This chapter describes the most important market objects for the major asset classes.

---

## 2. Rates market data

Rates products rely on term structures.

### Typical raw quotes

- overnight fixings
- deposits
- OIS swap rates
- IRS par swap rates
- FRAs
- futures
- basis swaps

### Core market objects

- OIS discount curve
- IBOR / term forward curves
- inflation curves if relevant
- swaption volatility cube or surface for options

### Typical pricing dependencies

- discounting from OIS
- projection from tenor-specific forward curves
- option pricing from vol surfaces

### Risk factors

- zero-rate nodes
- forward-rate nodes
- principal components of the curve
- vol nodes

---

## 3. Credit market data

Credit pricing often depends on both rates and credit-specific objects.

### Typical raw quotes

- bond prices or z-spreads
- CDS spreads by maturity
- index spreads
- recovery assumptions
- tranche / index option vols in more advanced settings

### Core market objects

- spread curve
- hazard-rate or default-intensity curve
- recovery rate term or scalar assumption
- base discount curve

### Survival probabilities

In reduced-form models, survival probability is

$$
Q(0,T) = \exp\left(-\int_0^T \lambda(u)\,du\right),
$$

where $\lambda(t)$ is the default intensity.

### Defaultable discounting intuition

Very roughly, if a payoff is contingent on survival, pricing depends on both discounting and survival:

$$
V_0 \approx \mathbb{E}\left[e^{-\int_0^T r_u\,du} \mathbf{1}_{\{\tau > T\}} X_T\right].
$$

In practice, the implementation details depend on the instrument.

### Practical implementation stages

1. spread-shock support and CS01-style analytics
2. spread-curve market-state object
3. hazard-curve bootstrap from CDS quotes
4. structured credit extensions later if needed

---

## 4. FX market data

FX products connect two currencies, so FX pricing naturally requires more than a single spot rate.

### Typical raw quotes

- spot FX
- FX forwards or forward points
- domestic and foreign interest-rate curves
- FX implied vol surface
- cross-currency basis quotes for advanced setups

### Core market objects

- spot FX map
- domestic discount curve
- foreign discount curve
- forward curve or implied forwards
- option vol surface

### Covered interest parity intuition

Ignoring some market frictions and basis effects, an FX forward is linked to discount curves by

$$
F(0,T) = S_0 \frac{P_f(0,T)}{P_d(0,T)}.
$$

So FX and rates are tightly linked.

---

## 5. Equity market data

Equity pricing depends on more than spot.

### Typical raw quotes

- stock or index spot level
- dividend assumptions or dividend curve
- funding or repo curve
- implied volatility surface

### Core market objects

- spot price
- dividend yield or dividend schedule
- funding curve
- volatility surface

### Simple pricing relation

A stylized equity forward price can be written as

$$
F(0,T) = S_0 e^{(r-q)T},
$$

where $r$ is a funding rate and $q$ is a dividend yield under simple assumptions.

---

## 6. Commodity market data

Commodity markets often differ from equities because forward curves are central and storage, seasonality, and convenience yield matter.

### Typical raw quotes

- spot price
- futures prices by expiry
- option implied vols by expiry / delta / strike
- storage or convenience-yield assumptions in some models

### Core market objects

- futures or forward curve
- discount curve
- optional convenience-yield model parameters
- volatility surface

### Common representation

A risk system often models commodity factors directly by expiry buckets:

$$
\{F(0,T_i)\}_{i=1}^n.
$$

This is analogous to rate-node modeling.

---

## 7. Volatility surfaces

Many derivatives need a volatility surface rather than a single volatility parameter.

### Typical surface dimensions

- maturity and strike
- maturity and delta
- expiry, tenor, and strike for swaptions

### Why the surface matters

The surface controls optionality and therefore affects:

- vega risk
- convexity
- nonlinear scenario P&L
- hedging behavior

### Modeling choices

A production system may store the surface as:

- quoted market nodes plus interpolation rules
- parametric model output
- total variance parameterization

---

## 8. Correlation and covariance structures

Risk engines also need **cross-factor dependence**.

Examples:

- covariance of rate-node changes
- correlation of equity returns across names or sectors
- joint dynamics of rates, spreads, and FX

A covariance matrix for factors $X$ can be written as

$$
\Sigma = \operatorname{Cov}(X).
$$

This is needed for:

- Gaussian factor simulation
- principal-component reduction
- parametric VaR
- stress design and clustering

---

## 9. Unified factor taxonomy

A scalable platform benefits from a generic factor representation. For example, a factor can be identified by

$$
(\text{asset class}, \text{family}, \text{currency / underlier}, \text{pillar}, \text{quote type}).
$$

Examples:

- `(Rates, OIS, USD, 5Y, ZeroRate)`
- `(Credit, SpreadCurve, CDX.IG, 5Y, Spread)`
- `(FX, Spot, EURUSD, Spot, Mid)`
- `(Equity, Spot, SPX, Spot, Mid)`
- `(Commodity, ForwardCurve, Brent, Dec-2027, Forward)`

This gives the platform a common language for scenarios and attribution.

---

## 10. Pricing all asset classes: what market data is usually needed?

The table below is conceptual rather than exhaustive.

### Rates

Needed:

- discount curve
- forward curves
- vol surface for options

### Credit

Needed:

- base discount curve
- credit spread or hazard curve
- recovery assumption
- option vol or correlation data for advanced products

### FX

Needed:

- spot FX
- domestic and foreign curves
- forward points or implied forwards
- vol surface

### Equities

Needed:

- spot
- dividend curve
- funding / repo curve
- vol surface

### Commodities

Needed:

- forward curve
- discount curve
- vol surface
- seasonality / storage / convenience-yield assumptions if required

---

## 11. Risk implications by asset class

### Rates

Main risks:

- DV01 / PV01
- key-rate risk
- basis risk
- convexity
- vega for options

### Credit

Main risks:

- CS01
- spread-bucket risk
- jump-to-default for some instruments
- recovery sensitivity

### FX

Main risks:

- delta to spot
- curve sensitivity in both currencies
- vega and smile risk

### Equities

Main risks:

- delta, gamma, vega
- dividend sensitivity
- funding / borrow sensitivity

### Commodities

Main risks:

- forward-bucket risk
- calendar spread risk
- vega
- seasonality-sensitive shape risk

---

## 12. Implementation guidance for Quant Risk Platform

The platform should store market objects in a typed immutable market snapshot.

A good conceptual interface is:

$$
\text{MarketSnapshot} = \{ \text{curves}, \text{surfaces}, \text{spots}, \text{fixings}, \text{correlations}, \text{metadata} \}.
$$

Each pricing function should request only the identifiers it needs.  
Example:

- a vanilla IRS might need `USD/OIS/DISCOUNT` and `USD/SOFR/3M_FORWARD`
- a corporate bond with spread risk might need `USD/OIS/DISCOUNT` and `ISSUER_X/SPREAD_CURVE`
- an FX option might need `EURUSD/SPOT`, `USD/OIS`, `EUR/OIS`, and `EURUSD/VOL_SURFACE`

This makes the architecture scalable without coupling every trade type to the entire market state.

---

## 13. Summary

Pricing across asset classes requires a family of market objects, not just a single price vector.  
The platform should therefore be designed around:

- typed market identifiers
- reusable curves and surfaces
- explicit quote conventions
- cross-asset factor taxonomy
- a clear separation between raw data, built market objects, and analytics

This is what turns a collection of pricing functions into a real risk platform.
