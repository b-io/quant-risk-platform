# Option Pricing and Exercise Styles

This chapter explains how option-pricing design choices fit together in practice. It connects:

- Black-Scholes-Merton and Black 76,
- European, American, and Bermudan exercise,
- lattice, PDE, and Monte Carlo methods,
- and the practical question of which model family belongs in which production workflow.

The goal is not just to list formulas, but to explain how desks and pricing platforms actually organize the problem.

---

## 1. Three separate choices that often get confused

When practitioners say “the option model,” they often mix together three different decisions:

1. **payoff and exercise style** — European, American, Bermudan, barrier, Asian, callable, and so on,
2. **stochastic model for the underlying** — Black-Scholes, local vol, Heston, Hull-White, LMM, SABR,
3. **numerical method** — closed form, tree, finite difference, Monte Carlo, Fourier, regression Monte Carlo.

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

The pricing problem is therefore an **optimal stopping** problem:

$$
V(t,S)=
\max\bigl(
\text{intrinsic value},
\text{continuation value}
\bigr).
$$

This extra flexibility makes American options worth at least as much as the corresponding European options.

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

Why practitioners still care:

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

## 4. Black 76 and why rates and commodity desks use it

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
C \approx 0.97\bigl(0.03\cdot 0.4117 - 0.032\cdot 0.3360\bigr)
\approx 0.00154.
$$

So the option premium is about 15.4 basis points of forward notional before any annuity scaling for a swaption-style
application.

---

## 5. Why American options do not usually have a simple closed form

For an American option, the holder chooses whether to exercise early. This makes pricing a dynamic programming problem,
not just a terminal expectation.

At every time step the value is:

$$
V = \max(\text{exercise now}, \text{discounted continuation}).
$$

That is why American options are normally priced with:

- **binomial or trinomial trees**,
- **finite difference PDE methods**,
- **Longstaff-Schwartz regression Monte Carlo** for high-dimensional problems.

### Important practical result

For a non-dividend-paying stock:

$$
C_{\text{American}} = C_{\text{European}}
$$

for a plain vanilla call.

Why? Exercising early destroys time value and requires paying the strike too soon.

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

## 6. Bermudan options in practice

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

A practitioner takeaway is that Bermudan valuation is usually less about having the fanciest smile model and more about
having a stable exercise policy under a model that supports early exercise cleanly.

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
- the desk needs stable callable pricing and Greeks.

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
- quickly becomes difficult in higher dimension.

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

This is not as exact or transparent as a tree in one dimension, but it scales better to realistic path-dependent or
multi-factor problems.

---

## 9. Design implications for the platform

A pricing platform should not expose a single “option pricer” abstraction with hidden logic. It should separate:

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

This separation makes it much easier to explain why two desks can use the same volatility surface but different pricing
engines for the same product family.

---

## 10. Trade-offs by model family

### 10.1 Black-Scholes-Merton / Black 76

Advantages:

- transparent,
- fast,
- benchmark quoting language,
- easy Greeks,
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

- smile dynamics can be unrealistic,
- hedging can disappoint when actual market dynamics are stochastic-vol driven.

### 10.3 Stochastic volatility

Advantages:

- better smile dynamics,
- better forward-smile behavior,
- more realistic for many exotics.

Limitations:

- calibration complexity,
- numerical cost,
- risk explanation is harder.

### 10.4 Short-rate / LGM / Hull-White type models for callable rates products

Advantages:

- support trees and Bermudan exercise well,
- integrate naturally with callable fixed-income products,
- operationally standard on many rates desks.

Limitations:

- smile fit may be weaker than richer market models,
- not the natural quoting model for every vanilla surface.

---

## 11. Modern practice

Modern practice is modular:

- **equity vanilla books** often quote and risk-manage in Black-Scholes implied-vol terms,
- **equity exotics** often use local vol, stochastic vol, or local-stochastic-vol hybrids,
- **FX vanilla** often uses delta-based surfaces with Black-style pricing and smile parameterizations,
- **rates vanillas** often use Black, normal, or shifted-lognormal quoting with SABR-type smile layers,
- **callable and Bermudan rates products** often use Hull-White, LGM, or tree-based frameworks because exercise logic
  matters as much as smile fit,
- **high-dimensional callable exotics** often require Monte Carlo with regression methods.

The recurring lesson is that the best production model is usually not the most theoretically ambitious one. It is the
one that balances:

- calibration quality,
- hedge stability,
- explainability,
- computational speed,
- and operational robustness.
