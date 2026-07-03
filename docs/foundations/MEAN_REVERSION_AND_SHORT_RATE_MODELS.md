# Mean Reversion and Short-Rate Models

This chapter explains mean reversion and shows how it enters short-rate modeling.

## Notation used in this chapter

Unless stated otherwise:

- $X_t$ is a generic state variable at time $t$.
- $r_t$ is the short rate at time $t$.
- $W_t$ is Brownian motion.
- $a$ is mean-reversion speed.
- $b$ is the long-run mean level.
- $\sigma$ is volatility.
- $\theta(t)$ is a time-dependent drift function.

## 1. What mean reversion means

A mean-reverting process is pulled toward a target level instead of wandering like a pure random walk forever.

Let $X_t$ follow the Ornstein-Uhlenbeck form

$$
dX_t=a(b-X_t)dt+\sigma dW_t
$$

Where:

- $a>0$ controls how quickly the process is pulled back.
- $b$ is the long-run level.
- $\sigma$ controls random fluctuations.

Its conditional expectation is

$$
\mathbb{E}[X_t]=b+(X_0-b)e^{-at}
$$

The half-life of a deviation is

$$
t_{1/2}=\frac{\ln 2}{a}
$$

## 2. What the short rate means

The short rate is the instantaneous risk-free rate for an infinitesimal borrowing or lending period. If $P(t,T)$ is the zero-coupon bond price at time $t$ for maturity $T$, then

$$
r_t=-\left.\frac{\partial}{\partial T}\log P(t,T)\right|_{T=t}
$$

Where:

- $P(t,T)$ is the zero-coupon bond price.
- $r_t$ is the short rate implied by the very short end of the term structure.

A short-rate model specifies a stochastic process for $r_t$ and derives bond prices and rate derivatives from that process.

## 3. Main short-rate models

### Vasicek

$$
dr_t=a(b-r_t)dt+\sigma dW_t
$$

Use: tractable Gaussian mean reversion with closed-form bond pricing.

Main caveat: rates can become negative.

### Hull-White one-factor

$$
dr_t=(\theta(t)-ar_t)dt+\sigma dW_t
$$

Use: mean reversion with time-dependent drift chosen to fit today's initial curve exactly.

Main practical role: callable products, Bermudan swaptions, exposure simulation, and scenario engines.

### CIR

$$
dr_t=a(b-r_t)dt+\sigma\sqrt{r_t}\,dW_t
$$

Use: mean reversion with state-dependent volatility and non-negative-rate tendency.

Main caveat: less convenient than Hull-White in many pricing implementations.

### Black-Karasinski

$$
d\ln r_t=a\left(b(t)-\ln r_t\right)dt+\sigma dW_t
$$

Use: lognormal-style mean reversion with positive rates in the original form.

## 4. Comparison table

| Model | SDE | Mean reversion | Fits initial curve exactly | Negative rates possible | Typical use |
|---|---|---:|---:|---:|---|
| Vasicek | $dr_t=a(b-r_t)dt+\sigma dW_t$ | Yes | No | Yes | interpretation and simple analytics |
| Hull-White | $dr_t=(\theta(t)-ar_t)dt+\sigma dW_t$ | Yes | Yes | Yes | rates exotics and simulation |
| CIR | $dr_t=a(b-r_t)dt+\sigma\sqrt{r_t}dW_t$ | Yes | Not in basic form | Usually no | affine term-structure modeling |
| Black-Karasinski | $d\ln r_t=a(b(t)-\ln r_t)dt+\sigma dW_t$ | Yes | After calibration | No in original form | lattice pricing and structured rates |

## 5. How these models are used in practice

Short-rate models are usually used for:

- callable and Bermudan-style interest-rate products,
- exposure simulation and XVA,
- ALM and scenario generation,
- pedagogical and prototyping work when a compact dynamic rates model is needed.

For liquid vanilla option calibration, systems often rely more directly on Black-style models, SABR, or market models
because those align more directly with market quoting conventions.
