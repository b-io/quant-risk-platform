# Yield Curves, OIS Bootstrapping, Different Yield Types, and Recession Dynamics

## Notation used in this chapter

Unless stated otherwise:

- $0$ is today, the valuation date.
- $T$, $T_1$, and $T_2$ are maturities in years from today.
- $P(0,T)$ is the discount factor from today to $T$.
- $PV$ is present value.
- $r(0,T)$ is the continuously compounded zero rate to maturity $T$.
- $F(0;T_1,T_2)$ is the simple forward rate applying between $T_1$ and $T_2$.
- $\tau(T_1,T_2)$ is the accrual year fraction between $T_1$ and $T_2$ under the chosen day-count convention.
- $m$ is compounding frequency per year.
- $r$ is a quoted annualized rate when compounding frequency is being discussed.
- $ccy$ denotes the currency of the curve set.

## 1. What a yield curve is

A yield curve is the term structure of interest rates.

It tells you what the market-implied annualized rate is for different maturities $T$:
- 1M
- 3M
- 6M
- 1Y
- 2Y
- 5Y
- 10Y
- 30Y

Mathematically, the most fundamental object is usually the **discount factor**:

$$
P(0,T)
$$

Where:
- $P(0,T)$ is the discount factor from today to maturity $T$.
- $T$ is a maturity or future time, typically measured in years from today.
- $0$ denotes the valuation date, or “today,” when it appears in term-structure notation.

which is the present value today of 1 unit of currency received at time $T$.

From discount factors, you can derive a zero rate:

$$
r(0,T) = -\frac{\ln P(0,T)}{T}
$$

Where:
- $r(0,T)$ is the continuously compounded zero rate from today to maturity $T$.

and a forward rate between $T_1$ and $T_2$:

$$
F(0;T_1,T_2)
=
\frac{1}{\tau(T_1,T_2)}
\left(
\frac{P(0,T_1)}{P(0,T_2)} - 1
\right)
$$

where $T_1$ and $T_2$ are the start and end of the forward accrual period and $\tau(T_1,T_2)$ is the corresponding year fraction.

The key practical point is:

> The curve is not just a picture. It is a reusable pricing object from which discount factors, forwards, sensitivities, carry, and scenario shocks are derived.

#### Example for 1 — discount factor, discrete compounding, and continuous compounding

Suppose the 1Y annual rate is 4%.

If 100 is received in 1 year, then the present value is

$$
PV = \frac{100}{1.04} = 96.1538
$$

Where:
- $PV$ is the present value of the cash flow or instrument.

So the discount factor is

$$
P(0,1)=\frac{1}{1.04}=0.961538
$$

If the same 4% annualized rate is quoted with semiannual compounding, then

$$
P_{\text{semi}}(0,1)=\frac{1}{(1+0.04/2)^2}=0.961169
$$

With continuous compounding:

$$
P_{\text{cont}}(0,1)=e^{-0.04}=0.960789
$$

This is why a rates developer must always know:

- the quote convention,
- the day count,
- the compounding convention,
- the payment frequency.

Otherwise two rates that “look the same” can produce slightly different present values.

### 1.2 Why discrete compounding converges to continuous compounding

With compounding frequency $m$:

$$
P_m(0,T)=\frac{1}{\left(1+\frac{r}{m}\right)^{mT}}
$$

Where:
- $P_m(0,T)$ is the discount factor to maturity $T$ under compounding frequency $m$.
- $m$ is the compounding frequency per year.

As $m\to\infty$,

$$
\left(1+\frac{r}{m}\right)^{mT}\to e^{rT}
$$

so

$$
P_m(0,T)\to e^{-rT}
$$

This convergence comes from Euler's exponential limit, that is from the definition of the exponential function and Euler's number $e$.

Numerical example with $r=5\%$ and $T=1$:

- annual: 0.952381
- semiannual: 0.951814
- monthly: 0.951328
- continuous: 0.951229

### 1.3 When curve construction becomes a root-finding problem

Some curve steps are direct algebra. Others require solving a non-linear pricing equation for an unknown parameter or node.

