# CAPM and Other Asset-Pricing Foundations for Modern Valuation and Risk Management

## 1. Why this chapter matters

A practical way to organize modern valuation and risk management is to separate three layers:

1. **economic return models** used to think about expected returns and risk premia,
2. **no-arbitrage pricing models** used to value contingent cash flows,
3. **statistical and factor models** used to estimate sensitivities, scenarios, and portfolio risk.

CAPM sits in the first layer. Black-Scholes-Merton sits in the second. Multifactor and term-structure models sit between
the second and third because they are used both for pricing and for risk.

The key point is:

> there is no single “master model.” Production valuation and risk systems combine discounted-cash-flow logic,
> no-arbitrage curve construction, factor models, and scenario methods.

---

## 2. The umbrella pricing identity: present value and discounting

For a deterministic cash flow $CF_T$ received at time $T$,

$$
PV_t = \frac{CF_T}{(1+r)^{T-t}}
$$

or more generally with continuously compounded discount rates,

$$
PV_t = CF_T e^{-y(t,T)(T-t)}
$$

where:

- $PV_t$ is present value at time $t$,
- $CF_T$ is the future cash flow,
- $r$ is a simple discount rate,
- $y(t,T)$ is the continuously compounded zero rate for maturity $T$.

For risky cash flows, the difficult question is not the algebra but the **correct discounting rule**. In practice there
are two broad approaches:

- **discount-rate approach**: estimate a required return and discount expected cash flows,
- **state-price or risk-neutral approach**: price by replication or by arbitrage-free expectation under a transformed
  measure.

These correspond to two central traditions in finance.

---

## 3. The stochastic discount factor view

A very general one-period asset-pricing identity is:

$$
P_t = \mathbb{E}_t[m_{t+1} X_{t+1}]
$$

where:

- $P_t$ is the asset price today,
- $X_{t+1}$ is next-period payoff,
- $m_{t+1}$ is the **stochastic discount factor** (SDF), also called the pricing kernel.

This is one of the most useful organizing equations in asset pricing.

Interpretation:

- payoffs that arrive in bad states of the world are more valuable,
- payoffs that arrive when marginal utility is high receive higher state prices,
- the SDF encodes time value of money and risk adjustment together.

If the gross return of asset $i$ is $R_{i,t+1}=X_{i,t+1}/P_{i,t}$, then the pricing condition becomes:

$$
1 = \mathbb{E}_t[m_{t+1} R_{i,t+1}]
$$

This implies a covariance form:

$$
\mathbb{E}_t[R_{i,t+1}] - R_{f,t} = -\frac{\operatorname{Cov}_t(m_{t+1},R_{i,t+1})}{\mathbb{E}_t[m_{t+1}]}
$$

So expected excess return is high when the asset performs badly in high-value states and therefore has poor hedging
value.

This SDF view is the umbrella under which CAPM, consumption-based models, and risk-neutral pricing can all be placed.

---

## 4. Mean-variance analysis and Markowitz

The Markowitz framework is the starting point for modern portfolio theory.

For weights $w$, expected return vector $\mu$, and covariance matrix $\Sigma$:

$$
\mu_p = w^\top \mu
$$

$$
\sigma_p^2 = w^\top \Sigma w
$$

The classical efficient frontier solves either:

$$
\min_w \; w^\top \Sigma w \quad \text{subject to } w^\top \mu = \mu^*,\; \mathbf{1}^\top w = 1
$$

or equivalently maximizes expected return for a given variance level.

Why it matters:

- it defines the language of portfolio risk and diversification,
- it motivates the tangency portfolio and Sharpe ratio,
- it is the conceptual base from which CAPM is derived.

For implementation details, see `docs/theory/MARKOWITZ_PORTFOLIO_THEORY.md`.

---

## 5. CAPM: capital asset pricing model

### 5.1 Core idea

CAPM says that in equilibrium the expected excess return of an asset depends only on its exposure to the
**market portfolio**.

The standard equation is:

$$
\mathbb{E}[R_i] = r_f + \beta_i \left( \mathbb{E}[R_M] - r_f \right)
$$

where:

- $R_i$ is the asset return,
- $r_f$ is the risk-free rate,
- $R_M$ is the market portfolio return,
- $\beta_i$ is the asset’s market beta.

Beta is defined by:

$$
\beta_i = \frac{\operatorname{Cov}(R_i,R_M)}{\operatorname{Var}(R_M)}
$$

Interpretation:

- $\beta_i > 1$: asset amplifies market moves,
- $\beta_i < 1$: asset is less sensitive than the market,
- $\beta_i < 0$: asset hedges market risk.

