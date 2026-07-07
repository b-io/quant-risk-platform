# Time-Dependent Volatility and Mean-Reversion Models for the Term Structure

This chapter extends the static curve-construction notes into dynamic short-rate and forward-rate models. The central
distinction is between:

- the **market curve**, built from observable instruments at the valuation time;
- the **dynamic model**, used for optionality, exposure simulation, scenario generation, and path-dependent valuation.

The dynamic layer introduces model choices that are absent from a purely static bootstrap:

- mean reversion;
- time-dependent volatility;
- state-dependent volatility;
- positivity constraints;
- exact fit to the initial term structure;
- calibration tractability.

## 1. Static Curves and Dynamic Models

A rates platform usually separates two layers.

The static layer builds discount and projection curves from market quotes:

- overnight-indexed discount curves;
- tenor-specific projection curves;
- basis curves where required.

The dynamic layer specifies how rates evolve after the valuation time. This layer is required for:

- callable and Bermudan structures;
- exposure simulation;
- path-dependent valuation;
- rate scenario generation;
- option pricing beyond static Black-style formulas.

The dynamic model should consume the built market state. It should not replace the market-state bootstrap used for
ordinary valuation and sensitivities.

## 2. General Short-Rate Template

A useful umbrella specification is:

$$
dr_t = \mu(t,r_t)dt + \sigma(t)g(r_t)dW_t.
$$

The function $\mu(t,r)$ controls drift and mean reversion. The function $\sigma(t)$ controls the volatility term
structure. The state function $g(r)$ controls whether volatility is normal, square-root, or proportional to the rate.

Common choices are:

| Model class | $g(r_t)$ | Main implication |
|---|---:|---|
| Normal | $1$ | absolute basis-point volatility |
| Square-root | $\sqrt{r_t}$ | damped volatility near zero |
| Lognormal | $r_t$ | proportional volatility and positive rates |

This decomposition is useful in implementation because it separates level behavior, mean reversion, and volatility
shape.

## 3. Time-Dependent Volatility

A simple specification is:

$$
\sigma(t) = \sigma_0 e^{-\lambda t},
\qquad \lambda > 0.
$$

This produces front-loaded volatility: short horizons receive larger shocks than long horizons. It can be combined with
normal, square-root, or lognormal state dynamics.

For example, if:

$$
\sigma(t)=1.5\%e^{-0.7t},
$$

then volatility declines from $1.50\%$ at $t=0$ to approximately $0.37\%$ at $t=2$.

## 4. Ho-Lee

The Ho-Lee model is:

$$
dr_t = \theta(t)dt + \sigma dW_t.
$$

Properties:

- normal-rate dynamics;
- exact initial-curve fit through $\theta(t)$;
- no mean reversion;
- possible negative rates;
- simple lattice implementation.

Ho-Lee is useful as a baseline model because it isolates the effect of exact curve fit without mean reversion. Its main
limitation is that shocks do not naturally decay.

## 5. Hull-White

The one-factor Hull-White model is:

$$
dr_t = \left(\theta(t)-a r_t\right)dt + \sigma(t)dW_t.
$$

Properties:

- exact fit to the initial curve through $\theta(t)$;
- mean reversion controlled by $a$;
- optional time-dependent volatility;
- Gaussian rates.

For the stochastic component, the horizon variance is:

$$
\mathrm{Var}(r_t)
=
\int_0^t e^{-2a(t-u)}\sigma(u)^2du.
$$

Mean reversion and volatility decay therefore both reduce long-horizon uncertainty.

Hull-White is often a natural first dynamic model when the implementation requires curve fit, tractability, and
tree-based valuation.

## 6. CIR and Square-Root Models

The CIR model is:

$$
dr_t = a(b-r_t)dt + \sigma\sqrt{r_t}dW_t.
$$

Properties:

- mean reversion to $b$;
- state-dependent volatility;
- positive-rate orientation;
- affine bond-pricing structure.

