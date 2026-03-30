# Monte Carlo Implementation, GBM, Books, Portfolios, and Risk in Practice

## Notation used in this chapter

Unless stated otherwise:

- $S_t$ is the asset price at time $t$.
- $S_0$ is the spot price today.
- $\mu$ is drift under the chosen measure.
- $r$ is the risk-free rate when risk-neutral pricing is used.
- $\sigma$ is volatility.
- $W_t$ is Brownian motion.
- $dt$ is an infinitesimal time increment and $\Delta t$ is a finite time step.
- $Z$ is a standard normal random variable.
- $V$ is a trade or portfolio value.
- $\Delta V$ is the P&L or change in value.
- $DV01$ is the dollar value change for a 1 basis point rate move.
- $VaR$ is value at risk and $ES$ is expected shortfall.

## 1. What is a book and what is a portfolio?

A **position** is one trade or one holding.

Examples:

- long 10,000 shares of an equity index ETF
- short 50 EUR/USD forwards
- receive-fixed 5Y EUR swap
- long 200 payer swaptions

A **portfolio** is a collection of positions valued together.

A **book** is usually an operational or organizational subdivision of positions inside a larger portfolio. In practice,
a book is often tied to one of these dimensions:

- trader or portfolio manager
- strategy
- desk
- legal entity
- accounting bucket
- asset class
- reporting perimeter

So the hierarchy is often:

$$
\text{firm} \rightarrow \text{desk} \rightarrow \text{book} \rightarrow \text{portfolio} \rightarrow \text{positions}
$$

or, in some firms, portfolio sits above book. The naming varies, but the core idea is the same: a book is a manageable
subset of exposures used for pricing, P\&L, limits, and risk reporting.

> A book is the set of trades a desk wants to risk-manage and report together. A portfolio is the economic collection of
> positions whose value and risk we aggregate under a common market state.

## 2. How risk is calculated in practice

In production, risk is not one formula. It is a pipeline.

The usual chain is:

$$
\text{positions} \rightarrow \text{market data} \rightarrow \text{risk factors} \rightarrow \text{models} \rightarrow \text{revaluation} \rightarrow \text{aggregation} \rightarrow \text{reports and limits}
$$

Typical steps:

1. Load positions and normalize trade economics.
2. Build the market state: spots, curves, vol surfaces, fixings, spreads, FX, correlations.
3. Map positions to pricing models.
4. Compute base valuation.
5. Compute sensitivities and scenario revaluations.
6. Aggregate by book, desk, strategy, legal entity, and portfolio.
7. Store results for P\&L explain, risk limits, stress, and audit.

Common risk outputs:

- PV / NPV
- delta, gamma, vega, rho
- DV01 / PV01 / key-rate DV01
- CS01
- VaR / expected shortfall
- stress and scenario losses
- concentration metrics
- P\&L explain

For liquid books, a lot of daily risk is sensitivity-based. For nonlinear books, scenario revaluation and Monte Carlo
become much more important.

## 2A. Practical fast-risk approximation: modified duration

Before doing a full revaluation, front office and risk often want a quick first-order estimate of a rates move.

For a bond with price $P$, modified duration $D_{\text{mod}}$, and small yield change $\Delta y$:

$$
\Delta P \approx -D_{\text{mod}} \, P \, \Delta y
$$

#### Example for 2 — GBM path generation for one risk factor

Suppose:

- bond price $P = 97.28$
- modified duration $D_{\text{mod}} = 4.35$
- parallel yield shock $\Delta y = +25\text{bp} = 0.0025$

Then:

$$
\Delta P \approx -4.35 \times 97.28 \times 0.0025 \approx -1.06
$$

Where:

- $0$ denotes the valuation date, or “today,” when it appears in term-structure notation.

Interpretation:

- the bond would lose about 1.06 price points for a +25bp parallel shock.

What front office realistically looks at:

- quick duration-based estimate immediately after a data release,
- then full curve revaluation,
- then bucketed DV01 / key-rate DV01,
- then P\&L explain versus actual move.

What independent risk realistically looks at:

- whether the fast approximation and full revaluation are directionally consistent,
- concentration by key tenor,
- nonlinear residual from convexity,
- whether stress losses are still acceptable after the move.

Good practice:

- use modified duration only as a fast linear approximation,
- use full revaluation for large curve moves or nonlinear trades,
- keep approximation methods explicitly labeled in outputs.

## 3. What Monte Carlo is computing

Most pricing and risk problems can be written as an expectation:

$$
V_0 = e^{-rT} \mathbb{E}[\text{payoff}(X_T)]
$$

Where:

- $\mathbb{E}[\cdot]$ denotes expectation.

or more generally under the risk-neutral measure:

$$
V_0 = \mathbb{E}^{\mathbb{Q}}\left[D(0,T)\,\Pi_T\right]
$$

