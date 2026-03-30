# Modeling of Rates, Factors, and Curves in Practice

## 0. Why this chapter exists

There are two different tasks that people often mix together:

1. **Static market-curve construction today**
2. **Dynamic modeling of how rates, spreads, or prices evolve through time**

For today's pricing and risk, front-office platforms usually start from **bootstrapped market curves**.
For scenario generation, Monte Carlo, option pricing, and risk-factor evolution, they then use **stochastic models**.

A useful summary is:

> It helps to separate today's market-state construction from dynamic factor modeling. For current PV and sensitivities,
> the right object is a market-consistent bootstrapped curve. For scenario evolution, Monte Carlo, and derivatives
> beyond simple bump-and-revalue, a dynamic model such as GBM, Ho-Lee, Hull-White, LMM, or a parametric curve model is
> needed depending on the use case.

---

## 1. Brownian motion and Brownian motion with drift

### 1.1 Standard Brownian motion

A standard Brownian motion $(W_t)_{t\ge 0}$ satisfies:

$$
W_0 = 0
$$

$$
W_t - W_s \sim \mathcal{N}(0, t-s), \qquad t > s
$$

Where, in this subsection:

- $W_t$ is Brownian motion at time $t$.
- $W_s$ is Brownian motion at an earlier time $s$.
- $\mathcal{N}(0,t-s)$ is a normal distribution with mean $0$ and variance $t-s$.
- $t-s$ is the elapsed time between the two dates.

Interpretation:

- the increment has mean zero,
- its variance grows linearly with time,
- shocks accumulate through time.

#### Example for 1.1 — one Brownian increment over 3 months

Take $s=0$ and $t=0.25$ years.
Then

$$
W_{0.25} - W_0 \sim \mathcal{N}(0,0.25)
$$

so the standard deviation is

$$
\sqrt{0.25} = 0.5
$$

If one simulation draw gives $Z=0.8$ from a standard normal, then the Brownian increment is

$$
\Delta W = \sqrt{0.25}\,Z = 0.5 \times 0.8 = 0.4
$$

This means the Brownian shock over the quarter is $0.4$ in that simulated path.

### 1.2 Arithmetic Brownian motion with drift

A simple process with drift is

$$
X_t = X_0 + \mu t + \sigma W_t
$$

or, in differential form,

$$
dX_t = \mu \, dt + \sigma \, dW_t
$$

Where, in this subsection:

- $X_t$ is the modeled quantity at time $t$.
- $X_0$ is its value today.
- $\mu$ is the drift per unit time.
- $\sigma$ is the volatility per square-root of time.
- $dt$ is an infinitesimal time step.
- $dW_t$ is the Brownian increment over that step.

This model is often a good first approximation for:

- short-rate models with normal shocks,
- spread changes,
- P&L factors expressed in basis points or absolute units.

As a standalone factor process, it is useful for intuition and simulation, but it is not by itself a full
market-consistent term-structure model.

#### Example for 1.2 — modeling a spread in absolute units

Suppose:

- today's spread level is $X_0 = 120$ bp,
- drift is $\mu = -5$ bp per year,
- volatility is $\sigma = 20$ bp per square-root-year,
- horizon is $t=1$ year,
- simulated Brownian draw is $W_1 = 0.3$.

Then

$$
X_1 = 120 + (-5)\cdot 1 + 20 \cdot 0.3 = 121 \text{ bp}
$$

So in this path the spread widens slightly from 120 bp to 121 bp.

### 1.3 Geometric Brownian motion

For equity or FX under the simplest lognormal model,

$$
dS_t = \mu S_t \, dt + \sigma S_t \, dW_t
$$

The exact solution is

$$
S_T = S_0 \exp\left(\left(\mu - \frac{1}{2}\sigma^2\right)T + \sigma \sqrt{T}\, Z\right)
$$

Where, in this subsection:

- $S_t$ is the asset price at time $t$.
- $S_0$ is the current price.
- $\mu$ is the drift.
- $\sigma$ is the volatility.
- $T$ is the horizon in years.
- $Z \sim \mathcal{N}(0,1)$ is a standard normal draw.

This is the standard geometric Brownian motion used in Black-Scholes.

#### Example for 1.3 — one-year GBM path for an equity overlay

Suppose:

- $S_0 = 100$,
- $\mu = 3\%$,
- $\sigma = 20\%$,
- $T=1$,
- $Z=0.5$.

Then

$$
S_1 = 100 \exp\left((0.03 - 0.5 \cdot 0.2^2)\cdot 1 + 0.2\cdot 1 \cdot 0.5\right)
$$

$$
S_1 = 100 \exp(0.01 + 0.10) = 100 e^{0.11} \approx 111.63
$$

So the simulated one-year price is about 111.63.

### 1.4 Risk-neutral drift for pricing

Under risk-neutral pricing for a non-dividend-paying asset,

$$
dS_t = r S_t \, dt + \sigma S_t \, dW_t^{\mathbb{Q}}
$$

and therefore

$$
S_T = S_0 \exp\left(\left(r - \frac{1}{2}\sigma^2\right)T + \sigma \sqrt{T}\, Z\right)
$$

Where, in this subsection:

- $r$ is the risk-free rate.
- $W_t^{\mathbb{Q}}$ is Brownian motion under the risk-neutral measure $\mathbb{Q}$.

The key difference is that for pricing, the drift becomes the carry-consistent risk-neutral drift, not the real-world
expected return.

#### Example for 1.4 — pricing drift versus investment drift

If:

- $S_0 = 100$,
- risk-free rate $r=2\%$,
- real-world expected return $\mu = 8\%$,
- $\sigma = 20\%$,
- $T=1$,
- $Z=0$,

then under the pricing measure,

$$
S_1^{\mathbb{Q}} = 100 \exp(0.02 - 0.5\cdot 0.2^2) = 100 e^0 \approx 100
$$

while under the real-world drift,

$$
S_1^{\mathbb{P}} = 100 \exp(0.08 - 0.5\cdot 0.2^2) = 100 e^{0.06} \approx 106.18
$$

So the same volatility can be used with different drifts depending on whether the goal is pricing or forecasting.

### 1.5 Arbitrage-free models versus equilibrium models

In rates modeling, people often separate **arbitrage-free models** from **equilibrium models**.

An **arbitrage-free term-structure model** is built so that today's observed bond prices or zero curve can be matched
consistently, usually under the risk-neutral measure. In practice, this means the model is tied to the market curve we
see today.

An **equilibrium model** starts from a structural or long-run assumption about how the short rate behaves, and the
initial term structure is then implied by the model parameters rather than forced to match every market quote exactly.

- **arbitrage-free / no-arbitrage model**: fit today's curve first, then evolve it consistently;
- **equilibrium model**: specify the dynamics first, and let the curve come out of the model.

Toy processes such as standard Brownian motion or arithmetic Brownian motion with constant drift are useful to explain
shocks and Monte Carlo logic, but by themselves they are not full market-consistent term-structure models. They do not,
on their own, tell you how to price the whole zero-coupon bond curve consistently with today's market quotes.

#### Example for 1.5 — matching today's curve versus implying a curve

Suppose today's market curve says:

- 1Y zero rate = 2.00%,
- 2Y zero rate = 2.40%.

An arbitrage-free short-rate model can be built with a time-dependent drift so that the model reproduces those two
zero-coupon prices exactly today.

An equilibrium model with fixed parameters may instead imply:

- 1Y zero rate = 2.10%,
- 2Y zero rate = 2.25%.

That may still be a perfectly coherent model, but it is not fitted to today's observed market curve point by point.

The practical difference is:

- for **front-office pricing today**, desks usually want exact or near-exact market fit;
- for **economic intuition or long-run dynamics**, an equilibrium model may still be informative.

---

## 2. Short-rate models for interest rates

### 2.1 Why rates are modeled differently from equities

For an equity price, a lognormal model is often a reasonable first approximation because prices stay positive naturally.
For interest rates, the object we often model is not the bond price directly but a **rate process**, for example the
short rate $r_t$.

A short-rate model drives the term structure through time and can be used for:

- scenario generation,
- interest-rate option pricing,
- tree or lattice methods,
- Monte Carlo exposure and risk.

#### Example for 2.1 — one static curve versus one dynamic scenario

A bootstrapped curve today might say:

- 1Y zero rate = 2.0%,
- 2Y zero rate = 2.2%,
- 5Y zero rate = 2.7%.

That is today's **static** market state.

