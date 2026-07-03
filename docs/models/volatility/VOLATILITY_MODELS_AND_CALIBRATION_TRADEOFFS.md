# Volatility Models and Calibration Trade-offs

This chapter describes the main model families used after a volatility surface has been constructed. The organizing
criteria are:

- fit to observable vanilla prices;
- plausibility of future smile dynamics;
- hedge behavior;
- numerical stability;
- calibration cost;
- implementation and validation burden.

Model selection is not a search for a universal best model. It is a constrained engineering problem:

$$
\text{choose a model family that balances static fit, dynamic behavior, numerical stability, and operational control}.
$$

## 1. Surface Object and Dynamics Model

The implied-volatility surface and the dynamics model are distinct objects.

The **surface** encodes market-implied vanilla option prices at the valuation time. It is a market-data object.

The **dynamics model** specifies how the underlying, volatility, rates, and other state variables evolve between the
valuation time and maturity. It is a pricing-model object.

This distinction matters for:

- path-dependent options;
- forward-start options;
- callable or early-exercise structures;
- scenario analysis;
- hedging over time.

The platform should therefore store the surface, model family, calibration configuration, numerical engine, and scenario
semantics separately.

## 2. Constant-Volatility Benchmark Models

### 2.1 Black-Scholes-Merton

The Black-Scholes-Merton model assumes:

$$
dS_t = \mu S_t \, dt + \sigma S_t \, dW_t,
$$

with constant $\sigma$.

Advantages:

- closed form for European vanilla options;
- transparent Greeks;
- standard benchmark language;
- low implementation cost.

Limitations:

- cannot reproduce volatility smile or skew;
- assumes one volatility across strikes and maturities;
- gives limited dynamics for exotic hedging.

Typical uses:

- benchmark valuation;
- sanity checks;
- first-order risk explanations;
- liquid vanilla infrastructure.

### 2.2 Black 76 and Bachelier

Black 76 applies a constant lognormal volatility assumption to forwards or futures. Bachelier uses normal dynamics:

$$
dF_t = \sigma_N dW_t.
$$

Bachelier is particularly useful when the underlying can be near zero or negative. This is common in rates and some
commodity contexts.

## 3. Local Volatility

Local volatility assumes:

$$
dS_t = (r-q)S_t \, dt + \sigma_{\text{loc}}(t,S_t) S_t \, dW_t.
$$

The function $\sigma_{\text{loc}}(t,S)$ is calibrated so that the model reproduces the full vanilla surface at the
valuation time.

Advantages:

- exact fit to the current vanilla surface under ideal data conditions;
- transparent one-factor diffusion structure;
- natural implementation through finite differences or Monte Carlo;
- useful benchmark for equity and FX exotics.

Limitations:

- future smile dynamics are mechanically implied by the one-factor diffusion;
- hedge behavior can be poor if observed volatility is itself stochastic;
- calibration requires arbitrage-clean surface inputs.

Implementation interpretation:

Local volatility is a surface-consistent model. It is not automatically a superior hedging model.

## 4. Stochastic Volatility

In stochastic-volatility models, the underlying and volatility evolve jointly. A standard example is Heston:

$$
dS_t = (r-q)S_t \, dt + \sqrt{v_t} S_t \, dW_t^{(1)},
$$

$$
dv_t = \kappa(\theta-v_t)dt + \xi \sqrt{v_t} \, dW_t^{(2)},
$$

with:

$$
dW_t^{(1)} dW_t^{(2)} = \rho \, dt.
$$

Advantages:

- captures skew and smile through stochastic variance and correlation;
- can produce more plausible forward-smile behavior than local volatility;
- can improve hedging for path-dependent exotics.

Limitations:

- calibration is slower and more fragile than constant-volatility benchmarks;
- parameter stability can be poor;
- exact fit to the full vanilla surface is not automatic;
- controls and validation are more extensive.

## 5. Local-Stochastic Volatility

Local-stochastic-volatility models combine:

- a local-volatility component for surface fit;
- a stochastic-volatility component for richer dynamics.

The intended decomposition is:

$$
\text{LSV} = \text{surface fit} + \text{stochastic smile dynamics}.
$$

Advantages:

- close fit to the current surface;
- richer forward-smile behavior;
- broad product coverage for exotic books.

Limitations:

- materially heavier calibration;
- more complex numerical methods;
- larger governance and validation burden.

