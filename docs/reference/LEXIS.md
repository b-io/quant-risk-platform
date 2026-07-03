# Lexis

This chapter defines canonical language for the platform. The goal is not to
replace asset-class conventions, but to keep shared terms stable across market
data, valuation, risk, storage, and implementation documents.

## Platform Objects

**Trade** is the contractual position held by the platform. A trade carries
identifiers, economic terms, lifecycle dates, book, counterparty, currency,
quantity, and product payload.

**Product** is the reusable payoff family associated with a trade, such as a
swap, bond, option, forward, futures contract, CDS, storage contract, or swing
contract.

**Product type** is the canonical product classifier used by loaders, support
checks, pricing dispatch, and reporting.

**Asset class** is the market family that supplies the dominant risk factors:
rates, FX, credit, equity, commodities, or cross-asset.

**Payoff** is the mathematical cashflow or optionality rule of the product.
Payoffs should be separated from trade identifiers and market data.

**Schedule** is the set of accrual periods, payment dates, fixing dates,
delivery periods, or exercise dates required by the product.

**Book** is the portfolio grouping used for ownership, aggregation, limits,
risk reporting, and PnL explain.

**Counterparty** is the legal or clearing entity attached to a trade. It should
remain distinct from issuer, reference entity, exchange, and clearing house.

**Valuation date** is the date on which market data, fixings, trade state, and
model configuration are interpreted.

## Market Data

**Market state** is the normalized collection of quotes, fixings, curves,
surfaces, conventions, metadata, and model inputs available to pricing and risk.

**Market snapshot** is a versioned market state for one valuation date and
source policy. A snapshot should be replayable.

**Quote** is an observed or approved market value with identity, source,
timestamp, unit, currency, convention, and quality metadata.

**Quote provenance** records where a quote came from, when it was observed or
approved, and which transformation or override policy produced the value used by
the platform.

**Curve** is a term structure derived from quotes and conventions. Examples
include discount curves, projection curves, survival curves, forward curves,
inflation curves, dividend curves, borrow curves, and commodity forward curves.

**Surface** is a two-dimensional or higher-dimensional market object, commonly
used for implied volatility by expiry and strike or moneyness.

**Stale quote** is a quote whose timestamp, source status, or observation policy
no longer satisfies the market-data validation rule for the run.

**Missing quote** is a required observation that cannot be found or inferred
under the declared fallback policy.

## Pricing And Results

**NPV** is the present value of a trade or portfolio under a specified market
state, model state, and valuation context.

**Pricing result** is the structured output of valuation. It should include NPV,
currency, model, valuation date, curve and surface identifiers, risk tags,
diagnostics, and product support status.

**Diagnostic** is a structured message explaining a valuation, support, market
data, calibration, or numerical condition. Diagnostics should be machine
readable enough for reporting and control checks.

**Supported** means that the product has a declared pricing implementation,
market-input contract, risk-factor mapping, and regression coverage for the
requested workflow.

**Partially supported** means that the product can be represented or priced only
under declared limitations.

**Unsupported** means that the platform can identify the product but cannot
complete the requested workflow. The failure reason should be explicit.

## Risk Factors

**Risk factor** is a named market quantity that can be shocked, simulated, or
attributed. Examples include rates curve nodes, FX spots, equity spots, credit
spreads, hazard rates, volatility points, and commodity forward nodes.

**RiskFactorId** is the canonical identifier for a risk factor. It should encode
asset class, factor type, underlying, tenor or expiry, strike or moneyness where
needed, currency, and convention.

**Sensitivity** is the local change in value with respect to a risk factor. It
must state bump size, bump unit, revaluation policy, and aggregation convention.

**PnL explain** decomposes portfolio value change into components such as carry,
roll-down, market move, cash, trade activity, FX translation, model or config
change, and residual.

**VaR** is a loss quantile over a horizon under a scenario distribution.

**Expected Shortfall** is the conditional tail loss beyond a VaR threshold.

## Rates

**Discount curve** maps dates or times to discount factors.

**Projection curve** maps dates or accrual periods to forward rates for a
specific floating index and tenor.

**OIS curve** is a curve built from overnight-indexed instruments, commonly used
as the collateralized discounting curve.

**IBOR tenor curve** is a projection curve for a specific term rate tenor.

