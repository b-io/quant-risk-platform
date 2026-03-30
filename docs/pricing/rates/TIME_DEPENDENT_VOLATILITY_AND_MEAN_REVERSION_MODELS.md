# Time-Dependent-Volatility and Mean-Reversion Models for the Term Structure

This note extends the basic rates-model discussion beyond simple non-mean-reverting specifications such as Ho-Lee.

The practical goal is to explain the model families that desks use once they want at least one of the following:

- **mean reversion**,
- **state-dependent volatility**,
- **time-dependent volatility**,
- **positivity of rates**,
- **exact fit to the initial curve**,
- **a workable compromise between calibration quality and numerical speed**.

It complements:

- `docs/pricing/rates/YIELD_CURVE_AND_OIS_CONSTRUCTION.md` for today's market curve construction,
- `docs/pricing/rates/RATES_FACTORS_AND_CURVE_MODELS.md` for the wider cross-asset model-selection picture,
- `docs/theory/MEAN_REVERSION_AND_SHORT_RATE_MODELS.md` for the mathematical intuition behind the model family.

---

## 1. Static curves versus dynamic models

A production rates stack usually separates two layers.

### Static layer

Build today's discount and projection curves from market quotes.

Examples:

- SOFR OIS curve for USD discounting,
- €STR OIS curve for EUR discounting,
- tenor-specific forward curves for projection.

### Dynamic layer

Model how rates evolve through time for:

- callable or Bermudan structures,
- Monte Carlo exposure,
- rate scenario generation,
- stress extensions,
- rates option pricing beyond simple Black-style approximations.

The models in this note live in the **dynamic layer**.

---

## 2. A useful organizing template

A convenient umbrella specification is

$$
dr_t = \mu(t,r_t)dt + \sigma(t) g(r_t) dW_t
$$

Different model families correspond to different choices of $\mu$ and $g$.

### Normal models

$$
g(r_t) = 1
$$

Absolute basis-point volatility is independent of the rate level.

### Square-root models

$$
g(r_t) = \sqrt{r_t}
$$

Volatility falls when rates are near zero and rises when rates are high.

### Lognormal models

$$
g(r_t) = r_t
$$

Volatility is proportional to the level of the rate.

This single template is a good design lens for documentation and implementation because it separates:

- **level behavior**,
- **mean reversion**, and
- **volatility shape**.

---

## 3. Time-dependent volatility

A simple and very common time-dependent specification is

$$
\sigma(t) = \sigma_0 e^{-\lambda t}
$$

with $\lambda > 0$.

Interpretation:

- the front end of the horizon is volatile,
- longer-dated incremental shocks are smaller,
- the model produces a decaying volatility term structure.

This can be combined with normal, square-root, or lognormal rate dynamics.

### Example for Section 3 — decay profile

Suppose

$$
\sigma(t) = 1.5\% \times e^{-0.7t}
$$

Then the instantaneous volatility is approximately:

- $1.50\%$ at $t=0$,
- $0.745\%$ at $t=1$,
- $0.370\%$ at $t=2$,
- $0.045\%$ at $t=5$.

This is exactly the type of term structure used when the desk wants short-end uncertainty to dominate long-horizon uncertainty.

---

## 4. Ho-Lee as the baseline non-mean-reverting model

The pedagogical baseline is

$$
dr_t = \theta(t)dt + \sigma dW_t
$$

Main properties:

- normal-rate model,
- can fit today's initial curve through $\theta(t)$,
- no mean reversion,
- negative rates are possible,
- useful for intuition and simple lattice examples.

### Example for Section 4 — one-year Ho-Lee scenario

Suppose:

- $r_0 = 2.0\%$,
- $\theta = 0.2\%$ per year,
- $\sigma = 1.0\%$ per square-root-year,
- $t = 1$ year,
- simulated Brownian draw $W_1 = 0.5$.

Then

$$
r_1 = 0.0200 + 0.0020 + 0.0100 \times 0.5 = 0.0270
$$

So the one-year short rate in that scenario is **2.70%**.

The limitation is that a temporary shock does not naturally decay.

---

## 5. Hull-White with time-dependent volatility

A practical next step is the one-factor Hull-White model

$$
dr_t = (\theta(t) - a r_t)dt + \sigma(t)dW_t
$$