## 6. SABR and Parametric Smile Models

SABR is widely used for rates and FX smile interpolation. A stylized form is:

$$
dF_t = \alpha_t F_t^{\beta} dW_t^{(1)},
$$

$$
d\alpha_t = \nu \alpha_t dW_t^{(2)},
$$

with:

$$
dW_t^{(1)} dW_t^{(2)} = \rho \, dt.
$$

Parameter interpretation:

- $\alpha$ controls volatility level;
- $\beta$ controls backbone shape;
- $\nu$ controls volatility of volatility;
- $\rho$ controls skew direction and magnitude.

Advantages:

- compact parametric smile;
- interpretable parameters;
- effective fit for cap, floor, and swaption smiles;
- compatibility with Black, shifted-Black, and normal quoting conventions.

Limitations:

- asymptotic formulas have approximation error;
- extrapolation requires explicit controls;
- parameter instability can occur in illiquid regions;
- SABR is a smile model, not a universal dynamics model.

## 7. Callable and Early-Exercise Rates Products

For callable and Bermudan rates products, model selection depends on early-exercise behavior, rate evolution, and
calibration stability rather than smile fit alone.

Common model families include:

- Hull-White;
- LGM;
- Black-Karasinski;
- short-rate trees;
- LMM where forward-rate consistency and correlation structure are material.

Advantages:

- support for lattice methods and backward induction;
- stable exercise-policy representation;
- direct connection between exercise decisions and term-structure moves.

Limitations:

- full smile calibration may be limited;
- additional smile overlays may be required;
- some models require adaptation in negative-rate environments.

## 8. Calibration Design

### 8.1 Exact Fit and Stable Fit

Some models can fit the current surface almost exactly. Others produce a smoother approximate fit.

An exact fit reduces vanilla repricing error but can introduce parameter noise. A stable fit reduces calibration noise
but may leave systematic residuals.

The platform should store both:

$$
\text{calibration residual} =
\text{model price} - \text{market price},
$$

and parameter stability diagnostics across calibration dates.

### 8.2 Node-Based and Parametric Calibration

Node-based calibration is flexible and captures local structure. It can also be noisy and difficult to extrapolate.

Parametric calibration is compact and easier to validate. It can miss local market structure.

The choice should be explicit in configuration.

### 8.3 Static Fit and Dynamic Plausibility

A model can fit the current surface and still imply poor future smile behavior. This is the main distinction between:

- static fit;
- dynamic plausibility.

Local volatility often prioritizes static fit. Stochastic volatility often improves dynamic plausibility. LSV attempts to
combine both at a higher implementation cost.

## 9. Risk and Hedging Implications

Model choice affects:

- delta and gamma;
- vega bucketing;
- vanna and volga;
- scenario behavior;
- cross-gamma and vol-of-vol exposure;
- PnL explain residuals.

Examples:

- local volatility may give surface-consistent vanilla deltas;
- stochastic volatility may give more plausible vanna and volga behavior;
- a short-rate tree may represent callable exercise logic more reliably than a model with better smile fit but poor
  exercise representation.

Model selection is therefore a risk-design choice as well as a pricing choice.

## 10. Asset-Class Model Patterns

| Asset class | Common implementation pattern | Main control point |
|---|---|---|
| Equity | Black-Scholes benchmark, local volatility, Heston, LSV | Smile dynamics and exotic hedging |
| FX | Delta-based smiles, Black-style surfaces, local/stochastic volatility | Delta convention and sticky-rule semantics |
| Rates | Black, normal, shifted lognormal, SABR, Hull-White, LGM | Negative-rate treatment and exercise modeling |
| Commodities | Futures-based models with term-structure-aware volatility | Seasonality and delivery-period structure |

## 11. Repository Design Implications

The platform should represent separately:

- observed volatility surface;
- calibration input set;
- calibration objective and weights;
- model family;
- model parameters;
- numerical engine;
- scenario semantics;
- risk-factor mapping.

This separation prevents market-data representation from being silently coupled to one pricing model.

## 12. Summary

No single volatility model is simultaneously:

- exact on every market quote;
- dynamically plausible;
- stable for hedging;
- computationally inexpensive;
- simple to validate.

Production systems therefore use model stacks. The implementation objective is to make the trade-offs explicit,
versioned, testable, and controllable.
