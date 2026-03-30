# Option Pricing Foundations and Put-Call Parity

## 1. Why this chapter matters

Put-call parity is one of the cleanest no-arbitrage identities in quantitative finance. It is not just a textbook formula:

- it links calls, puts, forwards, discounting, and carry,
- it helps detect inconsistent market quotes,
- it explains synthetic positions used by traders and structurers,
- it clarifies why option pricing must be consistent with the forward curve and the funding/dividend setup,
- it gives a practical bridge between introductory Black-Scholes reasoning and production pricing systems.

A good implementation team should treat put-call parity as both:

- a **theoretical identity** coming from static replication,
- a **production control** used to validate option surfaces, dividend assumptions, borrow assumptions, and discount curves.

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

This is the backbone of put-call parity. The difference between a call and a put with the same strike and maturity is just a forward-style linear payoff.

---

## 3. European, American, and Bermudan exercise styles

Before writing parity formulas, distinguish exercise rights:

- **European**: exercise only at maturity,
- **American**: exercise at any time up to maturity,
- **Bermudan**: exercise only on a discrete set of dates.

The clean, exact put-call parity identity applies most directly to **European** options. For American options, early-exercise rights disturb the exact equality and replace it with bounds or inequalities.

That is why parity is a foundational identity for option pricing, while American-option pricing usually requires trees, PDEs, or regression-based Monte Carlo.

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

If the continuously compounded risk-free rate is $r$, the amount borrowed today is $K e^{-rT}$. At maturity the loan repayment is $K$, so terminal payoff is:

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
K e^{-rT} = 100 e^{-0.05} pprox 95.12
$$

So parity gives:

$$
C_0 - P_0 = 100 - 95.12 = 4.88
$$

If the call is quoted at $10.40$, then the parity-consistent put must be:

$$
P_0 = 10.40 - 4.88 = 5.52
$$

If instead the market quoted the put at $7.00$, the pair would violate parity under the stated assumptions. That would immediately tell the desk to re-check one or more of:

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
	ext{long call} - 	ext{long put} = 	ext{long forward} - 	ext{financing adjustment}
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

If the stock pays a known continuous dividend yield $q$, then the present value of holding the stock to maturity is reduced by carry. The parity becomes:

$$
C_0 - P_0 = S_0 e^{-qT} - K e^{-rT}
$$

This is often the better production formula because many equity-index and FX settings naturally involve carry-like terms.

A more general carry notation is:

$$
C_0 - P_0 = S_0 e^{-cT} - K e^{-rT}
$$

where $c$ is a net carry term. Depending on asset class, that carry may reflect:

- dividend yield,
- foreign interest rate in FX,
- storage/convenience yield in commodities,
- securities lending or borrow effects in equities.

### Numerical example with dividends

Suppose:

- $S_0 = 100$,
- $K = 100$,
- $r = 4\%$,
- $q = 1.5\%$,
- $T = 2$ years.

Then:

$$
S_0 e^{-qT} = 100 e^{-0.03} pprox 97.04
$$

$$
K e^{-rT} = 100 e^{-0.08} pprox 92.31
$$

Hence:

$$
C_0 - P_0 pprox 97.04 - 92.31 = 4.73
$$

So if the call is priced at $11.20$, the parity-consistent put is:

$$
P_0 pprox 11.20 - 4.73 = 6.47
$$

---

## 8. Forward-based form of parity

In practice, many desks reason through the forward rather than spot directly.

For a forward price $F_0(T)$ and discount factor $P(0,T)$:

$$
C_0 - P_0 = P(0,T)igl(F_0(T)-Kigr)
$$

This is the most portable version across asset classes.

Why it matters:

- in rates, FX, equities, and commodities, forwards often carry the market conventions more naturally than spot,
- it separates **forward construction** from **discounting**,
- it makes clear that option consistency depends on both the forward input and the discount curve.

### Numerical example with a forward

Suppose:

- forward price $F_0(T)=103$,
- strike $K=100$,
- discount factor $P(0,T)=0.95$.

Then:

$$
C_0 - P_0 = 0.95(103-100)=2.85
$$

So if a quoted call is $8.10$, the parity-consistent put is:

$$
P_0 = 8.10 - 2.85 = 5.25
$$

This is often how parity checks are implemented in production validators.

---

## 9. Arbitrage argument behind parity

Parity is a no-arbitrage statement. If it does not hold, a trader can construct a static arbitrage.

Suppose the market has:

$$
C_0 - P_0 > S_0 - K e^{-rT}
$$

Then the call-minus-put portfolio is too expensive relative to stock-minus-bond. A trader could:

- sell the overpriced side: short call, long put,
- buy the underpriced side: long stock, borrow the discounted strike.

The initial trade generates positive cash inflow, and the terminal payoffs offset each other exactly. That is arbitrage.

If the inequality goes the other way, reverse the trade.

In practice, bid-offer spreads, funding charges, stock borrow costs, dividends, and execution frictions mean the control is not a pure equality at executable prices. But the logic remains exactly the same.

---

## 10. Practical arbitrage bounds for European options

Parity sits inside a broader family of no-arbitrage bounds.

