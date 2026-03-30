# CDS Curves and Credit Risk in Practice

This note consolidates the credit-specific material from the broader rates-and-risk notes.
It focuses on how CDS spreads map to survival curves, how recovery assumptions enter calibration, what outputs a
reusable credit curve should expose, and how CS01 attribution should be organized in a production platform.

## 3. CDS and credit curves

### 3.1 What a CDS spread represents

A CDS spread is the premium paid to insure against default of a reference entity.

The contract has:

- premium payments until maturity or default,
- protection payment if a credit event occurs,
- a recovery assumption.

At par spread:

$$
PV_{\text{premium}} = PV_{\text{protection}}
$$

Where:

- $PV$ is the present value of the cash flow or instrument.

### 3.2 Survival probability and default density

If hazard rate is $\lambda(t)$:

$$
Q(0,T) = \exp\left(-\int_0^T \lambda(u)\,du\right)
$$

Where:

- $Q(0,T)$ is the survival probability from today to time $T$.
- $u$ is the real argument of a characteristic function.
- $T$ is a maturity or future time, typically measured in years from today.
- $0$ denotes the valuation date, or “today,” when it appears in term-structure notation.

Default probability up to $T$:

$$
1 - Q(0,T)
$$

A simple default density intuition is:

$$
d\mathbb{P}(\tau \le t) \approx \lambda(t)Q(0,t)\,dt
$$

Where:

- $Q(0,t)$ is the survival probability from today to time $t$.
- $\lambda(t)$ is the hazard rate or default intensity at time $t$.
- $t$ is a time variable, or the real argument of a moment generating function depending on context.

#### Example for 3.2 — constant hazard rate intuition

Suppose the hazard rate is flat at

$$
\lambda = 2\%
$$

Then survival to 3 years is

$$
Q(0,3)=e^{-0.02\times 3}=e^{-0.06}=0.941765
$$

So the default probability by year 3 is

$$
1-Q(0,3)=1-0.941765=0.058235
$$

or about 5.82%.

### 3.3 Premium leg

Ignoring some details such as accrual-on-default, the premium leg is approximately:

$$
PV_{\text{premium}}
\approx
s \sum_{i=1}^{N} \alpha_i P(0,t_i) Q(0,t_i)
$$

where:

- $s$ is CDS spread,
- $\alpha_i$ is accrual factor,
- $P(0,t_i)$ is discount factor,
- $Q(0,t_i)$ is survival probability.

#### Example for 3.3 — premium leg with two annual payments

Suppose:

- CDS spread $s=150$ bp $=0.015$,
- annual accruals $\alpha_1=\alpha_2=1$,
- discount factors $P(0,1)=0.97$, $P(0,2)=0.93$,
- survival probabilities $Q(0,1)=0.99$, $Q(0,2)=0.96$.

Then

$$
PV_{\text{premium}} \approx 0.015\bigl(1\times 0.97\times 0.99 + 1\times 0.93\times 0.96\bigr)
$$

$$
=0.015(0.9603+0.8928)=0.0277965
$$

So the premium leg PV is about 2.78% of notional in this simplified setup.

### 3.4 Protection leg

Protection leg intuition:

$$
PV_{\text{protection}}
\approx
(1-R)\int_0^T P(0,u)\, d(1-Q(0,u))
$$

where $R$ is recovery.

#### Example for 3.4 — protection leg intuition

Suppose:

- recovery $R=40\%$, so $LGD=60\%$,
- default probability over the life of the CDS is about 5%,
- average discounting over the horizon is about 0.95.

A rough protection-leg estimate is

$$
PV_{\text{protection}} \approx 0.60 \times 0.05 \times 0.95 = 0.0285
$$

So the protection leg is worth about 2.85% of notional. This is close to the premium-leg number above, which is exactly
what we expect near the par spread.

### 3.5 Calibration logic

For each quoted maturity $T_i$, solve for survival / hazard structure so that quoted spread is matched:

