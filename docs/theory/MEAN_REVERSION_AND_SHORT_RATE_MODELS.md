# Mean Reversion and Short-Rate Models

This chapter explains the mathematical idea of **mean reversion** and then shows how it appears in
interest-rate modeling.

It is the natural bridge between the general stochastic-process material in
`docs/theory/BROWNIAN_MOTION_AND_ITO.md` and the implementation-facing rates notes under `docs/pricing/rates/`.

---

## 1. Why mean reversion matters

Many financial quantities do not behave like a pure random walk forever.

For short interest rates in particular, practitioners usually expect three qualitative properties:

- rates may be shocked away from their recent level,
- very high or very low levels tend not to persist indefinitely,
- the uncertainty of future rates often depends on the current level of rates.

A model with **mean reversion** encodes the idea that the process is pulled back toward a target level.

---

## 2. The basic mean-reverting diffusion

A standard continuous-time mean-reverting process is

$$
dX_t = a(b - X_t)dt + \sigma dW_t
$$

Where, in this chapter:

- $X_t$ is the modeled state variable.
- $a > 0$ is the mean-reversion speed.
- $b$ is the long-run level.
- $\sigma$ is the instantaneous volatility.
- $W_t$ is Brownian motion.

This is the Ornstein-Uhlenbeck idea that later reappears in Vasicek, Hull-White, and log-rate models such as
Black-Karasinski.

### Interpretation

- If $X_t > b$, then the drift term $a(b-X_t)$ is negative, so the process is pulled downward.
- If $X_t < b$, then the drift term is positive, so the process is pulled upward.
- Larger $a$ means faster reversion.
- Larger $\sigma$ means more noise around the mean-reverting pull.

### Conditional expectation

For constant parameters,

$$
\mathbb{E}[X_t] = b + (X_0 - b)e^{-at}
$$

This formula is extremely important in practice because it makes the pull toward the long-run level explicit.

### Half-life of a shock

The half-life of a deviation is the time needed for the expected gap from the long-run level to be divided by two.
It is

$$
t_{1/2} = \frac{\ln 2}{a}
$$

So a larger mean-reversion speed implies a shorter half-life.

#### Example for Section 2 — expected value and half-life

Suppose:

- $X_0 = 5.0\%$,
- $b = 3.0\%$,
- $a = 0.7$.

Then after one year,

$$
\mathbb{E}[X_1] = 0.03 + (0.05 - 0.03)e^{-0.7}
$$

$$
\mathbb{E}[X_1] \approx 0.03993
$$

So the expected value after one year is about **3.993%**.

The half-life is

$$
t_{1/2} = \frac{\ln 2}{0.7} \approx 0.99 \text{ years}
$$

So a shock is expected to decay by half in roughly **one year**.

---

## 3. Why mean reversion is natural for interest rates

A short-rate model tries to describe the evolution of the instantaneous funding rate $r_t$.

Compared with equities, short rates have a stronger economic case for mean reversion:

- monetary-policy regimes tend to anchor short-term rates,
- very high short rates often trigger economic slowdown and later easing,
- very low short rates often create pressure for later normalization,
- yield curves and rate options are sensitive not only to the level of rates but also to how quickly shocks fade.

This is why rates practitioners often move quickly from Ho-Lee-style no-mean-reversion intuition to Vasicek,
Hull-White, CIR, and Black-Karasinski style models.

---

## 4. A unifying short-rate family

A useful mental template for many one-factor short-rate models is

$$
dr_t = \mu(t, r_t)dt + \sigma(t) r_t^{\gamma} dW_t
$$

The parameters control two separate ideas:

1. **Drift / pull** through $\mu(t,r_t)$.
2. **How volatility depends on the rate level** through the exponent $\gamma$.

Important special cases are:

- $\gamma = 0$: **normal-rate models** such as Ho-Lee and Hull-White.
- $\gamma = \tfrac{1}{2}$: **square-root volatility** models such as CIR and Courtadon-style square-root models.
- $\gamma = 1$: **lognormal / proportional volatility** models such as Dothan, Rendleman-Bartter, and the log-rate formulation behind Black-Karasinski.

This classification is practically useful because it explains how the volatility behaves when the short rate is low or high.

---

## 5. Normal mean-reverting short rates

### 5.1 Vasicek

The classical equilibrium model is

$$
dr_t = a(b-r_t)dt + \sigma dW_t
$$

Main properties:

- mean reversion toward $b$,
- constant basis-point volatility,
- Gaussian distribution for $r_t$,
- analytically convenient,
- rates can become negative.

#### Example for 5.1 — expected short rate under Vasicek

Suppose:

- $r_0 = 3.0\%$,
- $b = 2.0\%$,
- $a = 0.8$,
- horizon $t = 1$ year.

Then

$$
\mathbb{E}[r_1] = 0.02 + (0.03 - 0.02)e^{-0.8}
$$