A dynamic model then asks:

- if rates evolve over the next week or month, what new curve states become possible?
- what is the distribution of future 2Y or 5Y rates?
- what is the distribution of future book P&L?

That is the role of rate modeling.

### 2.2 Ho-Lee model

A simple normal short-rate model is

$$
dr_t = \theta(t) \, dt + \sigma \, dW_t
$$

Where, in this subsection:

- $r_t$ is the short rate at time $t$.
- $\theta(t)$ is the deterministic drift function chosen to fit the initial term structure.
- $\sigma$ is the short-rate volatility.
- $W_t$ is Brownian motion.

If $\theta(t)$ is constant and equal to $\theta$, then

$$
r_t = r_0 + \theta t + \sigma W_t
$$

This is an arithmetic Brownian motion applied to the short rate.

Ho-Lee is typically introduced as a **no-arbitrage / arbitrage-free short-rate model** because the drift term
$\theta(t)$ can be chosen so that the model matches today's observed term structure exactly. In other words, the model
is anchored to today's market bond prices rather than leaving them as a by-product of fixed long-run parameters.

Main characteristics:

- normal-rate dynamics,
- very simple,
- no mean reversion,
- rates can become negative,
- can be calibrated to today's initial curve through $\theta(t)$,
- useful pedagogically and sometimes for simple normal-rate scenario generation.

#### Example for 2.2 — one-year Ho-Lee short-rate scenario

Suppose:

- current short rate $r_0 = 2.00\%$,
- constant drift $\theta = 0.20\%$ per year,
- volatility $\sigma = 1.00\%$ per square-root-year,
- horizon $t=1$ year,
- one simulated draw gives $W_1 = 0.5$.

Then

$$
r_1 = 0.0200 + 0.0020 \cdot 1 + 0.0100 \cdot 0.5 = 0.0270
$$

So the one-year short rate in that scenario is 2.70%.

#### Example for 2.2 — why Ho-Lee is called arbitrage-free in practice

Suppose today's market gives the discount factors

$$
P(0,1)=0.9802, \qquad P(0,2)=0.9550
$$

equivalently about:

- 1Y zero rate $\approx 2.00\%$,
- 2Y zero rate $\approx 2.30\%$.

In a no-arbitrage setup such as Ho-Lee, the function $\theta(t)$ is chosen so that the model prices of the 1Y and 2Y
zero-coupon bonds match those market prices today.

So the logic is:

1. observe today's bond prices or zero rates,
2. solve for the drift adjustment $\theta(t)$,
3. use the calibrated model for scenario generation or option pricing.

The important practical point is not the exact algebra of $\theta(t)$, but the fact that the model is
**forced to match today's market curve**.

### 2.3 Vasicek model

A classical mean-reverting short-rate model is

$$
dr_t = a(b-r_t)dt + \sigma dW_t
$$

Where, in this subsection:

- $a$ is the mean-reversion speed.
- $b$ is the long-run mean level.
- $\sigma$ is the short-rate volatility.

Vasicek is usually presented as an **equilibrium model**. The idea is that the short rate fluctuates around a long-run
equilibrium level $b$, and the term structure is implied by the model parameters rather than forced to match every
market quote exactly.

That makes it very useful conceptually:

- the model is simple,
- the mean-reversion story is intuitive,
- bond prices are available in closed form,

but a plain Vasicek model with fixed parameters is usually not used as the primary daily market curve representation on
a trading desk because it does not guarantee an exact fit to today's observed term structure.

The conditional expectation is

$$
\mathbb{E}[r_t] = b + (r_0-b)e^{-at}
$$

#### Example for 2.3 — one-year Vasicek expected short rate

Suppose:

- current short rate $r_0 = 3.00\%$,
- long-run level $b = 2.00\%$,
- mean-reversion speed $a = 0.8$,
- horizon $t=1$ year.

Then

$$
\mathbb{E}[r_1] = 0.02 + (0.03-0.02)e^{-0.8}
$$

$$
\mathbb{E}[r_1] = 0.02 + 0.01 \times 0.44933 = 0.02449
$$

So the expected one-year short rate is about 2.449%.

#### Example for 2.3 — why Vasicek is called an equilibrium model

Suppose today's market curve is:

- 1Y zero rate = 2.00%,
- 2Y zero rate = 2.40%,
- 5Y zero rate = 3.10%.

A plain Vasicek parameter set such as

- $a=0.8$,
- $b=2.0\%$,
- $\sigma=1.0\%$,
- $r_0=2.2\%$

may imply its own smooth curve, for example:

- 1Y model zero rate = 2.08%,
- 2Y model zero rate = 2.28%,
- 5Y model zero rate = 2.62%.

That curve is internally coherent, but it is not fitted exactly to today's market quotes. That is why people distinguish
it from a no-arbitrage curve-fitting model such as Ho-Lee or Hull-White with time-dependent drift.

### 2.4 Hull-White one-factor model

A more practical mean-reverting normal-rate model is

$$
dr_t = \bigl(\theta(t) - a r_t\bigr)dt + \sigma dW_t
$$

A common equivalent intuition is

$$
dr_t = a\bigl(b(t)-r_t\bigr)dt + \sigma dW_t
$$

Where, in this subsection:

- $a$ is the mean-reversion speed.
- $b(t)$ is the time-dependent long-run level implied by the initial curve fit.

Main characteristics:

- fits the initial curve through $\theta(t)$,
- mean reversion makes rates more realistic than Ho-Lee,
- widely used for interest-rate options, trees, Monte Carlo, and exposure analytics,
- still allows negative rates in its normal form.

If the long-run level is constant $b$, the conditional expectation is

$$
\mathbb{E}[r_t] = b + (r_0-b)e^{-at}
$$

#### Example for 2.4 — expected rate under mean reversion

Suppose:

- current short rate $r_0 = 3.00\%$,
- long-run level $b = 2.50\%$,
- mean-reversion speed $a = 0.5$,
- horizon $t = 1$ year.

Then

$$
\mathbb{E}[r_1] = 0.025 + (0.03-0.025)e^{-0.5}
$$

$$
\mathbb{E}[r_1] = 0.025 + 0.005 \times 0.60653 = 0.02803
$$

So the expected one-year short rate is about 2.803%.

### 2.5 Ho-Lee versus Vasicek versus Hull-White in practice

The two models answer similar questions but with different realism.

- **Ho-Lee** is the cleanest no-arbitrage normal short-rate model and is excellent for teaching curve-fitting intuition
  and simple scenario demonstrations.
- **Vasicek** is the classical equilibrium mean-reverting model: elegant, intuitive, and analytically convenient, but
  not designed to fit today's curve exactly point by point.
- **Hull-White** keeps the no-arbitrage curve-fitting logic and adds mean reversion, which is why it is much more common
  in practical rates-option and exposure settings.

#### Example for 2.5 — why mean reversion matters

If the current short rate spikes from 2% to 5% because of a temporary shock:

- a Ho-Lee model keeps evolving around the new level plus drift,
- a Vasicek model pulls the rate back toward its equilibrium level,
- a Hull-White model also mean-reverts, but can still be aligned to today's market curve through a time-dependent drift.

That makes Vasicek useful for equilibrium intuition and Hull-White more plausible for market-consistent medium-horizon
rate scenarios.

### 2.6 When short-rate models are used in practice

Short-rate models are most useful for:

- callable bond valuation,
- Bermudan swaptions,
- XVA / exposure simulation,
- scenario generation with full-curve dynamics,
- tree-based pricing.

For today's plain-vanilla discount curve itself, desks usually still start from **market bootstrap**, not from a
short-rate model alone.

#### Example for 2.6 — what the desk actually does today

For a USD collateralized rates desk:

- today's OIS discount curve is usually bootstrapped from SOFR OIS quotes,
- today's forward curves are bootstrapped from liquid tenor instruments,
- a short-rate model may then be calibrated on top for option pricing or future exposure.

So the static curve and the dynamic model play different roles.

### 2.7 Forward-rate models and normal/lognormal option models

For vanilla rates options, desks often model a forward rate or swap rate directly rather than only the short rate.
Two very common approximations are:

**Normal forward model (Bachelier-style):**

$$
dF_t = \nu \, dW_t
$$

**Lognormal forward model (Black-style):**

$$
dF_t = \nu F_t \, dW_t
$$

Where, in this subsection:

- $F_t$ is the modeled forward or swap rate at time $t$.
- $\nu$ is the volatility parameter of that forward rate.

