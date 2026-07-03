# Risk Factors and Attribution

This chapter is the canonical home for explicit factor-risk design.

## 1. Role Of Explicit Factors

A production risk engine should not rely on undocumented implicit bumps. It should identify exactly which market object moved, how the move was represented, and how the result should aggregate.

A factor definition should specify:

- what moved,
- by how much,
- under which convention,
- which trades and reports consume the result.

## 2. Canonical factor structure

A useful factor definition includes at least:

- factor family such as rates, credit, FX, equity, commodity, or volatility,
- market object identifier,
- currency or underlier,
- pillar or bucket label,
- shock rule,
- aggregation tags such as book, strategy, issuer, sector, or reporting group.

## 3. Factor-to-trade mapping

Let $r_{i,k}$ be the sensitivity of trade $i$ to factor $k$. Then portfolio exposure to factor $k$ is

$$
R_k^{portfolio}=\sum_i r_{i,k}
$$

Where:

- $R_k^{portfolio}$ is the aggregated exposure to factor $k$.
- $r_{i,k}$ is the trade-level exposure of trade $i$ to factor $k$.

This formula is simple only if all trade-level results were produced from the same market snapshot, the same bump rule, and the same factor definition.

## 4. Scenario attribution

Let $P\&L_i^{scenario}$ be the scenario P&L of trade $i$ under a common shocked market state. Then portfolio scenario P&L is

$$
P\&L_{portfolio}^{scenario}=\sum_i P\&L_i^{scenario}
$$

Where:

- $P\&L_{portfolio}^{scenario}$ is the scenario P&L after aggregation.

Scenario aggregation is mathematically direct but operationally demanding: every trade must be shocked and repriced under
the same scenario semantics.

## 5. Typical factor families

- rates parallel and key-rate shifts,
- credit spread factors,
- FX spot and basis factors,
- equity spot and dividend factors,
- commodity curve factors,
- volatility surface nodes.

## 6. Attribution views

Useful attribution views include:

- by trade,
- by book,
- by reporting group,
- by currency,
- by factor family,
- by tenor bucket,
- by issuer or sector for credit.

A robust design keeps the same raw factor definitions while allowing multiple aggregation views on top.

## 7. Relationship to explain and VaR

The same factor vocabulary should feed:

- sensitivities,
- P&L explain,
- historical stress,
- VaR and Expected Shortfall,
- contribution and concentration reports.

That consistency is what makes results reconcilable across reports.