$$
\mathbb{E}[r_1] \approx 0.02449
$$

So the expected one-year short rate is about **2.449%**.

### 5.2 Hull-White as a curve-fitting extension

A no-arbitrage extension of the normal mean-reverting idea is

$$
dr_t = (\theta(t) - a r_t)dt + \sigma(t) dW_t
$$

This keeps mean reversion but allows the model to fit the initial term structure through the deterministic function
$\theta(t)$.

If $\sigma(t)$ is time-dependent, the distribution is still Gaussian, but the variance now depends on the whole path of
$\sigma(u)$ from $0$ to $t$.

This is one of the most practical ways to combine:

- exact fit to the initial curve,
- mean reversion,
- time-dependent volatility.

---

## 6. Square-root mean-reverting short rates

### 6.1 CIR

The Cox-Ingersoll-Ross model is

$$
dr_t = a(b-r_t)dt + \sigma \sqrt{r_t} dW_t
$$

Main properties:

- mean reversion toward $b$,
- volatility increases with the square root of the rate level,
- non-negative rates in the core model,
- affine bond-pricing formulas,
- more realistic low-rate behavior than a Gaussian model when strict positivity matters.

A well-known positivity condition is the Feller condition:

$$
2ab \ge \sigma^2
$$

When it holds, the process is strongly kept away from zero.

The conditional mean is

$$
\mathbb{E}[r_t] = r_0 e^{-at} + b(1-e^{-at})
$$

#### Example for 6.1 — expected short rate under CIR

Suppose:

- $r_0 = 5.0\%$,
- $b = 3.0\%$,
- $a = 1.0$,
- $\sigma = 15\%$,
- horizon $t = 1$ year.

Then

$$
\mathbb{E}[r_1] = 0.05e^{-1} + 0.03(1-e^{-1})
$$

$$
\mathbb{E}[r_1] \approx 0.03736
$$

So the expected one-year short rate is about **3.736%**.

#### Example for 6.1 — one quarterly CIR scenario step

Take:

- current rate $r_t = 4.0\%$,
- $a = 0.8$,
- $b = 3.0\%$,
- $\sigma = 15\%$,
- $\Delta t = 0.25$,
- one standard-normal draw $Z = -0.5$.

Using a simple Euler-style illustration,

$$
\Delta r \approx a(b-r_t)\Delta t + \sigma\sqrt{r_t}\sqrt{\Delta t} Z
$$

$$
\Delta r \approx 0.8(0.03-0.04)(0.25) + 0.15\sqrt{0.04}\sqrt{0.25}(-0.5)
$$

$$
\Delta r \approx -0.0095
$$

So the new rate is approximately

$$
r_{t+\Delta t} \approx 4.0\% - 0.95\% = 3.05\%
$$

The diffusion shock shrinks automatically as the level of rates falls because of the $\sqrt{r_t}$ term.

### 6.2 Courtadon-style square-root models

In many term-structure summaries, the **Courtadon model** is presented as an early square-root mean-reverting short-rate model of the form

$$
dr_t = k(\theta - r_t)dt + \sigma \sqrt{r_t} dW_t
$$

In practical interpretation, this is extremely close to the CIR family:

- same mean-reversion idea,
- same square-root state-dependent volatility,
- same intuition that volatility is damped near zero and rises with the rate level.

In repository documentation it is therefore best to think of Courtadon as a historically important square-root member of
this broader family rather than as a radically different modeling philosophy.

#### Example for 6.2 — Courtadon-style one-step move

Suppose:

- $r_t = 4.0\%$,
- $k = 0.9$,
- $\theta = 3.0\%$,
- $\sigma = 12\%$,
- $\Delta t = 0.25$,
- $Z = 0.7$.

Then

$$
\Delta r \approx k(\theta-r_t)\Delta t + \sigma\sqrt{r_t}\sqrt{\Delta t} Z
$$

$$
\Delta r \approx 0.9(0.03-0.04)(0.25) + 0.12\sqrt{0.04}\sqrt{0.25}(0.7)
$$

$$
\Delta r \approx 0.00615
$$

So the new rate is about **4.615%**.

---

## 7. Lognormal short-rate models

### 7.1 Proportional-volatility short rates

A simple lognormal short-rate model is

$$
dr_t = \alpha r_t dt + \sigma r_t dW_t
$$

The key idea is that volatility is proportional to the current level of the short rate.

Main properties:

- rates stay positive,
- volatility scales with the rate level,
- there is no built-in mean reversion in the simplest version,
- large rates can lead to unrealistically explosive behavior if the model is used too literally.

#### Example for 7.1 — one quarterly lognormal short-rate move

Suppose:

- $r_0 = 3.0\%$,
- $\alpha = 1.0\%$,
- $\sigma = 20\%$,
- $\Delta t = 0.25$,
- $Z = 0.5$.

Using the exact GBM-style step,