A standard residual form is

$$
f(x)=PV_{\text{model}}(x)-PV_{\text{market}}=0
$$

Where:
- $x$ is the unknown parameter or node being solved for.
- $PV_{\text{model}}(x)$ is the model price as a function of that unknown.
- $PV_{\text{market}}$ is the observed market price or quote converted into price terms.

Newton-Raphson updates the current guess by

$$
x_{n+1}=x_n-\frac{f(x_n)}{f'(x_n)}
$$

Where:
- $n$ is the iteration index.
- $x_n$ is the current guess.
- $x_{n+1}$ is the next guess.
- $f'(x_n)$ is the derivative of the residual with respect to the unknown.

Typical production uses:
- implied bond yield from price,
- implied volatility from option price,
- CDS hazard-rate calibration,
- some bootstrap steps when the last node cannot be solved in closed form.

#### Example for 1.3 — implied bond yield from a market price

Suppose a 2Y annual-coupon bond has face value 100, annual coupon 4, and market price 100.50. The implied annually compounded yield $y$ solves

$$
\frac{4}{1+y}+\frac{104}{(1+y)^2}=100.50
$$

Define

$$
f(y)=\frac{4}{1+y}+\frac{104}{(1+y)^2}-100.50
$$

with derivative

$$
f'(y)=-\frac{4}{(1+y)^2}-\frac{208}{(1+y)^3}
$$

Start from $y_0=3.00\%=0.03$.

Then

$$
f(0.03)\approx 1.41347, \qquad f'(0.03)\approx -194.11985
$$

so

$$
y_1=0.03-\frac{1.41347}{-194.11985}\approx 0.0372814
$$

Evaluate again:

$$
f(0.0372814)\approx 0.0147527, \qquad f'(0.0372814)\approx -190.08657
$$

so

$$
y_2=0.0372814-\frac{0.0147527}{-190.08657}\approx 0.0373590
$$

The solution is approximately

$$
y\approx 3.7359\%
$$

This is the practical pattern: define a residual, differentiate it, and iterate until model and market match closely enough.

---

## 1A. Running worked example used in this chapter

Use the following simplified yearly-discounting example to keep the formulas concrete.

Suppose the USD collateralized discount factors are:

| Maturity | Discount factor $P(0,T)$ |
|---|---:|
| 1Y | 0.9600 |
| 2Y | 0.9180 |
| 3Y | 0.8750 |
| 4Y | 0.8330 |
| 5Y | 0.7920 |

#### Example for 1A — 5Y zero rate

$$
z(0,5) = -\frac{\ln(0.7920)}{5} \approx 4.66\%
$$

Where:
- $0$ denotes the valuation date, or “today,” when it appears in term-structure notation.

Interpretation:
- receiving USD 1 in 5 years is worth about USD 0.792 today,
- so the implied continuously compounded 5Y zero rate is about 4.66%.

#### Example for 1A — 1Y to 2Y forward rate using yearly accrual

$$
F(0;1,2)=\frac{P(0,1)}{P(0,2)}-1 = \frac{0.9600}{0.9180}-1 \approx 4.58\%
$$

Interpretation:
- the market-implied simple annual rate for borrowing from year 1 to year 2 is about 4.58%.

#### Example for 1A — quick bond PV

For a USD 100 notional annual fixed-rate bond paying 4% coupons and principal at maturity, the PV is

$$
PV = 4P(0,1)+4P(0,2)+4P(0,3)+4P(0,4)+104P(0,5)
$$

Where:
- $PV$ is the present value of the cash flow or instrument.

$$
PV = 4(0.9600+0.9180+0.8750+0.8330)+104(0.7920)=97.28
$$

Meaning:
- with this curve, a 4% 5Y annual bond is below par because the market zero rates are above 4%.

Good practice:
- always state compounding and accrual convention,
- always state whether the curve is OIS discounting or a sovereign bond yield curve,
- never mix bond yields, swap par rates, and discount factors without documenting the conversion.

## 1B. Forward discount factor between two dates

In practice, one of the most useful derived quantities is the **discount factor between two future dates**:

$$
P(T_1,T_2)=\frac{P(0,T_2)}{P(0,T_1)}
$$

Where:
- $P(0,T_1)$ is the discount factor from today to time $T_1$.
- $P(0,T_2)$ is the discount factor from today to time $T_2$.
- $T_1$ is the start date of a forward or accrual period, measured from today.
- $T_2$ is the end date of a forward or accrual period, measured from today.
- $0$ denotes the valuation date, or “today,” when it appears in term-structure notation.

This is what a forward-starting trade, carry analysis, or shifted-horizon risk engine really needs.

Using the running worked example:

$$
P(0,2)=0.9180, \qquad P(0,5)=0.7920
$$

so the forward discount factor from 2Y to 5Y is:

$$
P(2,5)=\frac{0.7920}{0.9180}=0.862745
$$

Interpretation:

- if year 2 were "today", then 1 received in year 5 would be worth 0.862745 at year 2,
- equivalently, the market is discounting the 3-year period from year 2 to year 5 at an annualized rate above zero.

The continuously compounded forward zero rate over the interval is:

$$
z(2,5)=-\frac{\ln(0.862745)}{3}\approx 4.918\%
$$

The simple annualized forward rate is:

$$
F(2,5)=\frac{1}{3}\left(\frac{1}{0.862745}-1\right)\approx 5.300\%
$$

### QuantLib mapping

Typical calls are:

```cpp
DiscountFactor df_0_2 = curve->discount(date2Y);
DiscountFactor df_0_5 = curve->discount(date5Y);
DiscountFactor df_2_5 = df_0_5 / df_0_2;

Rate fwd = curve->forwardRate(date2Y, date5Y,
                              Actual365Fixed(), Continuous).rate();
```

If you want a shifted curve object whose reference date is 2Y, QuantLib-style systems use an implied structure:

```cpp
Handle<YieldTermStructure> base(curve);
ext::shared_ptr<YieldTermStructure> fwdCurve =
    ext::make_shared<ImpliedTermStructure>(base, date2Y);
DiscountFactor df_2_5_again = fwdCurve->discount(date5Y);
```

This is the production-friendly way to explain forward discounting.

## 2. Why the yield curve matters

The yield curve matters because it drives:
- discounting of future cash flows,
- projection of floating-rate coupons,
- valuation of swaps, bonds, FRAs, futures, swaptions, CDS discount legs,
- risk measures such as DV01 / key-rate DV01,
- carry and roll-down,
- macro interpretation of growth, inflation, and policy.

In implementation terms:

> A rates platform is fundamentally a market-state construction problem. If the curve is wrong, valuation, P&L, hedging, and stress testing will all be wrong downstream.

---

## 3. Different kinds of yields and curves

Many people say “yield” loosely, but these are not the same thing.

### 3.1 OIS curve

The **OIS curve** is built from overnight-indexed swap instruments linked to the overnight reference rate:
- SOFR in USD,
- €STR in EUR,
- SONIA in GBP,
- SARON in CHF.

It is the standard collateralized discounting curve for modern derivatives.

It is often treated as the closest traded proxy to the collateralized risk-free term structure.

Why it matters:
- used for discounting collateralized derivatives,
- moves closely with expected policy rates,
- central for front-office rates and cross-asset pricing.

### 3.2 Sovereign yield curve

The **sovereign curve** is built from government bond yields:
- US Treasuries,
- German Bunds,
- Swiss Confederation bonds, etc.

This is not identical to OIS.

A sovereign yield reflects:
- expected policy path,
- term premium,
- bond supply / demand,
- liquidity differences,
- repo specialness,
- sovereign-specific effects.

Why it matters:
- it is the macro benchmark most people quote in the press,
- it is used to talk about recession signals such as curve inversion,
- it can diverge from OIS and swap curves.

### 3.3 Swap curve

The **swap curve** is built from IRS / OIS par swap rates.

It reflects:
- expected floating rates,
- discounting convention,
- balance-sheet and liquidity effects,
- swap spread effects relative to sovereigns.

Why it matters:
- swaps are core hedging and trading instruments,
- many rates books are managed in swap risk, not bond risk.

### 3.4 Corporate or credit yield curve

A **corporate yield** is usually:

$$
\text{corporate yield}
\approx
\text{base curve yield} + \text{credit spread} + \text{liquidity spread}
$$

where the base curve may be sovereign or OIS depending on desk and use case.

Why it matters:
- it embeds default risk and liquidity premium,
- it behaves very differently in recession than OIS or sovereign yields.

### 3.5 CDS curve

A CDS curve is not a “yield curve” in the bond sense.
It is usually a **survival probability** or **hazard rate** term structure calibrated from CDS spreads.

If hazard rate is $\lambda(t)$, then:

$$
Q(0,T)
=
\exp\left(
-\int_0^T \lambda(u)\,du
\right)
$$

Where:
- $Q(0,T)$ is the survival probability from today to time $T$.
- $u$ is the real argument of a characteristic function.
- $T$ is a maturity or future time, typically measured in years from today.
- $0$ denotes the valuation date, or “today,” when it appears in term-structure notation.

That curve is then used to price protection and compute CS01.

---

## 4. Why yields behave differently in a recession

This is one of the most important implementation points.

### 4.1 OIS and sovereign yields

In a recession or recession scare, the market often expects:
- weaker growth,
- lower inflation over time,
- future policy cuts.

That usually pushes **OIS** and **high-quality sovereign yields** lower, especially at the front and belly once cuts are expected.

### 4.2 Credit yields

At the same time, recession raises:
- default risk,
- downgrade risk,
- liquidity stress,
- risk aversion.

So **credit spreads widen**.

That means:

- the **base rate** may go down,
- but the **spread** may go up even more,
- so **credit yields can rise, or fall much less than sovereign yields**.

That is why global macro and credit teams differentiate carefully between:
- risk-free-ish curve moves,
- sovereign term-premium effects,
- credit spread effects.

A compact formula is:

$$
\Delta \text{credit yield}
\approx
\Delta \text{base rate}
+
\Delta \text{credit spread}
+
\Delta \text{liquidity premium}
$$

In recession:
- $\Delta \text{base rate}$ is often negative,
- $\Delta \text{credit spread}$ is often positive,
- net effect depends on which one dominates.

### 4.3 Why curve inversion is associated with recession

A classic recession signal is an **inverted sovereign curve**.

Mechanically:
- front-end yields stay high because current policy is tight,
- long-end yields fall because the market expects slower growth and future cuts.

So:

$$
y_{2Y} > y_{10Y}
$$

or similar inversion measures.

Summary:

> An inverted sovereign curve usually means policy is tight today, but the market expects weaker growth and lower policy rates in the future.

---

## 5. A concrete OIS build with numbers

Below is a deliberately simple but realistic worked example for a USD SOFR OIS discount curve.

### 5.1 What the raw market data looks like in practice

In a real desk environment, you usually do **not** hard-code Bloomberg mnemonics in analytics code.

Instead, you maintain an instrument master like:

- internal instrument ID,
- vendor source,
- vendor symbol,
- currency,
- index,
- maturity / tenor,
- instrument type,
- calendar,
- day count,
- settlement lag,
- fixed-leg convention,
- floating-leg convention.

This is because Bloomberg, Reuters, brokers, or direct venue identifiers are vendor-specific and can vary by page, feed, and setup.

A concise production summary is:

> The curve builder should consume normalized quote records such as `USD-SOFR-OIS-3M` or `USD-SOFR-OIS-5Y`, and the mapping to Bloomberg or other vendor tickers should live in a market-data reference layer, not inside pricing logic.

A realistic normalized quote set could be:

- overnight fixing or short stub inputs,
- 1M SOFR OIS,
- 3M SOFR OIS,
- 6M SOFR OIS,
- 1Y SOFR OIS swap,
- 2Y SOFR OIS swap,
- 3Y SOFR OIS swap,
- 4Y SOFR OIS swap,
- 5Y SOFR OIS swap,
- then 7Y / 10Y / 30Y, etc.

Some desks also use short-rate futures to anchor the front end.

#### Example for 5.1 — market quotes used in practice

Here is a worked example:

- 1M: 2.950%
- 3M: 2.920%
- 6M: 2.850%
- 1Y par OIS swap: 2.700%
- 2Y par OIS swap: 2.450%
- 3Y par OIS swap: 2.380%
- 4Y par OIS swap: 2.420%
- 5Y par OIS swap: 2.500%

This set is intentionally shaped like a market that expects lower rates in the medium term, then a slight re-steepening later.

### 5.3 Short end: convert quote into discount factors

For a simple short end approximation:

$$
P(0,T) = \frac{1}{1 + rT}
$$

Where:
- $P(0,T)$ is the discount factor from today to maturity $T$.
- $T$ is a maturity or future time, typically measured in years from today.
- $0$ denotes the valuation date, or “today,” when it appears in term-structure notation.

So for 1M with $T = 1/12$ and $r = 2.950\%$:

$$
P(0,1M)
=
\frac{1}{1 + 0.0295 \cdot \frac{1}{12}}
=
0.997548
$$

For 3M:

$$
P(0,3M)
=
\frac{1}{1 + 0.0292 \cdot 0.25}
=
0.992753
$$

For 6M:

$$
P(0,6M)
=
\frac{1}{1 + 0.0285 \cdot 0.5}
=
0.985950
$$

### 5.4 Long end: bootstrap from par OIS swaps

For a **simple annual-pay par OIS swap**, the fixed leg pays once per year and the quoted fixed rate is the market-clearing rate that makes the swap worth zero at inception. With payment dates $t_1,\dots,t_n$, the fixed rate $K_n$ satisfies:

$$
K_n \sum_{i=1}^{n} \alpha_i P(0,t_i) = 1 - P(0,t_n)
$$

Where:
- $n$ is the number of fixed-leg payment dates in the swap.
- $\alpha_i$ is the accrual year fraction for coupon period $i$.
- $t_i$ is the payment date of coupon period $i$.
- $0$ denotes the valuation date, or “today,” when it appears in term-structure notation.

If we simplify to annual payments with $\alpha_i = 1$, then for the 1Y swap:

$$
K_1 P(0,1Y) = 1 - P(0,1Y)
$$

so:

$$
P(0,1Y) = \frac{1}{1 + K_1}
= \frac{1}{1 + 0.0270}
= 0.973710
$$

For the 2Y swap:

$$
0.0245 \left(P(0,1Y) + P(0,2Y)\right) = 1 - P(0,2Y)
$$

Therefore:

$$
P(0,2Y)
=
\frac{1 - 0.0245 \cdot P(0,1Y)}{1 + 0.0245}
=
0.952800
$$

For the 3Y swap:

$$
0.0238 \left(P(0,1Y) + P(0,2Y) + P(0,3Y)\right) = 1 - P(0,3Y)
$$

so:

$$
P(0,3Y)
=
\frac{1 - 0.0238 \cdot \left(P(0,1Y)+P(0,2Y)\right)}{1 + 0.0238}
=
0.931968
$$

And similarly onward to 4Y and 5Y.

#### Example for 5.4 — one explicit 4Y bootstrap step

Suppose the 4Y par OIS swap rate is 2.420% and the earlier discount factors are already known:

$$
P(0,1Y)=0.973710,\quad P(0,2Y)=0.952800,\quad P(0,3Y)=0.931968
$$

Then the 4Y par condition is

$$
0.0242\bigl(P(0,1Y)+P(0,2Y)+P(0,3Y)+P(0,4Y)\bigr)=1-P(0,4Y)
$$

So

$$
P(0,4Y)=
rac{1-0.0242(0.973710+0.952800+0.931968)}{1+0.0242}=0.908831
$$

This is the essence of sequential bootstrapping: all earlier solved discount factors enter the next maturity equation.

### 5.5 Resulting bootstrapped curve

| pillar   |         T | quote_type                     | market_quote_pct   |   discount_factor | zero_rate_pct_cc   |
|:---------|----------:|:-------------------------------|:-------------------|------------------:|:-------------------|
| 1M       | 0.0833333 | short OIS / money-market style | 2.950%             |          0.997548 | 2.946%             |
| 3M       | 0.25      | short OIS / money-market style | 2.920%             |          0.992753 | 2.909%             |
| 6M       | 0.5       | short OIS / money-market style | 2.850%             |          0.985950 | 2.830%             |
| 1Y       | 1         | par OIS swap                   | 2.700%             |          0.973710 | 2.664%             |
| 2Y       | 2         | par OIS swap                   | 2.450%             |          0.952800 | 2.417%             |
| 3Y       | 3         | par OIS swap                   | 2.380%             |          0.931968 | 2.349%             |
| 4Y       | 4         | par OIS swap                   | 2.420%             |          0.908831 | 2.390%             |
| 5Y       | 5         | par OIS swap                   | 2.500%             |          0.883724 | 2.472%             |

The precise numbers will differ in production because:
- the true schedule is not this simplified,
- compounding conventions matter,
- calendars and settlement lags matter,
- interpolation choices matter,
- the short end may use futures or richer short instruments,
- OIS floating legs are compounded overnight in reality.

But the **bootstrapping logic** is the same.

---

## 6. Which bootstrap technique to use and why

### 6.1 Sequential piecewise bootstrap

The standard production approach is a **piecewise bootstrap**:
1. sort instruments by maturity,
2. convert each quote into a helper / instrument representation,
3. solve the unknown node that makes that instrument price correctly,
4. move to the next maturity.

This is preferred because it is:
- market-consistent,
- interpretable,
- auditable,
- stable under normal desk operations.

#### Example for 6.1 — why sequential matters operationally

If the 3Y quote changes but the 1Y and 2Y quotes do not, then a piecewise bootstrap lets you rebuild the 3Y node and all later nodes while leaving the front end unchanged. That is useful operationally because the desk can immediately see which curve region moved and why.

### 6.2 What do you interpolate?

Common choices include:
- linear in zero rates,
- log-linear in discount factors,
- monotone cubic interpolation on zero or forward rates.

A very defendable default for production is:

> Bootstrap piecewise and interpolate log-linearly on discount factors.

Why?
- discount factors remain positive,
- implementation is simple and robust,
- diagnostics are easier,
- fewer surprises in risk and explain.

If the desk cares strongly about smooth instantaneous forwards, a monotone spline can be justified, but you must monitor for interpolation artefacts.

#### Example for 6.2 — interpolation between 2Y and 3Y pillars

Suppose:

$$
P(0,2Y)=0.952800, \qquad P(0,3Y)=0.931968
$$

A log-linear interpolation on discount factors means interpolating

$$
\ln P(0,t)
$$

linearly between 2Y and 3Y. At 2.5Y this gives approximately

$$
P(0,2.5Y) \approx \exp\left(\tfrac{1}{2}\ln 0.952800 + \tfrac{1}{2}\ln 0.931968\right) \approx 0.942326
$$

This preserves positive discount factors and usually behaves more robustly than directly interpolating quoted rates.

### 6.3 Why OIS for discounting?

Because for collateralized derivatives, the OIS curve is the appropriate modern discounting benchmark.

Summary:

> For a collateralized rates platform, the OIS curve is the natural discount curve because it is closest to the collateral remuneration / overnight funding structure, while tenor-specific forward curves should be used to project floating coupons.

#### Example for 6.3 — discounting the same coupon with the wrong curve

Suppose a 3M floating coupon of 100,000 is projected from a Euribor forward curve, but its payment is discounted with an OIS discount factor of 0.992. Then its present value is 99,200.

If you discounted the same coupon with a riskier or higher-yielding curve by mistake, the present value would be too low and both pricing and P&L explain would be distorted.

---

## 7. How to implement this concretely in a front-office system

This is where your existing platform story is strong.

A clean implementation flow is:

$$
\text{raw quotes}
\rightarrow
\text{normalized market records}
\rightarrow
\text{convention resolution}
\rightarrow
\text{curve helpers}
\rightarrow
\text{piecewise bootstrap}
\rightarrow
\text{market state}
\rightarrow
\text{valuation / risk / explain}
$$

### 7.1 Normalized quote schema

For example:

```json
{
  "curve_id": "USD_OIS_SOFR",
  "instrument_id": "USD-SOFR-OIS-5Y",
  "instrument_type": "OIS_SWAP",
  "currency": "USD",
  "index": "SOFR",
  "tenor": "5Y",
  "quote_mid": 0.0250,
  "day_count": "ACT/360",
  "fixed_leg_frequency": "ANNUAL",
  "calendar": "USNY",
  "settlement_lag_days": 2,
  "source": "BLOOMBERG"
}
```

The point is not the exact schema.
The point is to keep conventions and identifiers **explicit**.

### 7.2 Build steps

1. Load the market snapshot.
2. Resolve conventions from a registry.
3. Create typed quote helpers.
4. Bootstrap the OIS curve.
5. Store the built curve in a reusable market state.
6. Build instruments against curve IDs, not hard-coded vendor names.
7. Reuse the same market state in valuation, risk, P&L explain, and scenarios.

### 7.3 Why quote handles are useful

If you use mutable quotes / handles, then scenario analysis becomes efficient:

- shock the underlying quote,
- let the dependent curve rebuild or re-evaluate,
- reprice portfolio,
- restore original quote.

That is a strong design for:
- DV01,
- key-rate DV01,
- scenario stress,
- intraday event-driven recalculation.

### 7.4 How this maps to the platform architecture

A platform with the current architecture already has the right shape for this design:
- `market_snapshot` for normalized market inputs,
- `market_convention_registry` for conventions,
- `market_state` for reusable built objects,
- `instrument_factory` for trade construction,
- `valuation_service`, `risk_service`, `pnl_explain_service`,
- `scenario_engine`,
- persistence via SQLite.

> The market layer should be extended so the OIS builder is a first-class market-state component with explicit quote IDs, conventions, diagnostics, and scenario hooks. Valuation, DV01, key-rate risk, and P&L explain should all reuse that same market state to avoid drift between services.

---

## 8. Graphs to explain the macro logic

### 8.1 Stylized sovereign curves across macro regimes

Interpretation:

- **High inflation / hawkish policy**: the front end is high because policy is tight now.
- **Soft landing / neutral**: curve is more balanced.
- **Recession pricing / future cuts**: front and belly come down because the market expects lower future policy rates.

### 8.2 Different yield types in recession

Interpretation:

- OIS and sovereign yields usually fall when the market prices cuts and weaker growth.
- Investment-grade and especially high-yield credit can move the other way because spreads widen.
- This is why a recession can be **bullish for government duration** but **bearish for credit**.

---

## 9. The simplest mental model to remember

Think of it this way:

$$
\text{yield}
=
\text{base risk-free-ish rate}
+
\text{term premium}
+
\text{credit/liquidity spread}
+
\text{instrument-specific basis}
$$

Different instruments load on these components differently.

That is why:
- OIS,
- sovereign,
- swap,
- bond,
- CDS

must be separated in a real front-office analytics platform.

---

## 10. Implementation summary

> A yield curve is the term structure of rates across maturities, but in implementation terms it is a reusable market object that gives discount factors, zero rates, forward rates, and risk sensitivities. In a modern derivatives stack the OIS curve should be used for discounting and separate forward curves for projection. The OIS curve should be built from normalized overnight-indexed swap quotes using a piecewise bootstrap, usually with robust interpolation such as log-linear discount factors. Vendor tickers belong in the market-data layer rather than pricing code, which should consume normalized instrument IDs plus conventions. OIS should also be distinguished from sovereign and credit yields because they behave differently in recession: OIS and government yields often fall as cuts are priced, while credit spreads widen, so corporate yields can stay high or even rise. That distinction matters directly for pricing, risk, P&L explain, and scenario analysis.
