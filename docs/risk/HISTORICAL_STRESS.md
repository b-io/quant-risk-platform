# Historical Stress Testing

## 1. What historical stress is

Historical stress testing evaluates today's portfolio under market moves that were actually observed during a past
crisis or stress window.

Instead of asking only local questions such as “what if the USD curve moves up by 10 bp?”, it asks a scenario question:

> What would happen to today's portfolio if the market moved the way it moved during a specific historical episode?

---

## 2. Mathematical setup

Let today's market factor vector be

$$
F_0 = (F_{0,1}, \dots, F_{0,d}).
$$

Suppose a historical window provides observed factor moves

$$
\Delta F^{(h)} = (\Delta F^{(h)}_1, \dots, \Delta F^{(h)}_d).
$$

Then the stressed market state is

$$
F_0^{\text{stress},h} = F_0 + \Delta F^{(h)}.
$$

If the current portfolio value is $V(F_0)$ and the stressed value is $V(F_0^{\text{stress},h})$, then the
historical-stress P&L is

$$
\Pi^{(h)} = V(F_0^{\text{stress},h}) - V(F_0).
$$

A large negative value of $\Pi^{(h)}$ indicates a severe loss under that scenario.

---

## 3. Why historical stress is useful

Historical stress is useful because it is grounded in real market behavior. It captures:

- large and correlated cross-asset moves,
- nonlinear portfolio effects,
- curve twists and basis dislocations,
- spread widening combined with liquidity shocks,
- market regimes that may not be visible in local Greeks.

It complements, rather than replaces, sensitivities, VaR, and Monte Carlo.

---

## 4. General process

A production workflow usually follows these steps:

1. define today's portfolio and today's market state,
2. choose a historical date or stress window,
3. compute historical factor moves,
4. map the historical factors to today's factor grid,
5. build the stressed market state,
6. reprice the portfolio,
7. aggregate and explain the stressed P&L.

---

## 5. Additive and relative shocks

Some factors are naturally shocked additively, others multiplicatively.

### 5.1 Additive shocks

For rates or spreads, one often uses additive shocks:

$$
F_0^{\text{stress}} = F_0 + \Delta F.
$$

Examples:

- OIS zero-rate nodes,
- credit spread nodes,
- implied vol points quoted in volatility points.

### 5.2 Relative shocks

For equity or commodity spots, one often uses relative shocks:

$$
S_0^{\text{stress}} = S_0 \times (1+r),
$$

where $r$ is the historical return.

For FX, both additive log-return and multiplicative-return conventions are common:

$$
S_0^{\text{stress}} = S_0 e^{r}.
$$

---

## 6. Mapping challenges

Historical stress is not simply “copy old prices into today's market”. Several mapping issues arise:

- today's curve pillars differ from historical pillars,
- some quotes are missing in one period but present in the other,
- conventions may have changed,
- instruments may use different underlying indices today,
- the portfolio composition is today's portfolio, not the historical one.

So a stress engine typically stores factor moves in normalized factor space, then maps them onto today's market
representation.

---

## 7. Stress by factor class

A useful historical stress framework separates factors such as:

- OIS and projection curve nodes,
- credit spread curve nodes,
- FX spots,
- equity spots,
- volatility surfaces,
- commodity forward curve nodes,
- basis and cross-currency spreads.

If the factor vector is partitioned as

$$
F = (F^{\text{rates}}, F^{\text{credit}}, F^{\text{FX}}, F^{\text{equity}}, F^{\text{vol}}, F^{\text{commodity}}),
$$

then a historical scenario may shock all components jointly, preserving the historically observed dependence structure.

---

## 8. Historical stress versus VaR

Historical stress and historical-simulation VaR are related but not identical.

### Historical stress

Evaluate one or a few named historical scenarios:

$$
\Pi^{(2008)}, \Pi^{(2020)}, \Pi^{(2022)}, \dots
$$

### Historical VaR

Use a long history of historical factor moves and take a quantile of the induced P&L distribution:

$$
\mathrm{VaR}_{\alpha}^{\text{hist}} = -Q_{1-\alpha}(\Pi^{(1)}, \dots, \Pi^{(N)}).
$$

So historical stress focuses on named episodes, while historical VaR focuses on a distributional quantile from many
observations.

---

## 9. Important historical stress scenarios

A useful first implementation should include a library of named scenarios.

It is helpful to separate two kinds of historical stress scenarios:

- **cross-asset crisis scenarios**, where equities, credit, FX, volatility, and rates move jointly;
- **rates-focused historical shocks**, where the main signal is a large move, twist, or regime shift in government and
  swap curves.

The scenario library should therefore contain both broad crisis episodes and famous large rates moves.

### 9.1 Global Financial Crisis (2008)

Typical factor behavior:

- credit spreads widened dramatically,
- interbank and basis spreads dislocated,
- equity markets sold off sharply,
- government rates often fell at the front end due to flight-to-quality,
- implied volatilities rose strongly.

Main assumptions to encode:

- large negative equity returns,
- large positive credit-spread shocks,
- downward parallel or bull-steepening moves in safe government curves,
- wider basis and liquidity premia.

### 9.2 Euro sovereign debt crisis (2011--2012)

Typical factor behavior:

- sovereign and bank credit spreads widened,
- peripheral-versus-core divergence increased,
- EUR risk sentiment weakened,
- cross-currency and funding stresses rose.

Main assumptions:

- issuer-specific and country-specific spread widening,
- non-parallel spread-curve moves,
- safe-haven curve compression in core currencies.

### 9.3 Rates sell-off / bond-massacre style shock (1994)

Typical factor behavior:

- large and rapid upward moves in front-end and intermediate rates,
- sharp repricing of central-bank expectations,
- losses in duration-heavy rates books,
- widening in some spread products as financing conditions tighten.

Main assumptions:

- additive upward shocks to OIS and swap curves by tenor,
- stronger shocks in short and intermediate maturities than in the long end when the chosen window implies bear
  flattening,
- optional widening in swap spreads, mortgage-related spreads, and credit-sensitive basis factors where available.

### 9.4 Taper tantrum (2013)

Typical factor behavior:

- abrupt upward repricing in long-end rates,
- bear steepening in USD curves,
- pressure on rate-sensitive equity sectors,
- spread pressure in EM and duration-sensitive credit.

Main assumptions:

- upward additive shocks to the USD OIS and swap curves,
- tenor-dependent steepener shocks rather than only parallel shifts,
- optional FX and spread follow-through for duration-sensitive assets.

### 9.5 Oil collapse (2014--2015)

Typical factor behavior:

- commodity forward curves shifted downward sharply,
- commodity-linked credit names weakened,
- inflation expectations softened in some regions.

Main assumptions:

- strong negative commodity spot and forward returns,
- selective credit-spread widening in energy sectors,
- linked moves in inflation curves where relevant.

### 9.6 Brexit referendum shock (2016)

Typical factor behavior:

- sharp moves in GBP FX,
- strong safe-haven demand in core rates,
- equity sector dispersion,
- volatility repricing across rates, FX, and equities.

Main assumptions:

- large relative shock to GBP spot and GBP crosses,
- safe-haven bull moves in core curves,
- volatility-surface repricing in GBP and EUR markets,
- cross-asset reporting by currency and region.

### 9.7 COVID shock (February--March 2020)

Typical factor behavior:

- violent equity sell-off,
- sharp widening in credit spreads,
- very large volatility increases,
- abrupt government-rate moves, often with flight-to-quality and policy easing,
- liquidity dislocations and basis widening.

Main assumptions:

- simultaneous multi-asset shock,
- strong volatility-surface repricing,
- large spread and basis shocks,
- nontrivial curve twists rather than only parallel shifts.

### 9.8 Inflation and rates repricing (2022)

Typical factor behavior:

- strong upward rate moves across developed markets,
- major bear-flattening or bear-steepening episodes depending on currency and window,
- widening in credit spreads,
- equity drawdowns, especially in duration-sensitive sectors.

Main assumptions:

- large additive shocks to OIS and projection curves,
- twist scenarios by tenor bucket,
- moderate-to-large spread widening,
- cross-asset pressure from discount-rate repricing.

### 9.9 UK gilt / LDI stress (2022)

Typical factor behavior:

- extremely large upward moves in long-end GBP rates,
- violent curve dislocations,
- severe stress in liability-driven investment structures,
- liquidity pressure in sterling rates markets.

Main assumptions:

- very large additive shocks to long-end GBP OIS and swap nodes,
- explicit steepener and dislocation scenarios in the 10Y to 30Y sector,
- optional widening in GBP swap spreads and basis factors.

### 9.10 Regional banking stress (2023)

Typical factor behavior:

- sharp front-end rate repricing,
- widening in financial credit spreads,
- safe-haven moves in government curves,
- strong moves in rate volatility and basis.

Main assumptions:

- concentrated financial-sector spread widening,
- localized but sharp rate-curve distortions,
- volatility spike at the short end.

### 9.11 Minimum recommended first scenario set

A practical first release does not need hundreds of scenarios. It should at least include:

- `GFC_2008`,
- `EURO_SOVEREIGN_2011`,
- `RATES_BOND_MASSACRE_1994`,
- `TAPER_TANTRUM_2013`,
- `COVID_Q1_2020`,
- `INFLATION_REPRICING_2022`,
- `UK_GILT_LDI_2022`,
- `REGIONAL_BANKING_2023`.

For rates implementation, the most important early scenarios are the 1994 sell-off, 2013 taper tantrum, 2022 global
inflation repricing, and 2022 UK gilt stress, because they represent very different shapes of large curve moves.

## 10. Aggregation and explain

Historical-stress output should not stop at total portfolio P&L. A useful report includes:

- total stressed P&L,
- P&L by desk, book, strategy, and currency,
- top losing trades,
- factor-group contributions,
- comparison across named scenarios.

If the portfolio can be decomposed into sub-portfolios $V = \sum_j V_j$, then

$$
\Pi^{(h)} = \sum_j \Pi_j^{(h)},
$$

which gives a natural attribution by desk or book.

---

## 11. What the current project should implement first

A strong first implementation should support:

- OIS curve-node historical shocks by currency,
- credit spread historical shocks by issuer or spread curve,
- optional FX and equity spot returns,
- replay of named scenarios on today's market state,
- structured reports with total P&L and top contributors.

The first version does not need a massive scenario database. A small curated library of named episodes is enough if the
factor mapping is clean.

---

## 12. Design implications for the risk engine

A production-shaped historical stress module should:

- define a normalized factor representation,
- separate scenario storage from scenario application,
- support additive and relative shocks,
- preserve today's instrument and curve conventions,
- reuse built instruments and market handles where possible,
- produce explainable output.

This makes historical stress not just a scenario runner, but a coherent part of the market-and-risk architecture.