$$
PV_{\text{premium},i}(\lambda) - PV_{\text{protection},i}(\lambda) = 0
$$

Where:

- $0$ denotes the valuation date, or “today,” when it appears in term-structure notation.

with discount curve and recovery supplied as inputs.

#### Example for 3.5 — toy calibration of a flat hazard rate

Suppose a 2Y CDS spread is quoted at 150 bp and recovery is 40%. In a simple flat-hazard approximation, a rough
relationship is

$$
s \approx (1-R)\lambda
$$

So

$$
0.015 \approx 0.60\lambda
\quad \Rightarrow \quad
\lambda \approx 0.025 = 2.5\%
$$

A real implementation uses the full premium-leg and protection-leg equations, but this back-of-the-envelope estimate
gives useful intuition.

### 3.6 CDS curve implementation checklist

A good answer:

> Typed CDS market quotes should include reference entity, seniority, currency, maturity, quote convention, and recovery
> assumption. A reusable credit curve object should then be built from those normalized quotes plus the corresponding
> discount curve. The output should expose survival probabilities, hazard rates, calibration diagnostics, and factor IDs
> so the same curve can feed pricing, CS01, stress, and P&L explain.

That is enough to sound practical and credible.

#### Example for 3.6 — what implementation metadata looks like

A normalized CDS quote record might contain:

- reference entity: ACME Corp,
- seniority: Senior Unsecured,
- currency: USD,
- maturity: 5Y,
- quote type: par spread,
- quote value: 185 bp,
- recovery assumption: 40%,
- source timestamp: 08:00:00 New York.

That metadata is what lets the platform build the correct credit curve and later aggregate CS01 by name, sector, rating,
and seniority.

---

## 4. CS01 and credit risk attribution

### 4.1 Parallel CS01

$$
CS01 \approx \frac{V(s+\Delta) - V(s-\Delta)}{2}
$$

with $\Delta = 1$ bp spread shift.

#### Example for 4.1 — parallel CS01

Suppose a CDS position is worth 2.40 today. After a +1 bp parallel spread shock it is worth 2.48, and after a -1 bp
shock it is worth 2.32. Then

$$
CS01 \approx \frac{2.48-2.32}{2}=0.08
$$

So the position changes by about 0.08 for a 1 bp parallel move in spreads.

### 4.2 Bucketed CS01

Same idea as key-rate risk, but across CDS maturities:

$$
CS01_k \approx \frac{V(\mathbf{s}+\Delta e_k) - V(\mathbf{s}-\Delta e_k)}{2}
$$

### 4.3 Why metadata matters more in credit

Rates bucket labels can often be just:

- currency,
- curve family,
- tenor.

Credit usually needs:

- reference entity,
- seniority,
- currency,
- maturity bucket,
- curve family,
- sector/rating tags for aggregation.

This is why a generic “tenor-only factor model” is not enough for credit.

#### Example for 4.3 — same tenor, different risk identity

Two positions can both be 5Y credit trades but still belong to different risk buckets:

- USD 5Y ACME Senior,
- EUR 5Y BETA Subordinated.

A tenor-only aggregation would merge them incorrectly, while a realistic credit platform keeps entity, currency, and
seniority so that risk and P&L explain remain meaningful.

---

## 5. Bond spread vs. CDS spread

You may be asked the intuition.

A simple answer:

- bond spread is a cash bond relative-value measure and includes bond-specific effects,
- CDS spread is a cleaner derivative market measure of default risk,
- in practice they are related but not identical because of liquidity, deliverability, funding, restructuring terms, and
  technical factors.

That level of detail is usually sufficient unless a deeper calibration discussion is required.

#### Example for 5 — bond spread versus CDS spread

Suppose a 5Y corporate bond yields 5.40% while the matched sovereign benchmark yields 3.90%. The bond spread is about
150 bp. If the same issuer’s 5Y CDS trades at 165 bp, the two numbers are close but not identical.

The gap can come from bond liquidity, bond-specific technicals, delivery options, or funding effects.

---
