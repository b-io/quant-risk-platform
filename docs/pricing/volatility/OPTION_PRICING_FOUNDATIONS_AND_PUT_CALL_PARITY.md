# Option Pricing Foundations and Put-Call Parity

## 1. Why this chapter matters

Put-call parity is one of the cleanest no-arbitrage identities in quantitative finance. It is not just a textbook
formula:

- it links calls, puts, forwards, discounting, and carry,
- it helps detect inconsistent market quotes,
- it explains synthetic positions used by traders and structurers,
- it clarifies why option pricing must be consistent with the forward curve and the funding/dividend setup,
- it provides a practical bridge between introductory Black-Scholes reasoning and production pricing systems.

A robust implementation treats put-call parity as both:

- a **theoretical identity** coming from static replication,
- a **production control** used to validate option surfaces, dividend assumptions, borrow assumptions, and discount
  curves.

---

## 2. Option payoff recap

For a European option with maturity $T$ and strike $K$:

- call payoff:

$$
C_T = (S_T - K)^+
$$

- put payoff:

$$
P_T = (K - S_T)^+
$$

where $x^+ = \max(x,0)$.

A crucial algebraic identity holds for every terminal stock price $S_T$:

$$
(S_T-K)^+ - (K-S_T)^+ = S_T - K
$$

This is the backbone of put-call parity. The difference between a call and a put with the same strike and maturity is
just a forward-style linear payoff.

---

## 3. European, American, and Bermudan exercise styles

Before writing parity formulas, distinguish exercise rights:

- **European**: exercise only at maturity,
- **American**: exercise at any time up to maturity,
- **Bermudan**: exercise only on a discrete set of dates.

The clean, exact put-call parity identity applies most directly to **European** options. For American options,
early-exercise rights disturb the exact equality and replace it with bounds or inequalities.

That is why parity is a foundational identity for option pricing, while American-option pricing usually requires trees,
PDEs, or regression-based Monte Carlo.

---

## 4. Put-call parity for a non-dividend-paying stock

Consider European call and put prices today, $C_0$ and $P_0$, on the same stock, strike $K$, and maturity $T$.

Construct two portfolios:

### Portfolio A

- long one call,
- short one put.

Terminal payoff:

$$
(S_T-K)^+ - (K-S_T)^+ = S_T - K
$$

### Portfolio B

- long one share of stock,
- borrow the present value of $K$.

If the continuously compounded risk-free rate is $r$, the amount borrowed today is $K e^{-rT}$. At maturity the loan
repayment is $K$, so terminal payoff is:

$$
S_T - K
$$

Since both portfolios have the same payoff at maturity, no-arbitrage implies they must have the same value today:

$$
C_0 - P_0 = S_0 - K e^{-rT}
$$

Equivalently,

$$
C_0 + K e^{-rT} = P_0 + S_0
$$

This is the classical **put-call parity** formula.

Interpretation:

- **call + bond** = **put + stock**,
- the option pair can replicate a synthetic forward,
- parity enforces internal consistency between option prices and the forward-implied spot/discount relation.

---

## 5. Numerical example: basic parity

Suppose:

- $S_0 = 100$,
- $K = 100$,
- $r = 5\%$,
- $T = 1$ year,
- no dividends.

Then the discounted strike is:

$$
K e^{-rT} = 100 e^{-0.05} \approx 95.12
$$

So parity gives:

$$
C_0 - P_0 = 100 - 95.12 = 4.88
$$

If the call is quoted at $10.40$, then the parity-consistent put must be:

$$
P_0 = 10.40 - 4.88 = 5.52
$$

If instead the market quoted the put at $7.00$, the pair would violate parity under the stated assumptions. That would
immediately signal that one or more of the following inputs or conventions should be re-checked:

- dividend assumptions,
- borrow/funding assumptions,
- stale quotes,
- exercise-style mismatch,
- settlement-convention mismatch,
- surface-construction or cleaning errors.

---

## 6. Synthetic positions implied by parity