This is often the first model to use in production when one wants:

- exact fit to today's initial term structure,
- mean reversion,
- time-dependent volatility.

### Why it is important

- $\theta(t)$ absorbs the current curve.
- $a$ controls how quickly shocks decay.
- $\sigma(t)$ shapes the volatility term structure.

### Variance with exponential decay

If

$$
\sigma(t) = \sigma_0 e^{-\lambda t}
$$

then for the stochastic part of $r_t$ the variance at horizon $t$ is

$$
\operatorname{Var}(r_t) = \int_0^t e^{-2a(t-u)}\sigma(u)^2du
$$

So mean reversion and volatility decay both reduce long-horizon uncertainty.

### Example for Section 5 — decaying-volatility Hull-White uncertainty

Suppose:

- $a = 0.6$,
- $\sigma(t) = 1.5\% e^{-0.4t}$,
- horizon $t = 2$ years.

Then the implied standard deviation of the stochastic component of $r_2$ is about **0.791%**.

For comparison, with the same initial volatility but constant $\sigma(t)=1.5\%$, the corresponding standard deviation would be about **1.306%**.

So exponential volatility decay produces materially tighter long-horizon rate distributions.

### Example for Section 5 — expected reversion of the short rate

Suppose:

- current short rate $r_0 = 3.0\%$,
- mean-reversion speed $a = 0.5$,
- effective long-run level around $2.5\%$.

Then the mean-reversion logic gives an expected one-year rate of roughly

$$
2.5\% + (3.0\% - 2.5\%)e^{-0.5} \approx 2.803\%
$$

So the model says the rate should drift back toward the center even if the short-horizon volatility is still meaningful.

---

## 6. CIR: mean reversion plus level-dependent volatility

The CIR model is

$$
dr_t = a(b-r_t)dt + \sigma \sqrt{r_t} dW_t
$$

This is the classical way to combine:

- mean reversion,
- positive-rate behavior,
- volatility that depends on the level of the rate.

### Interpretation

- When rates are high, the volatility term is larger.
- When rates approach zero, the diffusion shrinks.
- The process therefore behaves very differently from a Gaussian model in low-rate regimes.

### Example for Section 6 — expected rate under CIR

Suppose:

- $r_0 = 5.0\%$,
- $b = 3.0\%$,
- $a = 1.0$,
- horizon $t = 1$ year.

Then

$$
\mathbb{E}[r_1] = 0.05e^{-1} + 0.03(1-e^{-1}) \approx 3.736\%
$$

### Example for Section 6 — one quarterly CIR move

Take:

- $r_t = 4.0\%$,
- $a = 0.8$,
- $b = 3.0\%$,
- $\sigma = 15\%$,
- $\Delta t = 0.25$,
- $Z = -0.5$.

A simple discrete illustration gives

$$
r_{t+\Delta t} \approx 3.05\%
$$

So a downward move does not explode in size as the rate approaches zero because the diffusion is damped by $\sqrt{r_t}$.

### When CIR is attractive

CIR is attractive when positivity and affine tractability matter more than exact point-by-point fit to today's curve.

For curve-fitting to today's market one often prefers shifted extensions such as CIR++.

---

## 7. Courtadon model

The Courtadon model is often presented as an early mean-reverting square-root short-rate model:

$$
dr_t = k(\theta-r_t)dt + \sigma\sqrt{r_t} dW_t
$$

From a practical point of view, its behavior is very close to CIR:

- mean reversion to a target level,
- square-root level-dependent volatility,
- positive-rate orientation.

In a modern documentation set it is best described as part of the **square-root mean-reverting family**, with historical importance in the bond-option literature.

### Example for Section 7 — Courtadon-style scenario step

Suppose:

- $r_t = 4.0\%$,
- $k = 0.9$,
- $\theta = 3.0\%$,
- $\sigma = 12\%$,
- $\Delta t = 0.25$,
- $Z = 0.7$.

Then an Euler-style step gives approximately

$$
r_{t+\Delta t} \approx 4.615\%
$$

The model behaves like a square-root version of mean-reverting short-rate dynamics rather than like a normal Gaussian model.

---

## 8. Lognormal short-rate model

A classical proportional-volatility short-rate model is