Main interpretation:

- the **normal** model allows negative rates naturally and is widely used when desks quote normal vols,
- the **lognormal** model keeps the rate positive and is closer to Black-style market conventions,
- in practice, smile fitting often sits on top through normal-vol or shifted-lognormal SABR.

#### Example for 2.7 — one quarterly move of a forward swap rate

Suppose the current forward swap rate is

$$
F_0 = 2.50\%
$$

Take a quarterly step of

$$
\Delta t = 0.25
$$

and a normal volatility of

$$
\nu = 1.20\% \text{ per square-root-year}
$$

If one standard-normal draw is $Z=0.5$, then under the normal model

$$
\Delta F = \nu \sqrt{\Delta t} Z = 0.012 \times 0.5 \times 0.5 = 0.003
$$

so

$$
F_{0.25} = 2.50\% + 0.30\% = 2.80\%
$$

Under a lognormal model with the same volatility parameter,

$$
F_{0.25} \approx F_0 \exp\left(-\frac{1}{2}\nu^2 \Delta t + \nu \sqrt{\Delta t} Z\right)
$$

$$
F_{0.25} \approx 2.50\% \times e^{-0.5\cdot 0.012^2\cdot 0.25 + 0.012\cdot 0.5\cdot 0.5}
\approx 2.5075\%
$$

So the same volatility concept can produce very different path behavior depending on whether the model is normal or
lognormal.

---

## 3. Credit and spread modeling

### 3.1 Piecewise hazard-rate model

For reduced-form credit modeling, a common survival representation is

$$
Q(0,T) = \exp\left(-\int_0^T \lambda(u)\,du\right)
$$

Where, in this subsection:

- $Q(0,T)$ is survival probability to time $T$.
- $\lambda(u)$ is the hazard rate at time $u$.

If the hazard rate is piecewise constant, then on each interval the integral becomes a simple product.

#### Example for 3.1 — piecewise survival probabilities

Suppose hazard rates are:

- 0 to 1Y: $\lambda_1 = 2\%$,
- 1Y to 3Y: $\lambda_2 = 3\%$.

Then

$$
Q(0,1)=e^{-0.02\cdot 1}=0.98020
$$

and

$$
Q(0,3)=e^{-0.02\cdot 1 - 0.03\cdot 2}=e^{-0.08}=0.92312
$$

So survival to 3Y is about 92.31%.

### 3.2 What is modeled in practice for CDS curves

For today's CDS pricing, desks usually use:

- bootstrapped discount curves,
- bootstrapped piecewise survival or hazard curves,
- recovery assumption,
- spread-risk and hazard-risk sensitivities.

For richer dynamic credit modeling, firms may add spread processes, intensity dynamics, or copula / structural
dependence models, but the day-to-day production base case is often still the calibrated piecewise hazard curve.

#### Example for 3.2 — practical CDS curve construction

Suppose a name has market CDS spreads:

- 1Y: 80 bp,
- 3Y: 110 bp,
- 5Y: 140 bp.

A production curve builder typically solves piecewise hazard nodes so those maturities reprice, rather than imposing one
single constant hazard rate across all maturities.

---

## 4. Parametric curve models

### 4.1 Nelson-Siegel model

A standard Nelson-Siegel zero-yield curve is

$$
y(t) = \beta_0 + \beta_1 \frac{1-e^{-t/\tau}}{t/\tau} + \beta_2 \left(\frac{1-e^{-t/\tau}}{t/\tau} - e^{-t/\tau}\right)
$$

Where, in this subsection:

- $y(t)$ is the fitted zero yield at maturity $t$.
- $\beta_0$ is the long-term level.
- $\beta_1$ controls the slope, especially at the short end.
- $\beta_2$ controls medium-term curvature.
- $\tau$ is the decay parameter that determines where the hump sits.

Interpretation:

- $\beta_0$ sets the asymptotic long-end level,
- $\beta_1$ tilts the curve up or down,
- $\beta_2$ creates a hump or bowl in the middle,
- $\tau$ says how quickly short-end effects decay.

#### Example for 4.1 — Nelson-Siegel yields at 1Y, 5Y, and 10Y

Take parameters:

- $\beta_0 = 3.0\%$,
- $\beta_1 = -2.0\%$,
- $\beta_2 = 1.0\%$,
- $\tau = 1.5$ years.

