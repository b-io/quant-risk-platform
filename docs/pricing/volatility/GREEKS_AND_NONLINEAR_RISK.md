# Greeks, Non-Linear Risk, and Scenario Revaluation

This chapter explains the main Greeks, how to interpret them, and how they feed into hedging, P&L explain, and scenario reporting.

## Notation used in this chapter

Unless stated otherwise:

- $V$ is the model value of the instrument or portfolio.
- $S$ is the underlying level.
- $\sigma$ is implied volatility.
- $r$ is the relevant rate input.
- $t$ is calendar time.
- $K$ is strike.
- $\Delta S$ is a small move in the underlying.
- $\Delta \sigma$ is a small move in implied volatility.
- $\Delta r$ is a small move in the relevant rate.
- $dt$ is a small time increment.
- $h$ is a finite-difference bump size.

## 1. Linear versus non-linear payoffs

Let $\Pi_{forward}(T)$ be the payoff of a forward at maturity $T$. Then

$$
\Pi_{forward}(T)=S_T-K
$$

Where:

- $S_T$ is the underlying level at maturity.
- $K$ is the strike or delivery price.

Let $\Pi_{call}(T)$ and $\Pi_{put}(T)$ be European option payoffs. Then

$$
\Pi_{call}(T)=(S_T-K)^+
$$

$$
\Pi_{put}(T)=(K-S_T)^+
$$

The important practical point is that optional payoffs are curved rather than linear, so local sensitivities beyond first order matter.

## 2. Core Greeks

Let $V$ be the model value as a function of the main risk inputs $S$, $\sigma$, $r$, and $t$.

### 2.1 Delta

Let $\Delta$ be sensitivity of value to the underlying level. Then

$$
\Delta=\frac{\partial V}{\partial S}
$$

Where:

- $\Delta$ is delta.
- $\frac{\partial V}{\partial S}$ is the first derivative of value with respect to the underlying.

For a small underlying move,

$$
\Delta V\approx \Delta\,\Delta S
$$

Where:

- $\Delta V$ is the change in value.
- $\Delta S$ is the move in the underlying.

Example: if $\Delta=0.55$ and $\Delta S=2$, then $\Delta V\approx 1.10$.

### 2.2 Gamma

Let $\Gamma$ be curvature of value with respect to the underlying. Then

$$
\Gamma=\frac{\partial^2 V}{\partial S^2}
$$

Where:

- $\Gamma$ is gamma.

A positive gamma position benefits from larger underlying moves in either direction relative to a purely linear hedge.

### 2.3 Vega

Let $Vega$ be sensitivity of value to implied volatility. Then

$$
Vega=\frac{\partial V}{\partial \sigma}
$$

Where:

- $Vega$ is vega.
- $\sigma$ is the implied-volatility input or surface coordinate being shocked.

Example: if $Vega=0.40$ per volatility point and implied volatility rises by 2 points, then $\Delta V\approx 0.80$.

### 2.4 Theta

Let $\Theta$ be sensitivity of value to the passage of time under fixed market inputs. Then

$$
\Theta=\frac{\partial V}{\partial t}
$$

Where:

- $\Theta$ is theta.
- $t$ is calendar time under the sign convention used by the system.

Theta conventions vary. Some systems report the raw derivative, others report one-day carry such as $\Theta\,dt$.

### 2.5 Rho

Let $\rho$ be sensitivity of value to the relevant rate input. Then

$$
\rho=\frac{\partial V}{\partial r}
$$

Where:

- $\rho$ is rho.
- $r$ is the rate or yield input being shocked.


## 2.6 Closed-form Black-Scholes Greeks for a European call

Let $C$ be the Black-Scholes price of a European call on a non-dividend-paying stock. Then

$$
C = S N(d_1) - K e^{-r \tau} N(d_2)
$$

Where:

- $C$ is the call price.
- $S$ is the current underlying level.
- $K$ is the strike.
- $r$ is the continuously compounded risk-free rate.
- $\tau$ is time to maturity.
- $N(\cdot)$ is the standard normal cumulative distribution function.
- $d_1$ and $d_2$ are the standard Black-Scholes quantities defined below.

Let $d_1$ and $d_2$ be the Black-Scholes standardized variables. Then

$$
d_1 = \frac{\ln(S/K) + (r + \tfrac12 \sigma^2) \tau}{\sigma \sqrt{\tau}}
$$

$$
d_2 = d_1 - \sigma \sqrt{\tau}
$$

Where:

- $\sigma$ is the Black-Scholes volatility.

Let $\phi(x)$ be the standard normal density. Then

$$
\phi(x) = \frac{1}{\sqrt{2\pi}} e^{-x^2/2}
$$

Where:

- $\phi(x)$ is the standard normal probability density function.
- $x$ is a real argument.

The main Greeks are then:

$$
\Delta = N(d_1)
$$

Where:

- $\Delta$ is the call delta.

$$
\Gamma = \frac{\phi(d_1)}{S \sigma \sqrt{\tau}}
$$

Where:

- $\Gamma$ is the call gamma.

$$
Vega = S \phi(d_1) \sqrt{\tau}
$$

Where:

- $Vega$ is the call vega.
- This vega is per unit change in volatility, not per volatility point.

$$
\rho = K \tau e^{-r \tau} N(d_2)
$$

Where:

- $\rho$ is the call rho.

If calendar time is denoted by $t$ and maturity is fixed at $T$, then $\tau = T - t$ and

$$
\Theta = \frac{\partial C}{\partial t} = -\frac{\partial C}{\partial \tau}
$$

Where:

- $\Theta$ is theta under the convention of differentiation with respect to calendar time.

For the non-dividend-paying European call,

$$
\Theta = -\frac{S \phi(d_1) \sigma}{2 \sqrt{\tau}} - r K e^{-r \tau} N(d_2)
$$

Where:

- the first term is time decay from diffusion uncertainty collapsing as expiry approaches.
- the second term comes from discounting the strike.

### Numerical example: European call Greeks

Let a European call have:

- $S = 100$
- $K = 100$
- $r = 5\%$
- $\sigma = 20\%$
- $\tau = 1$

Then

$$
d_1 = 0.35
$$

$$
d_2 = 0.15
$$

Using $N(0.35) \approx 0.6368$, $N(0.15) \approx 0.5596$, and $\phi(0.35) \approx 0.3752$:

$$
\Delta \approx 0.6368
$$

$$
\Gamma \approx \frac{0.3752}{100 \cdot 0.2 \cdot 1} \approx 0.0188
$$

$$
Vega \approx 100 \cdot 0.3752 \cdot 1 \approx 37.52
$$

$$
\rho \approx 100 \cdot 1 \cdot e^{-0.05} \cdot 0.5596 \approx 53.23
$$

$$
\Theta \approx -6.41
$$

Interpretation:

- a one-unit increase in spot increases the option value by about $0.64$ locally.
- a one-unit squared spot move enters through a gamma of about $0.0188$.
- a volatility move of $0.01$ changes value by about $37.52 \times 0.01 \approx 0.375$.
- a one-point increase in the continuously compounded rate changes value by about $53.23 \times 0.01 \approx 0.532$.
- the annualized theta is about $-6.41$, so the option loses time value as expiry approaches when the other inputs are frozen.

## 3. Local P&L approximation

Let value depend on $S$, $\sigma$, $r$, and $t$. A second-order local approximation is

$$
\Delta V\approx \Delta\,\Delta S+\frac{1}{2}\Gamma(\Delta S)^2+Vega\,\Delta \sigma+\rho\,\Delta r+\Theta\,dt
$$

Where:

- $\frac{1}{2}\Gamma(\Delta S)^2$ is the convexity correction for the underlying move.
- $\Delta \sigma$ is the move in implied volatility.

This formula is useful for rapid explain and hedging diagnostics, but it is only a local approximation. It becomes less reliable for large shocks, barrier features, strong smile dynamics, or jump-to-default exposure.

## 4. Greeks and P&L explain

Let $P\&L_{market}$ be the market-move component of P&L. A simple Greeks-based approximation is

$$
P\&L_{market}\approx P\&L_{\Delta}+P\&L_{\Gamma}+P\&L_{Vega}+P\&L_{\rho}
$$

Where:

- $P\&L_{\Delta}$ is the first-order underlying contribution.
- $P\&L_{\Gamma}$ is the convexity contribution.
- $P\&L_{Vega}$ is the volatility contribution.
- $P\&L_{\rho}$ is the rate contribution.

Production systems often keep this approximation for diagnostics while using full revaluation for official numbers.

## 5. Numerical Greeks

When closed-form Greeks are unavailable, finite differences are used.

Let $h$ be a small bump size in the underlying. Then a central-difference delta is

$$
\Delta\approx \frac{V(S+h)-V(S-h)}{2h}
$$

Where:

- $h$ is the bump size.

The corresponding central-difference gamma is

$$
\Gamma\approx \frac{V(S+h)-2V(S)+V(S-h)}{h^2}
$$

Numerical Greeks are only meaningful if the platform also defines the bump convention clearly, especially for volatility surfaces and rates curves.

## 6. Scenario revaluation versus local approximation

Let $M^{base}$ be the base market state and let $M^{shock}$ be the shocked market state. Full scenario P&L is

$$
\Delta V=V(M^{shock})-V(M^{base})
$$

Where:

- $M^{base}$ is the base market state.
- $M^{shock}$ is the shocked market state.

A good platform should support both:

- local Greek approximations for fast diagnostics,
- full revaluation for official stress and non-linear books.