$$
dr_t = \alpha r_t dt + \sigma r_t dW_t
$$

This is not the same as Black-Karasinski because the basic version has no explicit mean reversion.

### Main intuition

- rates remain positive,
- volatility is proportional to the rate level,
- relative moves are more natural than absolute basis-point moves,
- high-rate regimes can become unrealistically explosive if the model is used without further structure.

### Example for Section 8 — one quarterly lognormal move

Suppose:

- $r_0 = 3.0\%$,
- $\alpha = 1.0\%$,
- $\sigma = 20\%$,
- $\Delta t = 0.25$,
- $Z = 0.5$.

Then

$$
r_{t+\Delta t} = r_t \exp\left((\alpha - \tfrac{1}{2}\sigma^2)\Delta t + \sigma\sqrt{\Delta t} Z\right)
$$

which gives approximately

$$
r_{t+\Delta t} \approx 3.146\%
$$

---

## 9. Black-Karasinski

Black-Karasinski adds **mean reversion** to a **lognormal-rate** framework.

A standard representation is

$$
d\ln r_t = (\theta(t) - \phi \ln r_t)dt + \sigma(t)dW_t
$$

This can be viewed as the lognormal analogue of Hull-White:

- Hull-White mean reverts the **rate** in normal space,
- Black-Karasinski mean reverts the **log rate** in log space.

### Why practitioners like it

- positivity of the short rate,
- flexible time-dependent drift and volatility,
- good fit for lattice implementations of callable and Bermudan products.

### Why it is harder

- less analytical tractability than Hull-White or CIR,
- numerical calibration is typically required for bond prices and trees.

### Example for Section 9 — mean reversion in log-rate space

Suppose:

- current short rate $r_0 = 4.0\%$,
- long-run level $\bar r = 2.5\%$,
- $\phi = 0.7$,
- $\sigma = 20\%$,
- horizon $t = 1$ year.

Then the median future rate implied by the mean of the log-rate is approximately

$$
3.157\%
$$

So the model pulls the rate downward toward the long-run target while keeping the short rate strictly positive.

---

## 10. Black-Derman-Toy and exponentially decaying volatility

The Black-Derman-Toy family is a lognormal short-rate tree model with time-dependent volatility.

A continuous-time representation of the time-dependent-volatility lognormal idea can be written in terms of the log short rate.
When the local volatility decays through time, for example with

$$
\sigma(t) = \sigma_0 e^{-\lambda t}
$$

the model produces a front-loaded volatility term structure.

This is useful when:

- short-maturity implied vols are much higher than long-maturity vols,
- the desk wants a positive-rate tree model,
- the calibration objective is primarily option-implied volatility term structure.

### Example for Section 10 — front-loaded lognormal volatility

Suppose a positive-rate tree is calibrated with

$$
\sigma(t) = 25\% e^{-0.6t}
$$

Then:

- the 1Y instantaneous volatility is about **13.72%**,
- the 3Y instantaneous volatility is about **4.13%**,
- the 5Y instantaneous volatility is about **1.24%**.

So most of the uncertainty sits in the front of the tree.

---

## 11. Trade-offs across the main model families

The important practical question is not only whether a model is mathematically elegant, but also what it gives up.

### Ho-Lee

**Strengths**

- simplest no-arbitrage short-rate model,
- exact fit to the initial curve,
- easy trees and intuition.

**Trade-offs**

- no mean reversion,
- long-horizon variance can become unrealistically large,
- poor structural realism for medium- and long-dated scenarios.

### Hull-White

**Strengths**

- exact fit to the initial curve,
- mean reversion,
- supports time-dependent volatility,
- fast enough for trees, lattices, callable products, and exposure engines.

**Trade-offs**

- normal-rate dynamics mean negative rates are possible,
- one-factor versions are too simple to reproduce the full correlation structure of the curve,
- one-factor versions do not fit a full swaption smile by themselves.

### CIR / Courtadon

**Strengths**

- positivity-oriented dynamics,
- level-dependent volatility,
- affine structure and relatively clean bond-pricing formulas.

**Trade-offs**

- the square-root diffusion is quite restrictive,
- basic versions do not fit today's curve exactly,
- even shifted variants are still relatively inflexible compared with piecewise-volatility Hull-White, LGM, or LMM-style frameworks,
- calibration to a full swaption surface or cube is often too rigid for active trading books.