Put-call parity is most useful when rewritten as synthetic positions.

From

$$
C_0 - P_0 = S_0 - K e^{-rT}
$$

we get:

### Synthetic forward

$$
\text{long call} - \text{long put} = \text{long forward} - \text{financing adjustment}
$$

More precisely, the option pair reproduces the payoff of a forward struck at $K$.

### Synthetic stock

$$
S_0 = C_0 - P_0 + K e^{-rT}
$$

So:

- long call,
- short put,
- long zero-coupon bond paying $K$

replicates the stock.

### Fiduciary call

$$
C_0 + K e^{-rT} = P_0 + S_0
$$

The left-hand side is often called a **fiduciary call**:

- buy a call,
- buy enough bond to pay the strike at expiry.

This has the same payoff as:

- buy a stock,
- buy a put.

That second combination is the classic **protective put**.

This identity is widely used in trading explanations and hedge decomposition.

---

## 7. Parity with continuous dividends or general carry

If the stock pays a known continuous dividend yield $q$, then the present value of holding the stock to maturity is
reduced by carry. The parity becomes:

$$
C_0 - P_0 = S_0 e^{-qT} - K e^{-rT}
$$

A more general cost-of-carry representation is:

$$
C_0 - P_0 = P(0,T)\bigl(F_0(T)-K\bigr)
$$

where:

- $P(0,T)$ is the discount factor,
- $F_0(T)$ is the forward price for delivery at $T$.

This form is especially useful because it extends naturally beyond equities to FX, commodities, and rates-style carry
setups.

---

## 8. Numerical example: parity with dividend yield

Suppose:

- $S_0 = 100$,
- $K = 100$,
- $r = 8\%$,
- $q = 3\%$,
- $T = 1$.

Then:

$$
S_0 e^{-qT} = 100 e^{-0.03} \approx 97.04
$$

and

$$
K e^{-rT} = 100 e^{-0.08} \approx 92.31
$$

So:

$$
C_0 - P_0 \approx 97.04 - 92.31 = 4.73
$$

If the call price is $11.20$, then the corresponding parity-consistent put is:

$$
P_0 \approx 11.20 - 4.73 = 6.47
$$

---

## 9. Forward form and practical importance

The forward-form identity

$$
C_0 - P_0 = P(0,T)\bigl(F_0(T)-K\bigr)
$$

is one of the most practical versions of parity.

It says the call-minus-put spread is simply the discounted value of the forward moneyness $F_0(T)-K$.

This is useful because modern pricing systems often store or derive:

- discount curves,
- dividend or carry curves,
- forward curves,
- option surfaces keyed by forward moneyness.

In that environment, parity is not a separate academic check. It is a direct consistency relation among core market
objects.

---

## 10. Arbitrage interpretation

If parity is violated, an arbitrage portfolio exists under the model assumptions.

For example, if

$$
C_0 - P_0 > S_0 - K e^{-rT}
$$

then the call-minus-put side is too expensive relative to the stock-minus-bond side.

A zero-net-investment arbitrage is then constructed by:

- shorting the expensive side,
- buying the cheap side.

That means:

- short call,
- long put,
- long stock,
- borrow by shorting the bond position appropriately.

At maturity, the payoffs cancel, but the initial setup produces a positive inflow.

In practice, exact arbitrage is softened by:

- bid-ask spreads,
- stock borrow costs,
- discrete dividends,
- early-exercise rights,
- settlement lags,
- margin and funding frictions.

Still, large parity breaks are a strong signal that the market data or assumptions are inconsistent.

---

## 11. Numerical example: parity residual as a control

Suppose a system stores:

- call price $C_0 = 9.80$,
- put price $P_0 = 6.10$,
- discount factor $P(0,T)=0.95$,
- forward $F_0(T)=104$,
- strike $K=100$.

The parity-implied difference is:

$$
P(0,T)\bigl(F_0(T)-K\bigr)=0.95\times 4=3.80
$$

The observed difference is:

$$
C_0 - P_0 = 9.80 - 6.10 = 3.70
$$

