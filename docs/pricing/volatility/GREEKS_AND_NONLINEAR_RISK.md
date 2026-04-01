# Greeks, Non-Linear Risk, and Scenario Revaluation

## Why this chapter matters

Option-pricing systems are judged not only by price accuracy, but also by the stability and interpretability of the risk
numbers they produce. Greeks are the bridge between pricing and risk management. They support hedging, explain, scenario
approximation, and risk aggregation, but they are only local approximations. This chapter explains the main Greeks, why
they matter operationally, and when a platform must switch from sensitivity approximation to full scenario revaluation.

## 1. Linear products versus optional products

A forward has linear payoff. For a long forward struck at $K$ on underlying $S_T$,
$$
\Pi_{forward}(T)=S_T-K
$$
A call option has non-linear payoff
$$
\Pi_{call}(T)=\max(S_T-K,0)
$$
and a put option has
$$
\Pi_{put}(T)=\max(K-S_T,0)
$$
The non-linearity is what creates convexity, volatility exposure, and the need for Greeks beyond simple delta.

## 2. Core Greeks

Let $V(S,\sigma,r,t,\ldots)$ be the model value of the instrument.

### 2.1 Delta

Delta is sensitivity to the underlying level:
$$
\Delta = \frac{\partial V}{\partial S}
$$
For a small underlying move $\Delta S$,
$$
\Delta V \approx \Delta\,\Delta S
$$
Delta is the first risk number a trader usually wants because it supports immediate hedge decisions.

### 2.2 Gamma

Gamma is curvature with respect to the underlying level:
$$
\Gamma = \frac{\partial^2 V}{\partial S^2}
$$
If gamma is large, delta itself changes quickly when the market moves. This is why an option hedge that looks fine for a
small move can become poor after a larger move.

### 2.3 Vega

Vega is sensitivity to implied volatility:
$$
Vega = \frac{\partial V}{\partial \sigma}
$$
A portfolio with small delta can still have substantial volatility exposure if it contains options.

### 2.4 Theta

Theta is sensitivity to the passage of time:
$$
\Theta = \frac{\partial V}{\partial t}
$$
Theta is often reported with a sign convention tied to one day of calendar or business time. Production documentation
must state the convention explicitly.

### 2.5 Rho

Rho is sensitivity to interest rates:
$$
\rho = \frac{\partial V}{\partial r}
$$
For rates products or long-dated options, rate sensitivity can be material even when the desk informally focuses on
delta and vega.

## 3. Greeks as local approximations

A second-order Taylor approximation for one underlying, one volatility input, and one rate input is
$$
\Delta V \approx \Delta\,\Delta S + \frac{1}{2}\Gamma(\Delta S)^2 + Vega\,\Delta\sigma + \rho\,\Delta r + \Theta\,\Delta t
$$
This expression is useful because it decomposes P&L into interpretable components, but it is only a local approximation.
Its accuracy deteriorates when:

- moves are large,
- volatility and correlation shift materially,
- the payoff is path-dependent,
- barriers or discontinuities are near,
- credit products face jump-to-default risk,
- the surface dynamics differ from the bump rule used in the approximation.

## 4. Why Greeks matter operationally

A front-office platform should expose Greeks because they support four distinct operational tasks.

### 4.1 Hedging

Desk hedges are usually initiated from first-order and second-order sensitivities. A trader can hedge spot or rate risk
quickly from delta and DV01, and can hedge convexity or volatility risk using gamma and vega information.

### 4.2 P&L explain

Greeks provide a first decomposition of market-move P&L. For example,
$$
P\&L_{market} \approx \Delta\text{-}P\&L + \Gamma\text{-}P\&L + Vega\text{-}P\&L + \rho\text{-}P\&L
$$
Even when the final explain uses full revaluation, Greek buckets remain useful diagnostics.

### 4.3 Limits and aggregation

Risk systems usually aggregate sensitivities by book, desk, portfolio, currency, tenor, or surface bucket. A single
trade-level number is not enough. The platform must support:

- trade-level Greeks,
- position-level netting,
- portfolio aggregation,
- bucketed reporting for hedging and limits.

### 4.4 Fast scenario views

For small or medium shocks, a Greek approximation can provide rapid scenario answers without full revaluation. This is
useful intraday when a desk wants an immediate answer while a more expensive exact revaluation is still running.

## 5. Portfolio aggregation and non-linear exposures

Portfolio Greeks are not just sums of isolated trade measures in an economic sense, even though the arithmetic
aggregation is simple after measures are produced consistently.

### 5.1 Netting and concentration

A book can have large gross deltas that mostly offset, leaving a small net delta but large gamma or vega. This is why a
risk report should show both net and gross views where relevant.

### 5.2 Surface-bucket aggregation

Vega should not be reported as a single global number only. It should also be mapped to stable surface buckets, for
example by tenor and strike or tenor and delta. A portfolio can be approximately vega-neutral overall while being highly
concentrated in a particular expiry bucket.

### 5.3 Cross-gamma and cross-risk

In multi-factor products, a fuller approximation may involve cross terms such as
$$
\frac{\partial^2 V}{\partial S\,\partial \sigma}\Delta S\,\Delta\sigma
$$
or cross-gamma across correlated underlyings. Whether the platform reports these explicitly depends on the desk, but the
engine should be designed so that higher-order or cross-risk terms can be added without changing the market-state model.

## 6. Scenario revaluation versus Greeks-based approximation

### 6.1 Full revaluation

Under a shocked market state $M^{shock}$,
$$
\Delta V = V(M^{shock}) - V(M^{base})
$$
This is conceptually the cleanest approach because the pricing model is re-run under the shocked inputs.

### 6.2 Trade-off

- Greeks-based approximation is fast and interpretable.
- Full revaluation is slower but more accurate for non-linear portfolios.

A production platform should support both. The user should be able to see when a report is a local sensitivity
approximation and when it is a true revaluation result.

### 6.3 When exact revaluation is preferred

Exact scenario revaluation is preferred when:

- the shock is large,
- the portfolio contains options with high convexity,
- there are barrier or callable features,
- volatility surfaces are shocked,
- credit jump risk is material,
- hedging or limit decisions depend on the accuracy of the scenario.

## 7. Numerical Greeks and implementation choices

When closed-form Greeks are not available, numerical differentiation is used.

A central finite-difference approximation for delta is
$$
\Delta \approx \frac{V(S+h)-V(S-h)}{2h}
$$
A central approximation for gamma is
$$
\Gamma \approx \frac{V(S+h)-2V(S)+V(S-h)}{h^2}
$$
These formulas are straightforward, but implementation quality matters:

- bump sizes should be consistent across products,
- the shocked market state must be built with correct sticky rules,
- revaluation should reuse cached instruments and market objects where possible,
- numerical noise should be monitored.

## 8. Sticky rules and risk interpretation

A volatility Greek depends on the bump convention used for the surface. For example, a spot bump can be combined with:

- sticky strike,
- sticky delta,
- sticky moneyness,
- sticky local-vol parameterization.

These rules can produce materially different scenario P&L for the same initial trade. Documentation and reports must
state the convention clearly, otherwise front office and risk may appear to disagree while merely using different bump
rules.

## 9. Relationship to the rest of the documentation

- `docs/pricing/volatility/OPTION_PRICING_FOUNDATIONS_AND_PUT_CALL_PARITY.md` covers payoff identities and static
  no-arbitrage relationships.
- `docs/pricing/volatility/VOLATILITY_SURFACES.md` covers surface construction, smile interpolation, and no-arbitrage
  checks.
- `docs/risk/RISK_MEASURES_AND_EXPLAIN.md` and `docs/risk/PNL_EXPLAIN_IN_PRACTICE.md` explain how Greeks feed into desk
  risk reports and P&L explain.