At $t=1$ year,

$$
y(1) \approx 1.7567\%
$$

At $t=5$ years,

$$
y(5) \approx 2.6750\%
$$

At $t=10$ years,

$$
y(10) \approx 2.8489\%
$$

So this parameter set gives an upward-sloping curve that starts below the long-run level and gradually approaches 3%.

### 4.2 Nelson-Siegel-Svensson extension

A common extension adds a second curvature term:

$$
y(t) = \beta_0 + \beta_1 \frac{1-e^{-t/\tau_1}}{t/\tau_1} + \beta_2 \left(\frac{1-e^{-t/\tau_1}}{t/\tau_1} - e^{-t/\tau_1}\right) + \beta_3 \left(\frac{1-e^{-t/\tau_2}}{t/\tau_2} - e^{-t/\tau_2}\right)
$$

Where, in this subsection:

- $\beta_3$ adds a second curvature component.
- $\tau_1$ and $\tau_2$ control the locations of the two hump-like components.

This allows more flexibility at intermediate and long maturities.

#### Example for 4.2 — adding a second hump component

Take:

- $\beta_0 = 3.0\%$,
- $\beta_1 = -2.0\%$,
- $\beta_2 = 1.2\%$,
- $\beta_3 = -0.4\%$,
- $\tau_1 = 1.5$,
- $\tau_2 = 5.0$.

Then the fitted yields are approximately:

- 1Y: 1.7650%,
- 5Y: 2.6201%,
- 10Y: 2.7598%,
- 20Y: 2.8492%.

The extra term gives more control over the longer-dated hump shape.

### 4.3 When parametric curve models are used in practice

Nelson-Siegel and Nelson-Siegel-Svensson are widely used for:

- central-bank published sovereign yield curves,
- macro scenario generation,
- regulatory or reporting curves,
- smooth curve summarization,
- low-dimensional factor modeling.

They are less often the primary production representation for high-precision OIS discounting on a trading desk, where
exact repricing of liquid instruments is usually more important than a very low-dimensional parametric fit.

#### Example for 4.3 — static trading curve versus macro scenario curve

A front-office USD OIS discount curve may need to reprice many liquid SOFR OIS quotes almost exactly.
A macro stress platform, by contrast, may prefer to move just a few parameters:

- level,
- slope,
- curvature.

Nelson-Siegel is attractive in the second case because it turns a full curve move into a small number of intuitive
parameters.

---

## 5. Which model is used in practice for which object?

### 5.1 OIS discount curves

For today's OIS discount curve in USD, EUR, or CHF, the standard production choice is usually:

- normalize liquid market quotes,
- use convention-aware helpers,
- bootstrap a piecewise curve,
- interpolate discount factors or zero rates with controlled extrapolation.

Typical references are:

- USD: SOFR OIS,
- EUR: €STR OIS,
- CHF: SARON OIS.

#### Example for 5.1 — today’s USD discounting stack

A desk may build:

- USD OIS discount curve from SOFR OIS quotes,
- projection curves for term rates or basis where relevant,
- then compute PV, DV01, carry, and scenario P&L from that curve set.

This is usually more important for day-to-day pricing than fitting a Ho-Lee model directly to the curve.

### 5.2 Sovereign yield curves

For government-bond reporting or published zero curves, common approaches are:

- bootstrap from bond prices plus interpolation,
- spline smoothing,
- Nelson-Siegel or Nelson-Siegel-Svensson fits.

#### Example for 5.2 — central-bank publication versus trading desk curve

A central bank may publish a smooth Nelson-Siegel-Svensson sovereign zero curve.
A trading desk may still use a more instrument-exact bootstrapped curve for P&L and hedging.

### 5.3 Swaptions and rates volatility smiles

For vanilla rates-option smiles, common practical choices include:

- normal or shifted-lognormal quoting,
- SABR-style smile fitting,
- direct calibration to caplet or swaption surfaces,
- Hull-White or LMM-type dynamic models for richer pricing and exposure uses.

#### Example for 5.3 — smile versus dynamic model

A desk may quote a 5Yx10Y swaption in normal vol and fit the smile with SABR.
For Bermudan or callable structures, it may use Hull-White, LGM, or a lattice model because smile fitting alone is not
enough.