**Deposit** is a money-market instrument with start date, maturity, rate,
currency, day count, and business-day convention.

**FRA** is a forward rate agreement on a future floating-rate accrual period.

**Swap** is a contract exchanging one set of cashflows for another, commonly
fixed versus floating or overnight indexed.

**Cap** and **floor** are portfolios of caplets or floorlets on floating-rate
fixings.

**Swaption** is an option to enter a swap. European swaptions have one exercise
date; Bermudan swaptions have a discrete exercise schedule.

## FX

**FX spot** is the exchange rate for near-standard spot settlement under a
currency-pair convention.

**FX forward** is an agreement to exchange currencies on a future date at a
fixed exchange rate.

**FX swap** combines two FX exchanges at different settlement dates.

**NDF** is a non-deliverable forward that settles the difference between a
contracted rate and a fixing in a settlement currency.

**FX volatility surface** maps expiry and strike or delta convention to implied
volatility.

## Credit

**Issuer** is the entity that issues a bond or loan.

**Reference entity** is the entity whose default drives a CDS payoff.

**Credit spread** is compensation above a benchmark curve. Its interpretation
depends on the benchmark, cashflow model, recovery assumption, and instrument.

**Hazard rate** is the default intensity used in reduced-form credit models.

**Survival curve** gives the probability of no default up to each maturity.

**Recovery rate** is the fraction of exposure or notional assumed recoverable
after default under the product convention.

**CDS** is a credit default swap with premium and protection legs.

**CS01** is the value change for a one basis point move in credit spreads under
the declared bump convention.

## Equity

**Equity spot** is the current price of an equity or index under the platform
quote convention.

**Dividend curve** describes expected cash or proportional dividends.

**Borrow curve** describes stock-borrow or financing costs relevant to forwards,
futures, and options.

**Equity forward** is the forward value implied by spot, dividends, funding, and
borrow assumptions.

**American option** is an option exercisable on a continuous or discrete set of
dates before expiry, depending on the product specification.

## Commodities

**Commodity spot** is the near-term price of a physical or financially settled
commodity under a location, grade, delivery, and unit convention.

**Commodity forward curve** is the term structure of forward or futures prices
for future delivery periods.

**Futures strip** is a collection of futures or forwards across adjacent
delivery periods.

**Convenience yield** is a model representation of the non-cash benefit or cost
of holding physical inventory.

**Power** means electric power. It is a flow commodity: generation, consumption,
storage, curtailment, and balancing are linked at short time scales.

**MW** is a unit of power or capacity. **MWh** is energy delivered over time.
For example, $10\ \mathrm{MW}$ delivered for $3$ hours equals
$30\ \mathrm{MWh}$.

**Baseload** means delivery during all hours in the delivery period.

**Peakload** means delivery during a defined subset of peak hours. The exact
definition is a market convention.

**Off-peak** is the complement of peakload under the same convention.

**Delivery period** is the interval during which a commodity is delivered. Power
and gas contracts often reference day, weekend, week, month, quarter, season,
or calendar-year delivery periods.

**Day-ahead** is a market for next-day electricity delivery cleared before the
delivery day.

**Intraday** is trading closer to physical delivery.

**Imbalance price** is the settlement price applied to deviations between
scheduled and actual injection or consumption.

**Gas storage** is a physical asset with injection, withdrawal, inventory, and
capacity constraints.

**Swing contract** gives volume flexibility subject to exercise, daily, and
cumulative constraints.

**PPA** is a power purchase agreement. Its valuation depends on delivery shape,
pricing formula, volume risk, credit risk, and settlement convention.

**EUA** is a European Union Allowance under the EU ETS. One allowance gives the
right to emit one tonne of carbon dioxide equivalent.

**Spark spread** is the simplified gross margin of a gas-fired power plant.

**Clean spark spread** subtracts carbon and variable operating costs from the
spark spread.

## Reusable Modeling Terms

**Implied volatility** is the volatility input that makes a pricing model
reproduce the observed option price.

**Greek** is a local sensitivity of value to a market, model, or time variable.

**Exercise policy** is the rule used to decide whether an exercisable product is
continued or exercised at each decision date.

**LSMC** is Least Squares Monte Carlo, a regression-based method for estimating
continuation value and exercise policy for path-dependent or early-exercise
products.