This is the core reason why practitioners often say that **CIR is elegant but too restrictive for full market calibration**.

### Simple lognormal short-rate model

**Strengths**

- positivity,
- intuitive proportional volatility.

**Trade-offs**

- no mean reversion,
- can become unstable or implausibly explosive in high-rate regimes,
- rarely used as a full production model on its own.

### Black-Karasinski

**Strengths**

- positivity,
- mean reversion,
- time-dependent calibration flexibility,
- useful for tree-based callable or Bermudan products.

**Trade-offs**

- more numerical work than Hull-White,
- calibration and Greeks are usually less transparent,
- behavior becomes awkward when the market regime spends a long time near zero or below zero.

### BDT-style trees

**Strengths**

- direct calibration to a volatility term structure,
- recombining tree representation,
- intuitive for callable cash-flow products.

**Trade-offs**

- usually too stylized for richer multi-factor hedging,
- not the natural choice for large modern cross-curve or XVA platforms,
- smile and basis extensions are not as natural as in market-model approaches.

---

## 12. Why CIR is often too restrictive for trading-desk calibration

The key issue is **dimensionality and flexibility**.

A basic CIR setup has only a few parameters:

- mean-reversion speed $a$,
- long-run level $b$,
- volatility scale $\sigma$,
- possibly a shift if a shifted extension is used.

That is elegant, but it is not much flexibility when the desk wants to fit:

- the full initial discount curve,
- the term structure of caplet or swaption volatilities,
- correlations across maturities,
- payer/receiver smile asymmetry,
- multi-curve basis effects.

A one-factor Gaussian model with piecewise volatility already has more calibration freedom.
A forward-rate market model such as LMM has much more freedom again because it models a vector of forward rates and their correlations directly.

### Example for Section 12 — exact curve fit versus equilibrium fit

Suppose today's zero curve is:

- 1Y zero = **2.00%**,
- 5Y zero = **2.80%**,
- 10Y zero = **3.15%**.

A Hull-White model with curve-implied drift can be built so that those three discount points are matched exactly at time 0.
So its curve-fit error at those nodes is **0 bp** by construction.

Now suppose an equilibrium-style square-root fit gives:

- 1Y model zero = **2.14%**,
- 5Y model zero = **2.67%**,
- 10Y model zero = **2.96%**.

Then the node errors are:

- **+14 bp** at 1Y,
- **-13 bp** at 5Y,
- **-19 bp** at 10Y.

That may still be acceptable for stylized scenario generation, but it is too loose for a desk that marks linear books off a market bootstrap and expects the dynamic model to sit consistently on top of it.

### Example for Section 12 — swaption calibration flexibility

Suppose the desk wants to represent the following ATM normal swaption vol term structure:

- 1Yx5Y = **86 bp**,
- 5Yx5Y = **73 bp**,
- 10Yx10Y = **57 bp**.

And on top of that the desk also observes smile effects such as:

- payer wing = ATM + **6 bp**,
- receiver wing = ATM + **5 bp**.

A one-factor CIR or Courtadon model may reproduce the broad level of the term structure, but it generally cannot fit all three ATM points plus both wings with one coherent low-parameter specification.

A piecewise-volatility one-factor Hull-White model can usually fit the **ATM term structure** much better, but still not the full smile.

An LMM or a market-volatility framework such as **Black/Bachelier + SABR** is much better suited when the desk wants the calibration object to be the traded swaption surface itself.

---

## 13. Modern practice on rates desks

A useful modern-practice summary is to separate the **calibration object** from the **dynamic engine**.

### 13.1 For today's curve state

Desks typically use:

- multi-curve bootstrap,
- OIS discounting,
- tenor-specific forward curves,
- direct sensitivity and bump infrastructure on those bootstrapped curves.

Short-rate models are usually **not** the primary representation of today's mark-to-market curve.

### 13.2 For liquid vanilla option quoting

Desks typically rely on:

- normal or shifted-lognormal quoting,
- Black/Bachelier formulas,
- SABR or related volatility-surface parameterizations,
- direct calibration to the swaption or cap/floor volatility surface.

So for many liquid vanillas, the market standard is **surface-first**, not **short-rate-first**.