For a non-dividend-paying stock:

### Call bounds

$$
\max(0, S_0 - K e^{-rT}) \le C_0 \le S_0
$$

### Put bounds

$$
\max(0, K e^{-rT} - S_0) \le P_0 \le K e^{-rT}
$$

These bounds matter operationally because they catch obvious data errors before any model calibration starts.

Example: if a call on a stock trading at $100$ is quoted at $104$, that quote is immediately invalid because a call cannot exceed the stock price under standard settlement assumptions.

---

## 11. What changes for American options

For **American** options, exact European parity becomes more subtle because of early exercise.

### American calls on non-dividend-paying stocks

Early exercise is not optimal, so the American call equals the European call. In that special case, the European call side still behaves cleanly.

### American puts

Early exercise can be optimal, especially when:

- the put is deep in the money,
- interest rates are high,
- volatility is relatively low,
- time to maturity is short.

So American puts usually do **not** satisfy the exact European parity identity.

Instead one works with inequalities rather than a single equality.

### Practical message

Use exact parity checks only when you are certain that:

- both options are European,
- dividend/carry assumptions are correct,
- discounting and settlement conventions are aligned.

For American or Bermudan products, parity intuition still helps, but pricing requires explicit early-exercise modeling.

---

## 12. Relation to Black-Scholes-Merton and Black-style pricing

Put-call parity does **not** depend on Black-Scholes-Merton. It is more primitive than the model.

- parity comes from **static replication** and no-arbitrage,
- Black-Scholes-Merton adds a specific diffusion model and dynamic hedging argument,
- if Black-Scholes prices are internally consistent, they will automatically satisfy parity.

This distinction matters in implementation:

- parity is a quote-consistency and market-data control,
- Black-Scholes, local vol, stochastic vol, and tree/PDE models are pricing engines.

A system should enforce parity-style checks **before** trusting model calibration.

---

## 13. What parity tells you about implied volatility surfaces

Parity places strong constraints on volatility-surface construction even though it is not itself a volatility model.

For a fixed maturity and strike pair, if call and put prices are quoted separately, then:

- they must imply the same forward,
- they must imply the same discounting/carry assumptions,
- they must not create synthetic-arbitrage violations.

In practice this means:

- cleaning call and put quotes together, not independently,
- fitting a surface in a way consistent with forwards and discount curves,
- checking calendar, strike, and parity constraints jointly.

A desk that calibrates a smile to calls only and then infers puts without matching carry inputs can create hidden arbitrage inconsistencies.

---

## 14. Production design implications

A practitioner-oriented pricing platform should expose parity explicitly.

### 14.1 Market data layer

Store enough information to evaluate parity correctly:

- spot,
- forward or dividend/borrow inputs,
- discount curve,
- exercise style,
- settlement convention,
- quote source and timestamp.

### 14.2 Validation layer

Implement controls such as:

- European parity residual,
- lower/upper option-price bounds,
- monotonicity in strike,
- convexity in strike,
- calendar consistency.

A simple parity residual is:

$$
arepsilon_{	ext{parity}} = igl(C_0 - P_0igr) - P(0,T)igl(F_0(T)-Kigr)
$$

This should be near zero after allowing for spreads and small numerical tolerances.

### 14.3 Analytics layer

Use parity to:

- infer missing put quotes from calls or vice versa,
- decompose positions into synthetic forwards and optionality,
- explain hedge slippage when carry or borrow assumptions change,
- detect inconsistencies between options and underlying forwards.

---

## 15. Common practitioner pitfalls

### Pitfall 1: wrong dividend assumptions

A parity break in equity options often comes from using the wrong dividend term structure rather than from “mispriced vol.”

### Pitfall 2: mixing European and American contracts

Some listed equity options are American, while many index options are European. Treating them as interchangeable can create apparent parity violations that are not true arbitrage.

### Pitfall 3: ignoring borrow and short-sale frictions

In single-name equities, hard-to-borrow effects can materially change practical executable relationships.

### Pitfall 4: ignoring settlement conventions

Cash-settled vs. physically settled products, premium timing, and futures-style margining can alter the operational form of the parity check.

### Pitfall 5: calibrating vol first, consistency later

The correct workflow is usually:

1. clean quotes,
2. align spot/forward/discount/dividend inputs,
3. check parity and bounds,
4. only then fit the volatility surface.

---

## 16. Summary

Put-call parity is one of the most useful identities in pricing because it links:

- vanilla option payoffs,
- forwards,
- discounting,
- carry,
- static replication,
- market-data validation.

The core formulas to remember are:

### No dividends

$$
C_0 - P_0 = S_0 - K e^{-rT}
$$

### Continuous dividend yield or general carry

$$
C_0 - P_0 = S_0 e^{-qT} - K e^{-rT}
$$

### Forward form

$$
C_0 - P_0 = P(0,T)igl(F_0(T)-Kigr)
$$

For European options, these are exact no-arbitrage equalities. For American options, the clean equality becomes less direct because of early exercise.

From a practical point of view, parity is not just a formula to memorize. It is a daily implementation and quality-control tool in any serious option-pricing platform.