### 5.2 Security Market Line

CAPM implies that assets should lie on the **Security Market Line**:

$$
\mathbb{E}[R_i] - r_f = \beta_i \cdot \text{market risk premium}
$$

Only **systematic risk** is priced. Idiosyncratic risk is not, because it can be diversified away in the model.

### 5.3 Assumptions behind the textbook version

The clean derivation assumes:

- investors care only about mean and variance,
- all investors can borrow/lend at the risk-free rate,
- everyone agrees on expected returns and covariances,
- markets are frictionless,
- investors hold combinations of the risk-free asset and the market portfolio.

These assumptions are too strong for literal use but the model remains foundational.

### 5.4 Why CAPM remains important in practice

CAPM still matters because it gives:

- a baseline expected-return model,
- a simple cost-of-equity estimate for valuation,
- a clean way to separate market risk from idiosyncratic risk,
- a common language for performance attribution and hurdle rates.

A standard corporate-finance use is:

$$
\text{Cost of equity} = r_f + \beta_{\text{equity}} \cdot \text{equity risk premium}
$$

which then feeds WACC-style discount rates.

### 5.5 Practical limitations

CAPM is often too narrow because:

- a single factor rarely explains cross-sectional returns well,
- beta estimates are unstable,
- the market portfolio is unobservable in the full theoretical sense,
- liquidity, carry, credit, size, value, and momentum effects matter in practice.

So CAPM is usually the **first benchmark**, not the final production model.

### 5.6 Small example

Suppose:

- risk-free rate $r_f = 2\%$,
- expected market return $\mathbb{E}[R_M] = 8\%$,
- asset beta $\beta_i = 1.3$.

Then:

$$
\mathbb{E}[R_i] = 2\% + 1.3(8\%-2\%) = 9.8\%
$$

So the asset’s required return is $9.8\%$ under CAPM.

---

## 6. Multifactor models and APT

### 6.1 Why CAPM is generalized

Many assets are exposed to multiple common drivers, not just the market. Examples:

- level / slope / curvature for rates,
- credit spread factors,
- inflation and growth shocks,
- commodity curve shape,
- equity value, size, momentum, profitability, investment.

This motivates **multifactor models**.

### 6.2 Linear factor return model

A common statistical or economic specification is:

$$
R_i - r_f = \alpha_i + \beta_{i1} F_1 + \cdots + \beta_{ik} F_k + \varepsilon_i
$$

where:

- $F_j$ are common factor realizations,
- $\beta_{ij}$ are factor loadings,
- $\varepsilon_i$ is idiosyncratic residual risk.

In matrix form:

$$
R - r_f \mathbf{1} = \alpha + B F + \varepsilon
$$

### 6.3 Arbitrage Pricing Theory (APT)

APT says that if returns are generated by a factor structure and idiosyncratic risk can be diversified away, then
expected excess returns must be approximately linear in factor exposures:

$$
\mathbb{E}[R_i] - r_f \approx \beta_i^\top \lambda
$$

where:

- $\beta_i$ is the vector of factor exposures for asset $i$,
- $\lambda$ is the vector of factor risk premia.

Interpretation:

- CAPM is a one-factor special case,
- APT is more flexible because it allows several priced sources of systematic risk.

### 6.4 Why APT matters in modern risk systems

APT-style models are close to the structure of real risk engines:

- map instruments to factor shocks,
- estimate sensitivities or scenario responses,
- aggregate factor PnL or exposures,
- separate common-factor risk from residual noise.

This is the logic behind many fixed-income, macro, and equity factor platforms.

---

## 7. Fama-French and empirical factor models

Empirical asset pricing expanded the CAPM framework with traded or mimicking factors. A standard example is the
three-factor specification:

$$
R_i - r_f = \alpha_i + \beta_M (R_M-r_f) + s_i \cdot SMB + h_i \cdot HML + \varepsilon_i
$$

Later extensions add profitability, investment, and momentum.

These models are not universal laws of nature. They are empirical summaries that help explain return cross-sections and
portfolio tilts.

Why they matter in practice:

- they improve performance attribution,
- they help distinguish skill from factor exposure,
- they provide a richer expected-return and risk-budgeting vocabulary than CAPM alone.

---

## 8. Consumption-based asset pricing and ICAPM

### 8.1 Consumption CAPM (CCAPM)

A canonical economic microfoundation uses investor utility over consumption. The SDF takes the form:

$$
m_{t+1} = \delta \frac{u'(C_{t+1})}{u'(C_t)}
$$

where:

- $\delta$ is a subjective discount factor,
- $u'(C_t)$ is marginal utility of consumption.

The intuition is deep and simple:

- assets that pay off in bad consumption states hedge the investor,
- those assets command lower expected returns,
- assets that do badly in bad times require higher premia.

### 8.2 Intertemporal CAPM (ICAPM)

Merton’s intertemporal CAPM extends the static CAPM idea when investment opportunities change over time. Investors care
not only about terminal wealth but also about future shifts in:

- interest rates,
- expected returns,
- volatility,
- macro state variables.

So assets can earn premia for exposure to the market portfolio **and** for exposure to state variables that affect
future opportunity sets.

Why this matters for practitioners:

- macro and rates desks naturally think in these terms,
- hedging demand for future funding conditions or growth/inflation regimes is an intertemporal idea,
- many term-structure and macro-factor frameworks are closer to ICAPM than to static CAPM.

---

## 9. Black-Scholes-Merton and risk-neutral pricing

CAPM is about **expected returns in equilibrium**. Black-Scholes-Merton is about
**arbitrage-free pricing of replicable contingent claims**.

For an underlying following geometric Brownian motion,

$$
dS_t = \mu S_t dt + \sigma S_t dW_t
$$

the key no-arbitrage result is that derivative prices do **not** depend on the real-world drift $\mu$ once replication
is possible. Instead valuation uses the risk-free rate under a risk-neutral measure.

For payoff $X_T$ at maturity $T$:

$$
V_t = B_t \; \mathbb{E}_t^{\mathbb{Q}}\left[\frac{X_T}{B_T}\right]
$$

where:

- $\mathbb{Q}$ is the risk-neutral measure,
- $B_t$ is the money-market numeraire.

Why Black-Scholes-Merton is foundational:

- it formalized dynamic hedging and replication,
- it separated real-world forecasting from arbitrage-free valuation,
- it remains the conceptual basis for modern derivatives engines.

In production, the exact Black-Scholes assumptions usually fail, but the risk-neutral pricing architecture remains
central.

A closely related static no-arbitrage identity is **put-call parity**. While Black-Scholes-Merton needs a diffusion
model, put-call parity does not: it comes directly from static replication. For a practitioner-oriented treatment of
parity, carry adjustments, synthetic forwards, exercise-style caveats, and numerical examples, see
`docs/pricing/volatility/OPTION_PRICING_FOUNDATIONS_AND_PUT_CALL_PARITY.md`.

---

## 10. Fixed-income and term-structure models

Rates valuation and risk management require models of discount curves, forward curves, and the evolution of the yield
curve.

### 10.1 Static curve representation

Common static representations include:

- zero-rate curves,
- discount-factor curves,
- instantaneous forward curves,
- parsimonious parameterizations such as Nelson-Siegel or Svensson.

These are useful for:

- bootstrapping curves from market instruments,
- interpolation and extrapolation,
- scenario generation,
- sensitivity and hedging.

### 10.2 Dynamic term-structure models

Important dynamic families include:

- **short-rate models**: Vasicek, CIR, Hull-White,
- **Heath-Jarrow-Morton (HJM)** models for forward rates,
- **affine term-structure models** where yields are affine in state variables,
- macro-finance models that link rates to inflation and growth dynamics.

Why this matters:

- rates books are driven by level, slope, and curvature factors,
- scenario engines need coherent curve moves,
- valuation requires arbitrage-free discounting and projection.

For implementation-facing notes in this repository, see:

- `docs/pricing/rates/YIELD_CURVE_AND_OIS_CONSTRUCTION.md`
- `docs/pricing/rates/RATES_FACTORS_AND_CURVE_MODELS.md`
- `docs/pricing/rates/DETAILED_YIELD_CURVE_IMPLEMENTATION.md`

---

## 11. Credit risk models

Two broad economic families dominate credit modeling.

### 11.1 Structural models

Structural models treat firm value as the driver of default. In the Merton model, equity is effectively a call option on
firm assets with debt as the strike.

Default occurs when asset value is insufficient relative to debt at maturity or at barrier-like trigger conditions in
extensions.

Why it matters:

- gives an economic link between balance-sheet structure and default risk,
- connects credit spreads to asset value volatility and leverage,
- useful conceptually for capital-structure reasoning.

### 11.2 Reduced-form or intensity models

Reduced-form models specify a default intensity or hazard rate directly. Survival probability over a horizon is then
driven by the integrated hazard rate.

These models are often more practical for market calibration because:

- CDS spreads map naturally to hazard-rate curves,
- calibration is simpler than structural balance-sheet inference,
- they fit trading and risk infrastructure well.

For implementation-facing notes in this repository, see:

- `docs/pricing/credit/CREDIT_CURVE_CONSTRUCTION.md`
- `docs/pricing/credit/CDS_CURVES_AND_CREDIT_RISK_IN_PRACTICE.md`

---

## 12. Black-Litterman and modern portfolio construction

Modern buy-side portfolio construction often starts from the Markowitz setup but tries to fix the instability of raw
expected-return estimates.

Black-Litterman does this by combining:

- an equilibrium prior, often reverse-engineered from market-cap weights,
- investor views,
- uncertainty about those views.

This is useful because naive mean-variance optimization is extremely sensitive to small changes in expected returns.

Interpretation:

- CAPM provides a market-implied equilibrium anchor,
- Black-Litterman adds controlled deviations from that anchor,
- the result is more stable portfolio construction.

Even when not implemented explicitly, this logic appears in many production allocation workflows through shrinkage,
priors, Bayesian blending, and robust optimization.

---

## 13. What each model is actually used for

A practical summary:

| Model family             | Main question answered                                      | Typical production use                                        |
|--------------------------|-------------------------------------------------------------|---------------------------------------------------------------|
| DCF / present value      | What is the value of cash flows after discounting?          | Bond valuation, project valuation, NPV, accrual products      |
| CAPM                     | What is a simple required return for equity risk?           | Cost of equity, benchmarking, performance attribution         |
| APT / factor models      | Which common risk drivers explain returns?                  | Risk decomposition, stress design, expected-return modeling   |
| Fama-French style models | Which empirical style factors drive cross-sections?         | Equity attribution, PM risk budgeting, manager evaluation     |
| CCAPM / ICAPM            | Why are some state exposures priced in equilibrium?         | Macro intuition, research frameworks, long-horizon allocation |
| Black-Scholes-Merton     | What is the arbitrage-free value of a hedgeable derivative? | Options pricing, Greeks, hedging infrastructure               |
| Term-structure models    | How should rates curves be represented and evolved?         | IR pricing, scenario engines, XVA/risk, stress testing        |
| Structural credit        | How does firm-value risk map to default?                    | Economic credit intuition, structural analysis                |
| Reduced-form credit      | How do observed spreads map to default intensities?         | CDS calibration, CVA/credit pricing, survival curves          |
| Black-Litterman          | How to combine equilibrium with views robustly?             | Strategic allocation, robust portfolio construction           |

---

## 14. How these models fit together in a modern platform

A good architecture usually uses them in combination:

1. **Market data and curves** establish discount factors, forwards, survival curves, and volatility inputs.
2. **No-arbitrage pricing models** value instruments and produce sensitivities.
3. **Factor and scenario models** explain and stress portfolio PnL.
4. **Expected-return models** such as CAPM or multifactor approaches support capital allocation, hurdle rates, or
   strategic portfolio decisions.
5. **Governance and validation** check whether model assumptions and calibration choices are appropriate for the use
   case.

This is why a quant/risk repository should not treat CAPM, Black-Scholes, rates models, and credit models as competing
alternatives. They answer different but connected questions.

---

## 15. Practical cautions

### 15.1 Don’t mix physical and risk-neutral quantities carelessly

- **physical / real-world measure** models are used for forecasting and scenario generation,
- **risk-neutral measure** models are used for arbitrage-free pricing.

Confusing the two leads to serious valuation and risk errors.

### 15.2 Don’t use one-factor logic where factor structure is rich

CAPM can be too crude for:

- rates books,
- multi-asset macro portfolios,
- credit portfolios,
- style-driven equity books.

### 15.3 Don’t ignore estimation error

Theoretical models are often cleaner than the data.

In production, the limiting factor is often:

- unstable betas,
- noisy expected returns,
- fragile covariance matrices,
- non-stationary regimes,
- liquidity and model-risk constraints.

### 15.4 Model choice must match purpose

Ask first:

- Is the goal **valuation**, **hedging**, **risk aggregation**, **expected-return estimation**, or
  **strategic allocation**?

That question usually determines the right model family better than theoretical elegance alone.

---

## 16. Minimal mental map

A useful compact map is:

- **Markowitz**: how portfolios combine return and variance,
- **CAPM**: market beta as the priced risk in the simplest equilibrium model,
- **APT / factor models**: several systematic factors can be priced,
- **SDF / CCAPM / ICAPM**: general equilibrium language for why state exposures matter,
- **Black-Scholes-Merton**: arbitrage-free derivative pricing under replication,
- **term-structure and credit models**: domain-specific no-arbitrage and risk models for rates and credit,
- **Black-Litterman**: robust portfolio construction on top of equilibrium ideas.

That set covers much of the conceptual foundation underlying modern valuation and risk management.
