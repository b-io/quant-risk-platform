# Options, Volatility, Monte Carlo and Stochastic Calculus

## Option chain

An option chain is a table of all listed options for one underlying.

It contains:

- expiry;
- strike;
- call bid/ask;
- put bid/ask;
- implied volatility;
- delta, gamma, vega, theta;
- volume;
- open interest.

Example:

| Expiry | Strike | Call bid | Call ask | Put bid | Put ask | Implied vol |
|--------|-------:|---------:|---------:|--------:|--------:|------------:|
| Sep-27 |     35 |      8.1 |      8.4 |     1.2 |     1.4 |         58% |
| Sep-27 |     40 |      5.0 |      5.3 |     3.0 |     3.2 |         60% |
| Sep-27 |     45 |      3.1 |      3.4 |     5.8 |     6.1 |         62% |

## Black-76 for options on futures

Call:

$$
C =
e^{-rT}
\left[
FN(d_1)-KN(d_2)
\right]
$$

Put:

$$
P =
e^{-rT}
\left[
KN(-d_2)-FN(-d_1)
\right]
$$

with:

$$
d_1 =
\frac{\ln(F/K)+\frac{1}{2}\sigma^2T}
{\sigma\sqrt{T}}
$$

$$
d_2 = d_1-\sigma\sqrt{T}
$$

Use for positive futures-style underlyings such as crude, gold, gas in simple settings.

## Bachelier / normal model

Useful when the underlying can be negative, especially power.

Call:

$$
C =
e^{-rT}
\left[
(F-K)N(d) +
\sigma_N\sqrt{T}\phi(d)
\right]
$$

with:

$$
d =
\frac{F-K}{\sigma_N\sqrt{T}}
$$

## Implied volatility

Implied volatility is the volatility that makes the model price equal the market price:

$$
C_{\text{model}}(\sigma_{\text{impl}}) =
C_{\text{market}}
$$

Numerically solve:

$$
f(\sigma)=C_{\text{model}}(\sigma)-C_{\text{market}}=0
$$

Newton update:

$$
\sigma_{n+1} =
\sigma_n -
\frac{C_{\text{model}}(\sigma_n)-C_{\text{market}}}
{\text{Vega}(\sigma_n)}
$$

Black-76 vega:

$$
\text{Vega} =
e^{-rT}F\phi(d_1)\sqrt{T}
$$

## Monte Carlo

Monte Carlo estimates an expectation by simulation.

Target:

$$
V = \mathbb{E}[g(X)]
$$

Estimator:

$$
\hat V_N =
\frac{1}{N}
\sum_{i=1}^{N} g(X_i)
$$

## Monte Carlo Convergence Rationale

Law of Large Numbers:

$$
\hat V_N \to V
\quad \text{as } N \to \infty
$$

Central Limit Theorem:

$$
\sqrt{N}(\hat V_N - V) \Rightarrow
N(0,\sigma_g^2)
$$

Standard error:

$$
SE(\hat V_N) =
\frac{s_g}{\sqrt{N}}
$$

## Risk-neutral pricing

For pricing:

$$
V_0 =
\mathbb{E}^{Q}
\left[
e^{-\int_0^T r_s ds}
\text{Payoff}
\right]
$$

For a call on a future:

$$
\hat C_0 =
e^{-rT}
\frac{1}{N}
\sum_{i=1}^{N}
\max(F_T^{(i)}-K,0)
$$

## Stochastic calculus

Stochastic calculus is used because prices are modelled as random processes.

Example:

$$
dS_t = \mu S_t dt + \sigma S_t dW_t
$$

where $W_t$ is Brownian motion.

Ordinary calculus is insufficient because Brownian paths are not differentiable.

Stochastic calculus gives:

- Ito's lemma;
- martingales;
- risk-neutral pricing;
- measure changes;
- simulation schemes;
- option pricing equations.

## Path generation: lognormal future

Under a simple risk-neutral futures model:

$$
dF_t = \sigma F_t dW_t
$$

Exact simulation:

$$
F_{t+\Delta t} =
F_t
\exp
\left[ -\frac{1}{2}\sigma^2\Delta t +
\sigma\sqrt{\Delta t}Z
\right]
$$

where:

$$
Z \sim N(0,1)
$$

## Path generation: normal model

For power where negative prices matter:

$$
dF_t = \sigma_N dW_t
$$

Simulation:

$$
F_{t+\Delta t} =
F_t + \sigma_N\sqrt{\Delta t}Z
$$

## Correlated paths

For multiple risk factors, use correlation matrix $\rho$.

Cholesky:

$$
\rho = LL^\top
$$

Generate independent normals:

$$
Z \sim N(0,I)
$$

Create correlated shocks:

$$
\epsilon = LZ
$$

## Implementation Notes

Monte Carlo valuation estimates discounted expectations by simulating future paths and averaging payoffs. The Law of
Large Numbers gives convergence, while the Central Limit Theorem gives the standard-error estimate:

$$
SE(\hat V_N) = \frac{s_g}{\sqrt{N}}.
$$

For pricing, paths are usually generated under a risk-neutral measure. For risk, paths are often generated under a
real-world or historically calibrated measure. The platform should therefore store the measure, model parameters, random
seed, time grid, path count, and variance-reduction settings with each simulation result.