So the residual is:

$$
\varepsilon_{\text{parity}} = \bigl(C_0 - P_0\bigr) - P(0,T)\bigl(F_0(T)-K\bigr)
= 3.70 - 3.80 = -0.10
$$

A production system would compare $|\varepsilon_{\text{parity}}|$ with a tolerance reflecting:

- bid-ask widths,
- stale quote risk,
- interpolation artifacts,
- carry/dividend uncertainty,
- rounding conventions.

This is a standard surface-quality control.

---

## 12. Parity and implied vol surfaces

Parity matters directly when building implied volatility surfaces.

Suppose call and put quotes arrive independently from the market. If parity is ignored, the resulting cleaned call and
put grids can imply inconsistent forwards or inconsistent discount/dividend assumptions.

That causes downstream problems:

- unstable implied dividends,
- noisy forwards,
- calendar-spread violations,
- poor local-vol or stochastic-vol calibration,
- unstable hedge ratios,
- false arbitrage signals.

A robust workflow typically does the following:

1. build or ingest discount and carry/dividend assumptions,
2. convert option prices to a forward-consistent representation,
3. check parity residuals across strikes and maturities,
4. clean or reconcile inconsistent quotes,
5. build the surface only after those controls pass.

This is why parity is a foundational market-data and calibration control, not just a first-course formula.

---

## 13. American and Bermudan caveats

For **American options**, exact European parity no longer holds because early exercise has value.

Typical facts:

- for a non-dividend-paying stock, an American call has the same value as the European call,
- an American put can be worth more than the European put,
- for dividend-paying stocks, early exercise can matter for calls as well.

So American options satisfy **bounds** rather than the clean European equality.

For **Bermudan options**, the same idea applies: restricted early exercise introduces an exercise premium, so exact
European parity is replaced by inequalities or model-based comparisons.

---

## 14. Relation to Black-Scholes-Merton

Put-call parity does **not** depend on the full Black-Scholes-Merton model.

It comes from **static no-arbitrage replication** and therefore survives much more generally than the specific lognormal,
constant-volatility assumptions of BSM.

However, parity is perfectly consistent with BSM. In fact:

- BSM call and put formulas satisfy parity exactly,
- parity is often used as a sanity check when implementing closed-form formulas,
- parity is one of the easiest ways to catch sign mistakes in discounting, dividends, or forwards.

So parity is more primitive than BSM: it is a no-arbitrage identity that any sound option-pricing system must respect.

---

## 15. Practical design implications for pricing systems

A production pricing and market-data stack usually benefits from explicit parity controls.

### 15.1 Canonical stored objects

The most stable internal representation is usually built around:

- discount curve,
- carry/dividend curve,
- forward curve,
- option quotes or vol surface.

Parity then becomes a derived consistency rule.

### 15.2 Validation checks

Useful automated checks include:

- parity residual by strike and expiry,
- forward consistency inferred from call/put pairs,
- monotonicity and convexity checks across strikes,
- calendar consistency across expiries,
- residual heatmaps after quote cleaning.

### 15.3 Calibration pipeline role

Before calibrating local volatility, SABR, stochastic-volatility, or jump models, it is good practice to ensure the
option surface is parity-consistent. Otherwise the calibration absorbs data defects instead of model structure.

---

## 16. Summary

Put-call parity is one of the most useful identities in practical derivatives work.

It provides:

- an exact no-arbitrage relation for European calls and puts,
- a synthetic-position toolkit,
- a forward-consistency identity,
- a market-data cleaning rule,
- a surface-validation control,
- a bridge between introductory pricing theory and production implementation.

In compact form:

### Non-dividend-paying stock

$$
C_0 - P_0 = S_0 - K e^{-rT}
$$

### With carry or dividends

$$
C_0 - P_0 = P(0,T)\bigl(F_0(T)-K\bigr)
$$

These formulas belong at the core of any option-pricing documentation set because they explain not only how prices
relate mathematically, but also how market objects must stay consistent inside a real pricing platform.
