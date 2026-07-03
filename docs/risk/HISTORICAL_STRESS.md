# Historical Stress Testing

Historical stress testing evaluates today's portfolio under market moves observed during a past episode.

## Notation used in this chapter

Unless stated otherwise:

- $F_0$ is today's factor vector.
- $\Delta F^{(h)}$ is the historical factor move for episode $h$.
- $F_0^{stress,h}$ is the stressed factor vector built from today's factors plus the historical move.
- $V(F)$ is portfolio value as a function of factor state $F$.
- $\Pi^{(h)}$ is stressed P&L for episode $h$.

## 1. Core definition

Let today's factor vector be $F_0$. Let a historical episode $h$ provide an observed move $\Delta F^{(h)}$. Then the stressed factor state is

$$
F_0^{stress,h}=F_0+\Delta F^{(h)}
$$

Where:

- $F_0^{stress,h}$ is the factor state used for revaluation under episode $h$.

Let the portfolio value under today's market be $V(F_0)$. Then historical-stress P&L is

$$
\Pi^{(h)}=V(F_0^{stress,h})-V(F_0)
$$

Where:

- $\Pi^{(h)}$ is the stressed P&L for historical episode $h$.

## 2. Analytical Use

Historical stress is useful because it preserves real cross-factor co-movements that occurred during an actual market regime. It naturally captures:

- large correlated cross-asset moves,
- curve twists and basis dislocations,
- spread widening and liquidity deterioration,
- non-linear portfolio effects under coherent market moves.

## 3. Practical workflow

A robust workflow is:

1. define today's portfolio and today's market snapshot,
2. choose a historical date or multi-day episode,
3. compute historical factor moves,
4. map those moves onto today's factor grid,
5. build the stressed market state,
6. reprice the portfolio,
7. aggregate and explain the stressed P&L.

## 4. Additive versus relative shocks

For rates or spreads, additive shocks are common. Let $r_0(T)$ be today's rate at maturity $T$ and let $\Delta r^{(h)}(T)$ be the historical change. Then

$$
r^{stress}(T)=r_0(T)+\Delta r^{(h)}(T)
$$

For spots or indices, relative shocks are often more natural. Let $S_0$ be today's spot and let $m^{(h)}$ be a historical return multiplier. Then

$$
S^{stress}=S_0\times m^{(h)}
$$

The documentation should state the shock type explicitly because additive and multiplicative mappings can produce materially different results.

## 5. Implementation requirements

A production historical-stress engine should archive:

- the portfolio snapshot,
- the base market state,
- the historical episode definition,
- the factor-mapping rules,
- the repricing methodology,
- contribution and drill-down outputs.

Without that lineage the result cannot be reproduced or explained.