where:

- $D(0,T)$ is the discount factor
- $\Pi_T$ is the future payoff depending on simulated state variables

Monte Carlo approximates this expectation by a sample average:

$$
\hat V_N = \frac{1}{N} \sum_{i=1}^N D^{(i)}(0,T)\,\Pi_T^{(i)}
$$

Where:

- $N$ is the number of simulated paths or observations.
- $T$ is a maturity or future time, typically measured in years from today.
- $0$ denotes the valuation date, or “today,” when it appears in term-structure notation.

This is why the Law of Large Numbers and CLT matter.

## 4. How to implement a Monte Carlo pricer

### High-level algorithm

1. Define the model and market data.
2. Simulate risk-factor paths or direct terminal states.
3. Revalue each trade or payoff on each path.
4. Discount cash flows.
5. Average over paths.
6. Estimate standard error and confidence interval.

### Minimal implementation design

A good production decomposition is:

- `RandomEngine`: pseudorandom or low-discrepancy numbers
- `PathGenerator`: turns random numbers into factor paths
- `MarketStateEvolver`: GBM, local vol, Heston, LMM, Hull-White, copula, etc.
- `PayoffEvaluator` or `TradePricer`: values one trade on one path
- `Aggregator`: sums over trades and paths
- `StatsAccumulator`: mean, variance, confidence interval

### Pseudocode

```text
build market state
initialize rng with deterministic seed and stream partition
for each worker block:
    local_sum = 0
    local_sum_sq = 0
    for path in assigned_block:
        simulate factors
        portfolio_value = 0
        for trade in portfolio:
            portfolio_value += price_trade_on_path(trade, path)
        local_sum += portfolio_value
        local_sum_sq += portfolio_value * portfolio_value
reduce local sums across workers
estimate mean, variance, stderr, confidence interval
```

### Online statistics

Do not store all path values if you do not need them. Use online accumulation, for example Welford's algorithm, to
update mean and variance in one pass. This saves memory and improves cache behavior.

## 5. Geometric Brownian Motion (GBM)

GBM is the standard diffusion behind Black-Scholes-like models for equities and many simple teaching examples.

The SDE is:

$$
dS_t = \mu S_t dt + \sigma S_t dW_t
$$

where:

- $S_t$ is the asset price
- $\mu$ is the drift under the chosen measure
- $\sigma$ is volatility
- $W_t$ is Brownian motion

Under the risk-neutral measure for an equity with continuous dividend yield $q$:

$$
dS_t = (r-q) S_t dt + \sigma S_t dW_t^{\mathbb Q}
$$

### Exact solution

GBM has a closed-form solution:

$$
S_T = S_0 \exp\left(\left(\mu - \frac{1}{2}\sigma^2\right)T + \sigma \sqrt{T} Z\right)
$$

Where:

- $S_T$ is the asset price at maturity $T$.
- $S_0$ is the asset price at the valuation date.
- $\sigma^2$ is the variance of the underlying random variable or process.
- $T$ is a maturity or future time, typically measured in years from today.

with $Z \sim \mathcal N(0,1)$.

Under risk-neutral pricing:

$$
S_T = S_0 \exp\left(\left(r-q- \frac{1}{2}\sigma^2\right)T + \sigma \sqrt{T} Z\right)
$$

For GBM, the **exact lognormal step** is usually preferred over Euler because it is more accurate and cheaper.

### Time-stepped version

For a grid $0=t_0<t_1<\dots<t_n=T$ and $\Delta t=t_{k+1}-t_k$:

$$
S_{t_{k+1}} = S_{t_k} \exp\left(\left(\mu - \frac{1}{2}\sigma^2\right)\Delta t + \sigma \sqrt{\Delta t}\, Z_k\right)
$$

Where:

- $\Delta t$ is the time step used in a discretization or simulation scheme.
- $\sigma^2$ is the variance of the underlying random variable or process.
- $t$ is a time variable, or the real argument of a moment generating function depending on context.

with independent $Z_k \sim \mathcal N(0,1)$.

## 6. Worked GBM example with a realistic book

Take a simple equity-options book on one stock index. This is a realistic teaching example for Monte Carlo, even though
rates/credit books would use richer factor models.

Assume:

- spot $S_0 = 100$
- risk-free rate $r = 3\%$
- dividend yield $q = 1\%$
- volatility $\sigma = 20\%$
- horizon $T = 1$ year

Book:

- long 1,000 shares
- long 100 ATM European calls with strike $K=100$
- short 50 OTM puts with strike $K=90$

### One simulated path

Suppose one normal draw is $Z=0.50$.

Then:

$$
S_T = 100\exp\left((0.03-0.01-0.5\cdot0.2^2)\cdot1 + 0.2\cdot 0.50\right)
$$

Where:

- $S_T$ is the asset price at maturity $T$.
- $0$ denotes the valuation date, or “today,” when it appears in term-structure notation.

$$
S_T = 100\exp(0.02-0.02+0.10)=100e^{0.10}\approx 110.52
$$

Now evaluate terminal payoffs.

Shares:

$$
\Pi_T^{\text{shares}} = 1000\cdot S_T = 110{,}520
$$

Call payoff per option:

$$
\max(S_T-K,0)=\max(110.52-100,0)=10.52
$$

For 100 calls:

$$
\Pi_T^{\text{calls}} = 100\cdot 10.52 = 1{,}052
$$

Put payoff per option:

$$
\max(90-S_T,0)=0
$$

For short 50 puts:

$$
\Pi_T^{\text{puts}} = -50\cdot 0 = 0
$$

Total terminal value on this path:

$$
\Pi_T = 110{,}520 + 1{,}052 = 111{,}572
$$

Discounted value on this path:

$$
V^{(i)} = e^{-0.03}\cdot 111{,}572 \approx 108{,}276
$$

You repeat this for many paths and average.

### Book performance over a horizon

If you want horizon P\&L instead of fair value, you simulate the market state at horizon $h$, revalue the full book
under that horizon state, and compare to today's value adjusted for carry/cash flows:

$$
\Delta V^{(i)} = V_h^{(i)} - V_0 + \text{cash flows / carry adjustments}
$$

Where:

- $\Delta V$ is the change in portfolio or instrument value.

Then you build the distribution of $\Delta V$ across scenarios.

This is how Monte Carlo VaR / ES is conceptually built.

## 7. Pricing vs. risk Monte Carlo

### Pricing Monte Carlo

Goal: estimate fair value under the risk-neutral measure.

$$
V_0 = \mathbb{E}^{\mathbb{Q}}[D(0,T)\Pi_T]
$$

Where:

- $\mathbb{E}[\cdot]$ denotes expectation.
- $T$ is a maturity or future time, typically measured in years from today.
- $0$ denotes the valuation date, or “today,” when it appears in term-structure notation.

### Risk Monte Carlo

Goal: estimate future portfolio P\&L distribution under a real-world or stressed scenario measure.

$$
\Delta V_h = V_h(X_h) - V_0
$$

Where:

- $\Delta V$ is the change in portfolio or instrument value.

Pricing MC and risk MC often share infrastructure but differ in:

- measure
- drift assumptions
- scenario generation
- horizon and revaluation methodology

## 8. How risk is calculated for a book in practice

For a nonlinear book, a common process is:

1. Load book positions.
2. Build current market state.
3. Generate scenarios for risk factors at horizon $h$.
4. Rebuild the market state under each scenario.
5. Reprice all positions.
6. Aggregate book P\&L.
7. Compute VaR, expected shortfall, stress losses, and factor contributions.

#### Example for 8 — book-level risk metrics from simulated P&L

#### Monte Carlo VaR

From simulated P\&L samples $\Delta V^{(1)},\dots,\Delta V^{(N)}$, the 99% VaR is approximately the 1st percentile loss:

$$
\text{VaR}_{99\%} \approx -Q_{1\%}(\Delta V)
$$

Where:

- $\Delta V$ is the change in portfolio or instrument value.
- $VaR$ denotes Value at Risk.

#### Expected shortfall

$$
\text{ES}_{99\%} = -\mathbb{E}[\Delta V \mid \Delta V \le Q_{1\%}(\Delta V)]
$$

Where:

- $\mathbb{E}[\cdot]$ denotes expectation.

#### Sensitivity risk

For liquid books, you often also compute:

$$
\Delta \approx \frac{V(S+\varepsilon)-V(S-\varepsilon)}{2\varepsilon}
$$

and similar bump-based measures for curves, spreads, and vol surfaces.

## 9. How to parallelize Monte Carlo

Monte Carlo is naturally parallel because paths are mostly independent.

### Level 1: path parallelism

The most common pattern is to split paths into blocks and assign each block to one worker thread or process.

$$
\hat V_N = \frac{1}{N}\sum_{j=1}^{M}\sum_{i \in \mathcal B_j} V^{(i)}
$$

Where:

- $M$ is imports in the GDP identity.
- $N$ is the number of simulated paths or observations.

Each worker computes:

- local sum
- local sum of squares
- possibly local Greeks or scenario stats

Then a reduction merges partial results.

This is the default and most scalable parallelization mode.

### Level 2: trade parallelism

If the book is large and path generation is shared, you can also parallelize over trades or trade groups, especially
when trade valuation is expensive.

This is useful when:

- the book is huge
- each trade has large pathwise work
- you need trade-level explain and allocation

But trade-level parallelism can be memory-heavier if each worker needs access to the full path state.

### Level 3: scenario parallelism