The diffusion term shrinks as the rate approaches zero. This is useful for positivity, but the low-dimensional parameter
set limits fit to a full curve and volatility surface. Shifted extensions such as CIR++ can improve curve fit while
retaining square-root dynamics.

Courtadon-style models belong to the same square-root mean-reverting family and are best treated as historical and
conceptual relatives of CIR.

## 7. Lognormal and Black-Karasinski Models

A simple proportional-volatility short-rate model is:

$$
dr_t = \alpha r_tdt + \sigma r_tdW_t.
$$

It preserves positivity but lacks explicit mean reversion.

Black-Karasinski introduces mean reversion in log-rate space:

$$
d\ln r_t = \left(\theta(t)-\phi\ln r_t\right)dt+\sigma(t)dW_t.
$$

Properties:

- positive short rates;
- mean reversion of the log-rate;
- clean use with tree-based callable valuation;
- more numerical calibration than Gaussian models.

The main limitation is behavior near zero or negative-rate regimes, where a strictly lognormal state variable can become
inconsistent with market conventions.

## 8. Black-Derman-Toy

Black-Derman-Toy is a lognormal short-rate tree framework with time-dependent volatility. It is useful when:

- the volatility term structure is the calibration target;
- a recombining tree is natural for the product;
- interpretability is preferred over high-dimensional factor richness.

Limitations include stylized dynamics, limited multi-factor structure, and weaker natural integration with modern
multi-curve and smile frameworks.

## 9. Calibration Dimensionality

Low-dimensional equilibrium models can be economically coherent and still fail to reproduce a full market curve or
volatility surface. The issue is calibration dimensionality.

A basic CIR specification has only a few parameters:

- mean-reversion speed $a$;
- long-run level $b$;
- volatility scale $\sigma$;
- optional shift.

That parameter set is often insufficient to fit:

- the full initial discount curve;
- caplet or swaption volatility term structures;
- correlations across maturities;
- payer and receiver smile asymmetry;
- multi-curve basis effects.

A piecewise-volatility Hull-White model has more calibration freedom. A forward-rate market model such as LMM has still
more freedom because it models a vector of forward rates and their correlations.

## 10. Model-Selection Criteria

| Requirement | Natural model family |
|---|---|
| Exact initial curve fit with tractability | Ho-Lee or Hull-White |
| Mean reversion with Gaussian rates | Hull-White or LGM |
| Positivity with affine structure | CIR or shifted CIR |
| Positivity with tree calibration | Black-Karasinski or BDT |
| Callable and Bermudan valuation | Hull-White, LGM, Black-Karasinski, BDT |
| Liquid caplet or swaption surface calibration | Black/Bachelier with SABR, LMM, or market-model variants |
| Multi-factor forward-rate correlation | LMM or multi-factor Gaussian models |
| Exposure simulation with parsimonious factors | Hull-White or low-dimensional Gaussian models |

The calibration object should be explicit. Fitting a curve, fitting an ATM volatility term structure, fitting a smile,
and fitting a correlation structure are different mathematical problems.

## 11. Repository Design Implications

The platform should represent the following objects separately:

- built market curve;
- model family;
- model parameters;
- volatility function;
- state transformation;
- calibration target;
- calibration residuals;
- simulation discretization;
- valuation engine.

A short-rate model configuration should include:

- family name;
- mean-reversion parameters;
- long-run level or curve-fit drift representation;
- volatility function type and parameters;
- factor dimension;
- positivity or state-transform flag;
- calibration timestamp and market source.

## 12. Summary

The progression beyond Ho-Lee is the introduction of:

- mean reversion;
- time-dependent volatility;
- state-dependent volatility;
- positivity constraints;
- richer calibration targets.

The implementation should not hide these choices inside a pricing engine. They are model-state choices and should be
stored, versioned, validated, and linked to the market snapshot that produced them.
