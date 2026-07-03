# Curves and Term Structure

## Definition of a Curve

A curve is a set of prices across future delivery periods.

Example gas curve:

```text
TTF Month+1
TTF Month+2
TTF Q1-2027
TTF Summer-2027
TTF Winter-2027
TTF Cal-2028
```

Example power curve:

```text
German Power Base Day
German Power Base Weekend
German Power Base Week
German Power Base Month
German Power Base Quarter
German Power Base Year
```

## Definition of Term Structure

Term structure means the shape of prices across maturities.

Example:

```text
Winter gas > Summer gas
```

This indicates seasonal demand/storage value.

## Forward and Futures Curves

A **forward curve** uses forward prices, usually OTC.

A **futures curve** uses exchange futures settlement prices.

Exchange-traded futures often provide liquid anchors for forward-curve construction.

Main differences:

| Feature           | Forward                    | Future               |
|-------------------|----------------------------|----------------------|
| Market            | OTC                        | Exchange             |
| Standardization   | Custom                     | Standard             |
| Counterparty risk | Bilateral                  | Cleared              |
| Margining         | Usually not daily          | Daily mark-to-market |
| Transparency      | Lower                      | Higher               |
| Use in curve      | Often custom/broker quotes | Liquid anchors       |

## Structural Features of Power Curves

Electricity contracts are delivered over periods. A monthly or annual product is therefore an average of hourly prices,
not a single point on a maturity axis.

For baseload:

$$
F(t,T_1,T_2) =
\frac{1}{N}
\sum_{h \in [T_1,T_2]} f_h
$$

where:

- $F(t,T_1,T_2)$ is the quoted delivery-period price;
- $f_h$ is the hourly forward price;
- $N$ is the number of delivery hours.

For peakload:

$$
F_{\text{peak}}(t,T_1,T_2) =
\frac{\sum_{h \in \mathcal{P}} w_h f_h}{\sum_{h \in \mathcal{P}} w_h}
$$

where $\mathcal{P}$ is the set of peak hours.

## Matrix formulation

Let:

- $Q$ = market quotes;
- $A$ = mapping from granular hourly/daily curve to products;
- $f$ = unknown granular curve.

Then:

$$
Q = Af
$$

Example:

```text
A yearly baseload price is the average of 8,760 hourly forward prices.
```

## Bid/ask constraints

If the market quote has bid/ask:

$$
Q_i^{bid} \leq
(Af)_i \leq
Q_i^{ask}
$$

These bounds are important because market data can be sparse, asynchronous, and noisy.

In curve construction, a **market constraint** means a condition that the granular curve must satisfy when aggregated
back to traded products. Common examples are:

- firm settlement constraints, where the curve must exactly reproduce a liquid exchange settlement;
- bid/ask constraints, where the curve must fall inside the observable market spread;
- no-obvious-arbitrage constraints, where monthly, quarterly, seasonal, yearly, peak, off-peak, and baseload averages
  must be mutually consistent;
- shape or plausibility constraints, such as bounded hourly spreads, controlled peak/off-peak ratios, or bounded
  deviations from an approved reference shape.

In notation:

$$
q^{bid} \leq Af \leq q^{ask}
$$

Here $A$ maps hourly or bucket-level prices $f$ back to quoted products. If a quote is very liquid and should be matched
exactly, set $q^{bid}=q^{ask}=q^{mid}$ for that row, or use a very tight tolerance.

## Curve construction objective

A practical optimization:

$$
\min_f
\sum_i \omega_i\left((Af)_i-Q_i^{mid}\right)^2 +
\lambda \sum_h (f_{h-1}-2f_h+f_{h+1})^2
$$

First term: match market quotes.  
Second term: smoothness.

If quote matching is treated as a binding market constraint, the equivalent problem is:

$$
\min_f
\frac{1}{2}(f-B)^\top W(f-B) +
\frac{\lambda}{2}\lVert Df\rVert^2
\quad \text{subject to} \quad
q^{bid} \leq Af \leq q^{ask}
$$

where $B$ is the bottom-up or historical prior shape, $W$ controls confidence in that prior, and $D$ is usually a first-
or second-difference matrix that penalizes roughness.

## Bottom-Up Shape Information

If $B_h$ is a fundamental/bottom-up forecast shape:

$$
\min_f
\sum_i \omega_i\left((Af)_i-Q_i^{mid}\right)^2 +
\lambda \sum_h (f_{h-1}-2f_h+f_{h+1})^2 +
\gamma \sum_h (f_h - \alpha - \beta B_h)^2
$$

This allows market prices to fix the level while bottom-up models provide hourly/seasonal shape.