$$
r_{t+\Delta t} = r_t \exp\left((\alpha - \tfrac{1}{2}\sigma^2)\Delta t + \sigma\sqrt{\Delta t}Z\right)
$$

we obtain approximately

$$
r_{t+\Delta t} \approx 3.146\%
$$

So the rate remains positive and moves in relative rather than absolute terms.

### 7.2 Black-Karasinski

Black-Karasinski applies mean reversion to the **log short rate** rather than to the rate itself.

A standard representation is

$$
d\ln r_t = (\theta(t) - \phi \ln r_t)dt + \sigma(t)dW_t
$$

Equivalently, if $x_t = \ln r_t$, then $x_t$ is an Ornstein-Uhlenbeck process and $r_t = e^{x_t}$.

Main properties:

- rates stay positive,
- mean reversion acts in log-rate space,
- time-dependent drift and volatility can be used,
- widely used in tree/lattice implementations for rates options,
- less analytically convenient than affine Gaussian or CIR models.

#### Example for 7.2 — expected log-rate and median short rate

Suppose:

- current rate $r_0 = 4.0\%$,
- long-run level $\bar r = 2.5\%$,
- mean-reversion speed $\phi = 0.7$,
- log-rate volatility $\sigma = 20\%$,
- horizon $t = 1$ year.

Let $x_t = \ln r_t$ and $\bar x = \ln \bar r$.
Then

$$
\mathbb{E}[x_1] = \bar x + (x_0 - \bar x)e^{-0.7}
$$

which gives a median future short rate of

$$
\exp(\mathbb{E}[x_1]) \approx 3.157\%
$$

So the model pulls the short rate down toward the long-run level while preserving positivity.

---

## 8. Time-dependent volatility

Time-dependent volatility is often introduced as

$$
\sigma(t) = \sigma_0 e^{-\lambda t}
$$

with $\lambda > 0$.

This means:

- short-horizon uncertainty is large,
- long-horizon incremental shocks are damped,
- the volatility term structure decays through time.

This is useful when the desk wants the short end of the curve to be more volatile than the long end, or when option-implied
volatility suggests a front-loaded uncertainty profile.

### Example for Section 8 — exponentially decaying volatility

Suppose

$$
\sigma(t) = 1.5\% \times e^{-0.7t}
$$

Then:

- at $t=0$, volatility is **1.50%**,
- at $t=1$, volatility is about **0.745%**,
- at $t=2$, volatility is about **0.370%**,
- at $t=5$, volatility is about **0.045%**.

So the random shocks are heavily concentrated in the earlier part of the horizon.

---

## 9. Mean reversion plus time-dependent volatility

These two ideas are often combined.

### 9.1 Normal form

A common practical model is

$$
dr_t = (\theta(t) - a r_t)dt + \sigma(t)dW_t
$$

This is the time-dependent-volatility Hull-White form.

- mean reversion comes from $a$,
- curve fitting comes from $\theta(t)$,
- volatility-term-structure control comes from $\sigma(t)$.

### 9.2 Square-root form

A level-dependent alternative is

$$
dr_t = a(b-r_t)dt + \sigma(t)\sqrt{r_t}dW_t
$$

This keeps the CIR intuition but lets the volatility scale change through calendar time.

### 9.3 Lognormal form

A positive-rate, mean-reverting log model is

$$
d\ln r_t = (\theta(t) - \phi \ln r_t)dt + \sigma(t)dW_t
$$

This is the Black-Karasinski style extension with time-varying volatility.

These three forms differ mainly in:

- whether negative rates are possible,
- whether volatility is absolute, square-root, or proportional in spirit,
- whether analytical tractability or positivity is the main design goal.

---

## 10. Which family is best for which purpose?

### When you want exact fit to today's curve

Prefer a no-arbitrage shifted or extended model such as:

- Hull-White,
- Black-Karasinski with numerical calibration,
- CIR++ or related shifted affine models.

### When positivity matters strongly

Prefer:

- CIR / square-root models,
- Black-Karasinski / log-rate models.

### When analytical convenience matters

Prefer:

- Vasicek,
- Hull-White,
- CIR.

### When you want intuitive macro-scenario storytelling

Prefer models with clear parameters for:

- mean-reversion speed,
- long-run level,
- front-loaded versus decaying volatility.

---

## 11. Practical summary

The main progression is:

1. **Ho-Lee**: simple, normal, no mean reversion.
2. **Vasicek**: adds mean reversion.
3. **Hull-White**: adds curve fitting and optionally time-dependent volatility.
4. **CIR / Courtadon-style square-root models**: add mean reversion with level-dependent volatility and positive-rate behavior.
5. **Lognormal and Black-Karasinski models**: preserve positivity through proportional or log-rate dynamics, with Black-Karasinski also adding mean reversion.

That progression is useful because it mirrors how real rates platforms evolve:

- first build today's market curve,
- then choose a dynamic model,
- then decide whether exact curve fit, positivity, or tractability matters most.