### 13.3 For callable or Bermudan products

Common practical choices are:

- one-factor or two-factor Hull-White,
- LGM-style Gaussian models,
- Markov-functional models,
- sometimes LMM when forward-rate consistency and richer correlation structure matter enough to justify the extra complexity.

The reason is that these models usually give a better compromise between:

- calibration quality,
- numerical speed,
- hedge explainability,
- implementation robustness.

### 13.4 For XVA, exposure, and scenario engines

Common practical choices are parsimonious models such as:

- one-factor or two-factor Gaussian short-rate models,
- Hull-White variants,
- simplified multi-factor extensions when correlation realism matters.

These are attractive because they are fast, stable, and easy to run repeatedly across large netting sets.

### 13.5 Where CIR, Courtadon, BDT, and Black-Karasinski still matter

They still matter in several roles:

- pedagogy,
- model-risk benchmarking,
- positivity-constrained stylized scenario engines,
- tree-based callable bond or liability applications,
- historical and theoretical foundations,
- related credit-intensity or affine-model applications.

But for a modern liquid trading environment, they are usually **supporting models** rather than the single canonical desk-wide model.

---

## 14. Which model to use when

### Use Hull-White with time-dependent volatility when

- exact fit to today's curve matters,
- normal-rate dynamics are acceptable,
- you want a practical balance of tractability and realism,
- the target use case is callable pricing, lattice methods, or exposure simulation.

### Use CIR when

- positivity matters,
- you want square-root level-dependent volatility,
- affine bond-pricing formulas are valuable,
- the task is stylized modeling rather than full trading-surface calibration.

### Use Courtadon-style square-root models when

- you want the same practical intuition as CIR,
- historical bond-option literature or a square-root specification is the natural reference.

### Use a simple lognormal model when

- positivity is important,
- you only need a basic proportional-volatility benchmark,
- mean reversion is not the main priority.

### Use Black-Karasinski when

- positivity and mean reversion are both required,
- time-dependent volatility or tree calibration is central,
- you can afford numerical calibration,
- the market regime is not dominated by persistent near-zero or negative rates.

### Use BDT-style time-dependent-volatility trees when

- the volatility term structure itself is the calibration target,
- the product is naturally handled in a recombining tree,
- the application values transparency more than full market-factor richness.

### Use LMM or forward-rate market models when

- calibration to the liquid caplet or swaption surface is central,
- forward-rate correlation structure matters for hedging,
- the desk is willing to pay the computational and implementation cost.

---

## 15. Practical implementation guidance for QRP

Within this repository, the clean implementation order is:

1. **Bootstrap today's curve** from market quotes.
2. **Store model parameters separately** from the market curve object.
3. **Represent model family explicitly** in configuration, for example:
   - `HO_LEE`,
   - `HULL_WHITE_1F`,
   - `CIR`,
   - `COURTADON`,
   - `BLACK_KARASINSKI`,
   - `BDT`,
   - `LMM`.
4. **Keep volatility-term-structure functions first-class**, for example constant, piecewise, or exponentially decaying.
5. **Separate vanilla-surface calibration from exotic-model calibration** so the same market snapshot can support Black/Bachelier/SABR and short-rate models side by side.
6. **Separate pricing calibration from scenario simulation** so the same model family can serve both.

A useful parameter payload for a short-rate model object is:

- family name,
- mean-reversion speed,
- long-run level or curve-fit drift function,
- volatility function type and parameters,
- positivity flag or state transformation,
- factor dimension,
- calibration timestamp and market source.

---

## 16. Bottom line

The important conceptual upgrade beyond Ho-Lee is not just “a more complicated equation.”
It is the introduction of four realistic design choices:

- **mean reversion**,
- **volatility that depends on time or rate level**,
- **positivity or exact curve-fit constraints**,
- **the calibration-versus-tractability trade-off**.

That is why the practical progression is usually:

- Ho-Lee for baseline intuition,
- Hull-White for mean reversion plus curve fit,
- CIR or Courtadon for square-root positive-rate behavior,
- Black-Karasinski or BDT when positive lognormal dynamics with time-dependent volatility are preferred,
- LMM or surface-based market models when the trading problem is really about fitting and hedging the liquid swaption market.