## Curve construction methods

| Method                        | Advantages                | Limitations                               |
|-------------------------------|---------------------------|-------------------------------------------|
| Direct market quotes          | Transparent market anchor | Sparse; no hourly shape                   |
| Bootstrapping                 | Market-consistent         | Requires sufficient quote hierarchy       |
| Linear interpolation          | Robust and predictable    | Ignores seasonality                       |
| Cubic spline                  | Smooth                    | Can overshoot                             |
| Maximum smoothness            | Stable curve              | Can suppress true seasonal structure      |
| Fourier seasonality           | Captures annual cycles    | Can overfit or underfit local structure   |
| Bottom-up shape + constraints | Suited to delivery curves | Depends on prior-shape quality            |
| PCA/factor model              | Suited to scenarios       | Historical calibration can fail in stress |
| HJM-style curve model         | Theoretically grounded    | Higher model and calibration complexity   |

### Direct market quotes

Direct market quotes are the first anchors of the curve. If reliable exchange settlements, broker marks, or tradable OTC
quotes exist for a product, the curve should normally reproduce those prices inside the bid/ask spread.

Examples:

- EEX German Power Baseload Month;
- EEX French Power Peak Quarter;
- regional OTC baseload Cal product;
- TTF gas month or season;
- EUA futures.

The limitation is that a quoted product is usually an average over a delivery block. A Cal baseload quote tells us the
average price over all hours in the calendar year, but it does not tell us the hourly shape, weekday/weekend split, winter
premium, or peak/off-peak spread.

### Bootstrapping

Bootstrapping solves for an unknown delivery period using larger and smaller quoted periods. The logic is weighted
averaging by delivery hours.

If Jan, Feb, Mar, Q2, Q3, and Cal are quoted, Q4 can be inferred as the residual needed to make the annual average equal
the Cal quote:

$$
F_{Q4} =
\frac{
F_{Cal}H_{Cal} -
F_{Jan}H_{Jan} -
F_{Feb}H_{Feb} -
F_{Mar}H_{Mar} -
F_{Q2}H_{Q2} -
F_{Q3}H_{Q3}
}{H_{Q4}}
$$

This is market-consistent, but it only applies when the quote hierarchy is sufficiently complete. It can also amplify
erroneous input data: one stale annual quote can produce an implausible residual quarter.

Use bootstrapping when the unknown block is **mathematically implied** by the quoted hierarchy. For example, if Cal,
Q1, Q2, and Q3 are known, Q4 is a residual. If Jan, Feb, and Q1 are known, Mar is a residual. In those cases, an
optimization is unnecessary because the averaging identity gives a direct solution.

Bootstrapping should not be used to infer shape that is not implied by the quoted hierarchy. A Cal quote alone does not
identify the hourly profile. A Q3 quote alone does not identify July, August, and September separately. Those problems
require a shape model plus constraints.

Recommended hierarchy:

1. use direct market quotes where liquid;
2. bootstrap only periods that are exactly implied by liquid larger/smaller blocks;
3. use constrained bottom-up or historical shape to allocate block prices into missing months, days, peak/off-peak
   buckets, or hours;
4. validate the final curve by aggregating it back to all quoted products.

### Linear interpolation

Linear interpolation fills missing points between known tenor nodes. It is robust and predictable, which makes it useful
for preliminary marks or less seasonal commodities.

For power, a straight line from summer to winter can miss the shape caused by heating demand, hydro reservoir value,
renewables, outages, holidays, or cross-border constraints.

### Cubic spline

A cubic spline gives a smooth curve through known nodes. It is visually appealing and can reduce artificial jumps.

The main limitation is overshoot. In power, sparse quotes and genuine seasonal discontinuities are common. A spline can
create prices above or below reasonable market levels between nodes. If a spline is used, monotone or constrained
variants are usually preferable to an unconstrained cubic spline.

### Maximum smoothness

Maximum smoothness treats curve construction as an optimization problem. The curve should match market quotes while
penalizing excessive curvature:

$$
\min_f
\sum_i \omega_i\left((Af)_i - Q_i^{mid}\right)^2 +
\lambda \sum_h (f_{h-1} - 2f_h + f_{h+1})^2
$$

This is useful for stable valuation curves, but smoothness is not an economic model. A physically plausible power curve
may have jumps around weekends, holidays, seasons, or scarcity periods. Smoothness should therefore be a regularizer, not
the only source of structure.

### Fourier seasonality

Fourier terms represent recurring seasonal shape with sine and cosine factors:

$$
f_t =
a_0 +
a_1 \cos(2\pi t) +
b_1 \sin(2\pi t) +
a_2 \cos(4\pi t) +
b_2 \sin(4\pi t)
$$

This representation captures broad annual and semi-annual cycles. It is less effective for short-lived spikes, holidays,
hydro shocks, plant outages, or scarcity events. Too many terms can overfit history; too few terms can miss economically
important shape.

### Bottom-up shape plus market constraints

For power curves, a fundamental or historical shape $B_h$ is often the most informative prior. The fitted curve is then
adjusted so that quoted products are matched.

Inputs to $B_h$ can include:

- historical spot shape by hour, weekday, month, and season;
- load forecasts;
- renewable generation forecasts;
- hydro reservoir and inflow assumptions;
- thermal stack, gas price, CO2 price, and plant availability;
- interconnector constraints;
- approved shape marks.

The model supplies the hourly shape; market quotes supply the traded levels. A common implementation is:

$$
f_h = \alpha_m + \beta_m B_h
$$

where $\alpha_m$ and $\beta_m$ are calibrated by month or delivery block so that the average over each traded product
matches the observed market quote.

The more general implementation is a constrained quadratic optimization. The curve is chosen to stay close to the
bottom-up prior while satisfying market constraints:

$$
\min_f
\frac{1}{2}\sum_h w_h(f_h-B_h)^2
\quad \text{subject to} \quad
q^{bid} \leq Af \leq q^{ask}
$$

If the market constraints are exact equalities $Af=q$, the equality-only solution has a closed form:

$$
f^* = B + W^{-1}A^\top(AW^{-1}A^\top)^{-1}(q-AB)
$$

The solution starts from the prior shape $B$ and adds the smallest weighted adjustment needed to reproduce the quoted
products.

With bid/ask inequalities, smoothness penalties, bounds, and optional no-arbitrage constraints, the same problem becomes
a convex quadratic program. It can be solved with a standard active-set, interior-point, or operator-splitting QP solver.
For a prototype, CVXPY is convenient. For production C++ analytics, the implementation should keep the problem in the
form:

$$
\min_f
\frac{1}{2}f^\top Hf + c^\top f
\quad \text{subject to} \quad
l \leq Mf \leq u
$$

where $H$ contains prior-shape and smoothness weights, $c$ contains the linear part of the prior-shape objective, and
$Mf$ contains all product averages and shape constraints.

#### Simple constrained example

Assume a one-day curve with four representative hourly buckets:

| Bucket | Prior shape $B_h$ |
|---|---:|
| Off-peak 1 | 58 |
| Off-peak 2 | 62 |
| Peak 1 | 80 |
| Peak 2 | 84 |

The market says day baseload should average 70 and peakload should average 84:

$$
\frac{f_1+f_2+f_3+f_4}{4} = 70
$$

$$
\frac{f_3+f_4}{2} = 84
$$

The prior averages are:

$$
\frac{58+62+80+84}{4} = 71
$$

$$
\frac{80+84}{2} = 82
$$

The prior is therefore high on baseload by 1 EUR/MWh and low on peak by 2 EUR/MWh. Solve:

$$
\min_f
\frac{1}{2}\sum_{h=1}^{4}(f_h-B_h)^2
\quad \text{subject to} \quad
\frac{f_1+f_2+f_3+f_4}{4}=70,\quad
\frac{f_3+f_4}{2}=84
$$

The constrained least-squares solution is:

$$
f^* = [54,\ 58,\ 82,\ 86]
$$

Check:

$$
\frac{54+58+82+86}{4} = 70
$$

$$
\frac{82+86}{2} = 84
$$

The result preserves the prior shape as much as possible, but forces the aggregated products to match the market.

#### Tenor correlation in curve fitting

Market constraints and tenor correlation serve different roles.

Market constraints define what the fitted curve must reproduce:

$$
q^{bid} \leq Af \leq q^{ask}
$$

Tenor correlation defines which adjustment away from the prior shape is most plausible. Instead of penalizing each curve
point independently, use the covariance matrix of historical curve moves:

$$
\min_f
\frac{1}{2}(f-B)^\top \Sigma^{-1}(f-B)
\quad \text{subject to} \quad
q^{bid} \leq Af \leq q^{ask}
$$

Here:

- $B$ is the bottom-up or historical prior curve;
- $f$ is the fitted curve;
- $\Sigma$ is the covariance matrix of curve moves across tenors or delivery buckets;
- $\Sigma^{-1}$ is the precision matrix;
- $A$ maps the fitted curve back to quoted traded products.