### 5.4 Trade-offs across the main rates model families

A useful practical hierarchy is:

- **Ho-Lee**: exact initial-curve fit and simple intuition, but no mean reversion.
- **Hull-White**: exact initial-curve fit plus mean reversion, fast trees and Monte Carlo, but limited smile and
  correlation richness in one factor.
- **CIR / Courtadon**: positivity and level-dependent volatility, but often too rigid for full trading-surface
  calibration.
- **Black-Karasinski / BDT**: positive-rate tree models, but more numerically involved and less comfortable in prolonged
  near-zero or negative-rate regimes.
- **LMM**: richer forward-rate dynamics and better alignment with liquid caplet or swaption calibration, but higher
  dimensionality and computational cost.

This is why a desk often says that **CIR is elegant**, **Hull-White is practical**, and **LMM is richer but heavier**.

#### Example for 5.4 — calibration target drives model choice

If the desk only needs a callable swap benchmark and fast Greeks, one-factor Hull-White may be enough.
If the desk wants to fit a large caplet or swaption surface and preserve forward-rate structure, LMM is more natural.
If the goal is a positivity-constrained benchmark process rather than a full quoting engine, CIR may still be a sensible
reference.

### 5.5 Credit curves and credit options

For credit curves:

- today's survival curve is usually a bootstrapped piecewise hazard structure.

For richer spread dynamics:

- firms may use spread-factor or intensity dynamics for stress testing or options, but the base daily curve is still
  usually the calibrated hazard structure.

#### Example for 5.5 — CDS curve versus spread scenario engine

A CDS pricer for today uses the bootstrapped survival curve.
A stress engine for next month may then shock hazard rates or credit spreads by sector and rating bucket.

### 5.6 Equities and FX

For equity and FX underlyings:

- GBM is still the simplest reference model for intuition and Monte Carlo,
- local volatility, stochastic volatility, or local-stochastic volatility are common for option books,
- Heston-type models are common when smile dynamics matter.

#### Example for 5.6 — simple versus richer equity modeling

For a basic one-year equity-linked note, GBM may be enough for a teaching example.
For a real option book with smile and skew risk, the desk will usually need local vol, stochastic vol, or a calibrated
implied-vol surface.

---

### 6.1 The hierarchy

A strong way to summarize the modeling hierarchy is:

1. **Current market state**: build today's curves and surfaces from liquid instruments.
2. **Risk representation**: define the factors to bump or shock.
3. **Dynamic modeling**: choose stochastic evolution only when needed for scenario generation, options, or exposure.
4. **Controls**: validate calibration quality, stability, and scenario realism.

#### Example for 6.1 — the same USD rates book under different layers

For the same USD rates book:

- today's PV comes from the bootstrapped SOFR curve,
- DV01 comes from curve bumps,
- stress comes from shocked curve scenarios,
- future exposure or callable valuation may then require Hull-White Monte Carlo or a lattice.

That is the clean separation between static valuation and dynamic modeling.

### 6.2 A good summary on model choice

A realistic answer is:

> For today's discount and projection curves, the right starting point is market-consistent bootstrap rather than a
> stochastic short-rate model. For dynamic rate evolution, model choice should follow the use case: Ho-Lee for a simple
> no-arbitrage normal short-rate model tied to today's curve, Vasicek for an equilibrium mean-reverting reference model,
> Hull-White for practical mean-reverting short-rate analytics that also fit the initial curve, CIR or Courtadon for
> stylized positive-rate affine dynamics, Black-Karasinski or BDT for positive-rate tree applications, LMM or
> surface-based market models when calibration to the liquid rates-option market matters most, Nelson-Siegel or Svensson
> for smooth parametric curve representation and macro scenarios, piecewise hazard curves for CDS calibration, and GBM
> or richer smile models for equities and FX depending on the product.

#### Example for 6.2 — matching the model to the use case

If the task is:

- **today's OIS discounting**: bootstrap.
- **callable swap valuation**: short-rate model such as Hull-White.
- **macro stress on a sovereign curve**: Nelson-Siegel factor shocks.
- **today's CDS pricing**: piecewise hazard bootstrap.
- **simple equity Monte Carlo**: GBM.

That is usually how a strong front-office quant would frame the answer.