For horizon risk or XVA-like workloads, you may parallelize over scenarios, dates, or outer simulations.

### Level 4: GPU parallelism

GPUs are attractive when:

- payoffs are homogeneous
- path logic is regular
- memory transfers are controlled
- branching is limited

GBM terminal-state pricing for vanilla options is a classic GPU-friendly workload.

More irregular books with exercise logic, path-dependent features, and large object graphs are harder to accelerate
efficiently.

## 10. How to optimize Monte Carlo

### A. Use exact sampling when available

For GBM, use the exact lognormal step, not Euler. It is both faster and more accurate.

### B. Avoid storing full paths unless needed

For European payoffs, simulate terminal states directly:

$$
S_T = S_0 \exp\left((r-q-\frac{1}{2}\sigma^2)T+\sigma\sqrt{T}Z\right)
$$

Where:

- $S_T$ is the asset price at maturity $T$.
- $S_0$ is the asset price at the valuation date.
- $\sigma^2$ is the variance of the underlying random variable or process.
- $\sigma$ is the volatility or standard deviation parameter.
- $T$ is a maturity or future time, typically measured in years from today.

No need to store intermediate steps.

### C. Chunk the work

Process paths in blocks sized to fit CPU cache and reduce synchronization overhead.

### D. Use thread-local accumulators

Avoid lock contention. Each worker should keep its own sums and combine them only at the end.

### E. Use vectorization / SIMD

Generate random numbers and update states in arrays to benefit from SIMD and cache locality.

### F. Use structure-of-arrays layout when appropriate

For large factor sets, `SoA` often vectorizes better than `AoS`.

### G. Use good RNG stream partitioning

Each worker must have independent, reproducible streams.

Typical approaches:

- skip-ahead / leapfrog
- block-splitting
- counter-based RNGs

This is very important in audited production environments.

### H. Use variance reduction

Variance reduction often gives more benefit than raw hardware scaling.

Common methods:

#### Antithetic variates

Use $Z$ and $-Z$ together.

#### Control variates

If you know a correlated quantity with known expectation, adjust the estimator.

For example, under GBM you can use an analytically priced vanilla as a control.

#### Stratification / Latin hypercube

Improve coverage of the sample space.

#### Quasi-Monte Carlo

Use low-discrepancy sequences such as Sobol to reduce effective error for smooth problems.

### I. Reduce pricing overhead per trade

For a book with many trades, the real bottleneck is often trade revaluation, not random numbers.

So optimize:

- precomputed invariant quantities
- curve lookups
- discount factors
- volatility surface interpolation
- cashflow schedules
- payoff branching

### J. Prefer pathwise Greeks or likelihood-ratio Greeks when possible

Finite-difference Greeks can multiply the runtime by the number of bumps.

Pathwise and LR estimators can be much cheaper if the payoff and model allow them.

## 11. Practical production trade-offs

### Reproducibility vs. speed

A front-office or risk engine usually needs deterministic reproducibility. That means:

- explicit seed control
- explicit stream partitioning
- stable reduction logic
- auditable configuration

### Throughput vs. latency

Intraday desk analytics may prefer lower-latency approximations or proxy models. End-of-day risk may run heavier Monte
Carlo with more paths and richer scenarios.

### Full revaluation vs. proxy risk

For large books, a common production compromise is:

- full Monte Carlo on the most nonlinear / material trades
- sensitivity-based approximations on simpler positions
- aggregation in one reporting layer

A strong concise answer is:

> A robust Monte Carlo implementation uses embarrassingly parallel path blocks with thread-local random streams and
> thread-local statistics, then reduces partial sums at the end. It avoids storing full paths unless the payoff is path
> dependent, uses exact simulation when available, and focuses as much on variance reduction and pricing-cost
> optimization as on raw parallelism. In practice, the real bottleneck in large books is often pathwise revaluation of
> trades, so caching, vectorization, memory layout, and scenario reuse matter a lot.

> In production, risk for a book means building a consistent market state, revaluing all positions under shocks or
> simulated scenarios, and aggregating the resulting P\&L distribution and sensitivities. A book is the operational set
> of positions managed and reported together, while a portfolio is the economic collection of positions whose value and
> risk we aggregate. For nonlinear books, Monte Carlo is used to generate horizon states and full-revaluation P\&L; for
> linear books, sensitivities and stress often dominate.

## 14. Important caveat for rates and credit platforms

GBM is a good teaching model, but for a rates / credit front-office platform you would usually need richer models:

- multiple curves
- spread dynamics
- stochastic vol or local vol where relevant
- correlation structures
- possibly jump or regime effects
- scenario-based risk models rather than pure GBM

That said, the engineering principles are the same:

- clean factor representation
- fast scenario generation
- deterministic reproducibility
- scalable revaluation
- explainable aggregation