This is a Mahalanobis-distance fit. A move receives a lower penalty if it is historically common and a higher penalty if
it is historically unlikely. If two tenors are highly correlated, the optimizer prefers to move them together rather than
creating an unusual twist. If a tenor is very volatile, moving it is less penalized than moving a historically stable
tenor.

The covariance matrix can be estimated from historical curve changes:

$$
\Delta f_t = f_t - f_{t-1}
$$

Then:

$$
\Sigma = \operatorname{Cov}(\Delta f_t)
$$

For lognormal-style relative moves, use log changes:

$$
\Delta \log f_t = \log f_t - \log f_{t-1}
$$

In production, raw covariance is often noisy or nearly singular, especially for sparse energy curves. Shrinkage or a
factor model is usually required:

$$
\Sigma_{\text{reg}} = (1-\eta)\Sigma_{\text{sample}} + \eta\Sigma_{\text{prior}}
$$

or:

$$
\Sigma = LL^\top + \Psi
$$

where $L$ contains level, slope, seasonal, peak/off-peak, or front/back factor loadings, and $\Psi$ is idiosyncratic
variance.

The QP form becomes:

$$
\min_f
\frac{1}{2}(f-B)^\top \Sigma_{\text{reg}}^{-1}(f-B) +
\frac{\lambda}{2}\lVert Df\rVert^2
\quad \text{subject to} \quad
q^{bid} \leq Af \leq q^{ask}
$$

where $D$ is an optional smoothness operator. The covariance term handles plausible co-movement; the smoothness term
discourages unnecessary jaggedness.

Simple two-tenor example:

| Tenor | Prior $B$ |
|---|---:|
| Front month | 80 |
| Back month | 70 |

Suppose the market only quotes an average of 78:

$$
\frac{f_1+f_2}{2} = 78
$$

The prior average is 75, so the fitted curve must move up by 3 on average. If the two tenors are highly correlated, the
covariance-weighted fit prefers a joint level move. If the front tenor is much more volatile than the back tenor, the fit
may put more of the adjustment into the front tenor because that move is statistically less surprising.

The implementation distinction is:

```text
market constraints = what must be matched
covariance matrix = how the curve is most plausibly adjusted
```

For deterministic curve fitting, this covariance-weighted objective is enough. For stochastic valuation, the same
correlation information is also used to generate correlated Brownian or factor shocks across tenors.

### PCA and factor models

PCA is mainly a risk and scenario tool rather than a deterministic curve-construction tool. Historical curve changes are
decomposed into factors such as level, slope, seasonal spread, and peak/off-peak moves.

Applications include:

- VaR and Expected Shortfall;
- stress scenarios;
- correlated power/gas/carbon shocks;
- explaining portfolio sensitivity to curve moves.

The main limitation is regime dependence. A factor model trained on calm history may underrepresent gas crises, hydro
droughts, nuclear outages, or policy shocks.

### HJM-style curve model

An HJM-style model describes the stochastic dynamics of the whole forward curve:

$$
dF(t,T) = \mu(t,T)dt + \sum_k \sigma_k(t,T)dW_t^k
$$

It is useful for options and structured products because it models how the whole curve evolves, not just one delivery
price. The trade-off is higher complexity: calibration, simulation, no-arbitrage conditions, and electricity-specific
non-storability make it more demanding than deterministic curve construction.

## Reference Curve-Building Workflow

A production energy curve builder should:

1. collect liquid exchange, broker, and OTC quotes for power, gas, carbon, and related spreads;
2. normalize each quote into an internal product definition: market area, currency, unit, delivery period, load profile,
   calendar, and source quality;
3. anchor liquid products to observable bid/ask quotes;
4. bootstrap directly implied periods where the quote structure allows it;
5. construct hourly or quarter-hourly power forwards using bottom-up/historical shape plus market constraints;
6. enforce no-obvious-arbitrage checks across months, quarters, seasons, years, base, peak, and off-peak;
7. compare with approved marks and external benchmarks;
8. store curve versions, source lineage, validation results, and model overrides for risk and audit.

This workflow is deliberately region-agnostic. The same structure applies whether the delivery zone is a national power
market, a gas hub, a carbon contract, or an internal aggregation area.

## Platform Representation

The curve builder should expose the following objects:

- input quote set with source and quality metadata;
- product-to-bucket mapping matrix $A$;
- prior or bottom-up shape $B$;
- fitted granular curve $f$;
- quote-reproduction diagnostics $Af-q$;
- validation results for delivery calendars, load profiles, and bid/ask constraints;
- curve version and lineage identifiers.

The curve should be a reusable market object. Pricing and risk should consume the same built curve, with explicit
scenario overlays when shocked curves are needed.
