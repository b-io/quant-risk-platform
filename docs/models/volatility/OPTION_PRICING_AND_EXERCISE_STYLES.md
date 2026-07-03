# Option Pricing and Exercise Styles

This chapter explains how option-pricing design choices fit together. It connects:

- Black-Scholes-Merton and Black 76,
- European, American, and Bermudan exercise,
- lattice, PDE, and Monte Carlo methods,
- and the implementation question of which model family belongs in which production workflow.

The goal is not just to list formulas, but to explain how pricing platforms organize the problem.

---

## 1. Three separate choices that often get confused

The phrase "option model" often combines three different decisions:

1. **payoff and exercise style** - European, American, Bermudan, barrier, Asian, callable, and so on,
2. **stochastic model for the underlying** - Black-Scholes, local vol, Heston, Hull-White, LMM, SABR,
3. **numerical method** - closed form, tree, finite difference, Monte Carlo, Fourier, regression Monte Carlo.

A robust platform keeps those choices conceptually separate.

Example:

- a European call on equity may use Black-Scholes-Merton in closed form,
- an American put may use the same diffusion but require a tree or PDE solver,
- a Bermudan swaption may use a short-rate or LGM model on a lattice,
- an autocallable note may require Monte Carlo under a local-stochastic-vol hybrid.

---

## 2. Exercise styles

### 2.1 European options

A European option can be exercised only at maturity.

That makes the pricing problem comparatively simple because there is no early-exercise decision. Many closed-form or
semi-analytic formulas are available.

Typical examples:

- listed equity index vanilla options,
- FX vanillas,
- caplets,
- many standard swaptions.

### 2.2 American options

An American option can be exercised at any time up to expiry.

Let $V^{Am}(t,S)$ be the American option value and let $H(t,S)$ be the immediate-exercise value. Then

$$
V^{Am}(t,S) = \sup_{\tau \in [t,T]} \mathbb{E}^{\mathbb{Q}}_t \left[e^{-r(\tau-t)} H(\tau, S_\tau)\right]
$$

Where:

- $V^{Am}(t,S)$ is the American option value at time $t$.
- $H(t,S)$ is the payoff obtained by exercising immediately.
- $\tau$ is an admissible stopping time between $t$ and $T$.
- $T$ is maturity.
- $\mathbb{Q}$ is the risk-neutral measure.

In a Markovian one-factor setting the same idea can be written as a dynamic programming equation:

$$
V^{Am}(t,S) = \max\left(H(t,S), V^{cont}(t,S)\right)
$$

Where:

- $V^{cont}(t,S)$ is the continuation value, meaning the discounted expected value of waiting rather than exercising now.

This extra flexibility makes an American option worth at least as much as the corresponding European option.

Important practical facts:

- an American call on a non-dividend-paying stock is generally not exercised early,
- an American put can be exercised early,
- an American call on a dividend-paying stock can also be exercised early.

### 2.3 Bermudan options

A Bermudan option can be exercised only on a finite set of dates.

This is extremely common in rates and structured products.

Examples:

- Bermudan swaptions,
- callable bonds,
- cancelable swaps,
- callable exotics with scheduled notice dates.

Bermudan exercise is a natural bridge between European simplicity and full American flexibility.

---

## 3. Black-Scholes-Merton in the correct place

Black-Scholes-Merton is a model for pricing **European-style** options under geometric Brownian motion.

For an asset following:

$$
dS_t = \mu S_t \, dt + \sigma S_t \, dW_t,
$$

a European payoff $X_T$ is priced under the risk-neutral measure as:

$$
V_t = B_t \, \mathbb{E}_t^{\mathbb{Q}}\left[\frac{X_T}{B_T}\right].
$$

For a non-dividend-paying European call:

$$
C = S_0 N(d_1) - K e^{-rT} N(d_2),
$$

where

$$
d_1 = \frac{\ln(S_0/K) + (r + \tfrac12 \sigma^2)T}{\sigma \sqrt{T}},
\qquad
d_2 = d_1 - \sigma \sqrt{T}.
$$

Role in the model stack:

- it is the benchmark quoting language for equity options,
- it anchors implied-volatility surfaces,
- it remains the cleanest introduction to replication and Greeks,
- even when a richer model is used, implied volatility is often still reported in Black-Scholes language.

### Numerical example: European call

Suppose:

- spot $S_0 = 100$,
- strike $K = 100$,
- rate $r = 5\%$,
- volatility $\sigma = 20\%$,
- maturity $T = 1$ year.

Then:

$$
d_1 = \frac{0 + (0.05 + 0.5\cdot 0.2^2)}{0.2} = 0.35,
\qquad
d_2 = 0.15.
$$

Using $N(0.35) \approx 0.6368$ and $N(0.15) \approx 0.5596$:

$$
C \approx 100\cdot 0.6368 - 100 e^{-0.05}\cdot 0.5596 \approx 10.45.
$$

---

## 4. Black 76 for Rates and Commodity Options

For many options on forwards or futures, the benchmark model is not spot Black-Scholes but **Black 76**.

For forward $F_0$ and discount factor $P(0,T)$:

$$
C = P(0,T)\left(F_0 N(d_1) - K N(d_2)\right),
$$

with

$$
d_1 = \frac{\ln(F_0/K) + \tfrac12 \sigma^2 T}{\sigma\sqrt{T}},
\qquad
d_2 = d_1 - \sigma\sqrt{T}.
$$

This is the natural benchmark for:

- caplets,
- floorlets,
- swaptions in Black-vol quoting,
- commodity options on futures.

The practical difference is that the stochastic object is the forward or futures price, not the spot asset with a
carry-adjusted drift.

### Numerical example: Black 76 call on a forward

Suppose:

- forward $F_0 = 3.00\%$,
- strike $K = 3.20\%$,
- maturity $T = 1$,
- Black volatility $\sigma = 20\%$,
- discount factor $P(0,T) = 0.97$.

Then:

$$
d_1 = \frac{\ln(0.03/0.032)+0.5\cdot 0.2^2}{0.2} \approx -0.2233,
\qquad
d_2 \approx -0.4233.
$$

Using $N(d_1) \approx 0.4117$ and $N(d_2) \approx 0.3360$:

$$
C \approx 0.97\bigl(0.03\cdot 0.4117 - 0.032\cdot 0.3360\bigr) \approx 0.00154.
$$

So the option premium is about 15.4 basis points of forward notional before any annuity scaling for a swaption-style
application.

---

## 5. American Options and the Absence of a General Closed Form

For an American option, the holder chooses whether to exercise early. This makes pricing a dynamic programming problem,
not just a terminal expectation.

At every time step the value is:

$$
V = \max(\text{exercise now}, \text{discounted continuation}).
$$

American options are therefore normally priced with:

- **binomial or trinomial trees**,
- **finite difference PDE methods**,
- **Longstaff-Schwartz regression Monte Carlo** for high-dimensional problems.

### Important practical result

For a non-dividend-paying stock:

$$
C_{\text{American}} = C_{\text{European}}
$$

for a plain vanilla call.

Early exercise destroys time value and requires paying the strike before maturity.

For puts, early exercise can be optimal, especially when the put is deep in the money and interest rates are positive.

### Numerical example: one-step American put

Suppose today:

- stock price $S_0 = 40$,
- strike $K = 50$,
- next-step discount factor $0.98$,
- continuation value from the tree = 8.70.

Immediate exercise gives:

$$
K - S_0 = 10.
$$

So the American put value at that node is:

$$
\max(10, 8.70) = 10.
$$

The early exercise decision is optimal here.

---

## 5.1 European versus American prices in theory and in practice

Let $V^{Eu}$ be the European option value and let $V^{Am}$ be the American option value for the same payoff family, strike, and final maturity. Then

$$
V^{Am} \ge V^{Eu}
$$

Where:

- $V^{Am}$ is the American option value.
- $V^{Eu}$ is the European option value.

The inequality holds because the American holder can always choose to behave like a European holder and exercise only at maturity.

### Calls on non-dividend-paying stocks

Let $C^{Am}$ be an American call and let $C^{Eu}$ be the corresponding European call on a non-dividend-paying stock. Then

$$
C^{Am} = C^{Eu}
$$

Where:

- $C^{Am}$ is the American call price.
- $C^{Eu}$ is the European call price.

Meaning:

- early exercise gives the holder the stock but destroys remaining time value,
- the strike must be paid earlier,
- there is no dividend benefit to compensate for that lost optionality.

So for a plain non-dividend-paying stock call, the early-exercise right has zero value.

### Puts on non-dividend-paying stocks

Let $P^{Am}$ be an American put and let $P^{Eu}$ be the corresponding European put. Then

$$
P^{Am} \ge P^{Eu}
$$

Where:

- $P^{Am}$ is the American put price.
- $P^{Eu}$ is the European put price.

Meaning:

- if the put is deep in the money and rates are positive, exercising early can be optimal because the strike is received sooner,
- the early-exercise right therefore has positive value in many states.

### Calls on dividend-paying stocks

Let $C^{Am}_{div}$ be an American call on a dividend-paying stock and let $C^{Eu}_{div}$ be the corresponding European call. Then

$$
C^{Am}_{div} \ge C^{Eu}_{div}
$$

Where:

- $C^{Am}_{div}$ is the American call price when dividends matter.
- $C^{Eu}_{div}$ is the European call price for the same contractual payoff and maturity.

Meaning:

- exercising just before an ex-dividend date can be optimal if the foregone dividend is large enough relative to the remaining time value.

### Practical view

In theory the American premium is

$$
\text{American premium} = V^{Am} - V^{Eu}
$$

Where:

- $V^{Am} - V^{Eu}$ measures the value of the early-exercise right.

Market implementation:

- index options are often European and are naturally priced with Black-Scholes or Black-style formulas,
- many single-name listed equity options are American and are priced with trees, finite differences, or approximation formulas,
- markets often still quote or monitor them using implied-volatility language even though the pricing engine itself is American,
- for rates products the closest practical analogue is often Bermudan rather than fully American exercise.

A useful implementation principle is:

- European options are usually calibration anchors because they are liquid and admit stable valuation formulas,
- American or Bermudan options are then priced on top of calibrated curves and volatility inputs using an exercise-aware engine.

---

## 6. Bermudan Options

Bermudan exercise is particularly important in fixed income because many callable structures have discrete exercise
windows aligned with coupon dates or call dates.

Typical examples:

- Bermudan swaptions,
- callable notes,
- cancelable swaps,
- mortgage-like embedded prepayment options at scheduled dates.

Bermudans are often priced with:

- trees under short-rate or low-dimensional factor models,
- finite differences for suitable low-dimensional PDEs,
- Monte Carlo plus backward induction or regression in higher dimensions.

Bermudan valuation is usually governed more by stable exercise-policy representation than by the richest possible smile
model.

---

## 7. Numerical methods and when they are used

### 7.1 Closed form

Best when:

- the payoff is European,
- the underlying model is simple,
- the product is liquid and standardized.

Examples:

- Black-Scholes European equity options,
- Black 76 caplets and simple swaptions,
- Bachelier normal-vol formulas for some rates options.

Trade-off:

- fast and transparent,
- but cannot capture early exercise or rich state dependence.

### 7.2 Trees and lattices

Best when:

- exercise is American or Bermudan,
- dimensionality is low,
- the application requires stable callable pricing and Greeks.

Examples:

- American puts,
- Bermudan swaptions under Hull-White or LGM,
- callable bond valuation.

Trade-off:

- intuitive and operationally robust,
- but less natural in high dimensions or rich cross-asset hybrids.

### 7.3 PDE / finite differences

Best when:

- the model is Markovian in a small number of state variables,
- early exercise matters,
- smooth Greeks are important.

Trade-off:

- excellent for low-dimensional problems,
- becomes computationally demanding in higher dimension.

### 7.4 Monte Carlo

Best when:

- path dependence matters,
- dimensionality is high,
- exposures or XVA-style workflows need a path engine anyway.

Trade-off:

- flexible,
- but early exercise requires regression or dual methods,
- Greeks and calibration noise need care.

---

## 8. Longstaff-Schwartz in one paragraph

Longstaff-Schwartz is the standard practical Monte Carlo approach for many American or Bermudan options in higher
dimensions.

The idea is:

1. simulate many paths forward,
2. start from maturity,
3. at each exercise date, estimate continuation value by regressing future discounted cash flows on basis functions of
   the state,
4. exercise when intrinsic value exceeds estimated continuation value.

This is not as exact or transparent as a tree in one dimension, but it scales better to high-dimensional path-dependent or
multi-factor problems.

---

## 9. Design implications for the platform

A pricing platform should not expose a single "option pricer" abstraction with hidden logic. It should separate:

- the **exercise style**,
- the **underlying model family**,
- the **numerical engine**,
- the **market objects** used for calibration,
- and the **risk-reporting semantics**.

In concrete terms an option pricer usually needs:

- curves,
- dividend or carry inputs,
- a volatility surface or model parameters,
- exercise schedule,
- payoff definition,
- engine choice,
- model diagnostics.

This separation explains how the same volatility surface can support different pricing engines for the same product
family.

---

## 10. Trade-offs by model family

### 10.1 Black-Scholes-Merton / Black 76

Advantages:

- transparent,
- fast,
- benchmark quoting language,
- simple Greeks,
- very stable operationally.

Limitations:

- constant volatility,
- no smile dynamics,
- no early exercise closed form,
- weak for exotic pricing if used literally.

### 10.2 Local volatility

Advantages:

- fits the vanilla surface exactly,
- preserves a one-factor diffusion structure,
- useful for many path-dependent exotics.

Limitations:

- smile dynamics can be implausible,
- hedging can disappoint when actual market dynamics are stochastic-vol driven.

### 10.3 Stochastic volatility

Advantages:

- better smile dynamics,
- better forward-smile behavior,
- more plausible for many exotics.

Limitations:

- calibration complexity,
- numerical cost,
- risk explanation requires broader diagnostics.

### 10.4 Short-rate / LGM / Hull-White type models for callable rates products

Advantages:

- support trees and Bermudan exercise well,
- integrate naturally with callable fixed-income products,
- operationally standard for many rates workflows.

Limitations:

- smile fit may be weaker than richer market models,
- not the natural quoting model for every vanilla surface.

---

## 11. Implementation Pattern

The implementation pattern is modular:

- **equity vanilla books** often quote and risk-manage in Black-Scholes implied-vol terms,
- **equity exotics** often use local vol, stochastic vol, or local-stochastic-vol hybrids,
- **FX vanilla** often uses delta-based surfaces with Black-style pricing and smile parameterizations,
- **rates vanillas** often use Black, normal, or shifted-lognormal quoting with SABR-type smile layers,
- **callable and Bermudan rates products** often use Hull-White, LGM, or tree-based frameworks because exercise logic
  matters as much as smile fit,
- **high-dimensional callable exotics** often require Monte Carlo with regression methods.

The preferred production model is not necessarily the most theoretically ambitious one. It is the one that balances:

- calibration quality,
- hedge stability,
- explainability,
- computational speed,
- and operational robustness.
