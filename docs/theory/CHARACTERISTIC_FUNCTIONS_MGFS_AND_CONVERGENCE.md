# Characteristic Functions, Moment Generating Functions, and Modes of Convergence

## Notation used in this chapter

Unless stated otherwise:

- $X$ is a random variable.
- $X_1,\dots,X_n$ are sample observations.
- $\mu = \mathbb{E}[X]$ is the population mean.
- $\sigma^2 = \mathrm{Var}(X)$ is the population variance.
- $\hat{\theta}$ is an estimator of an unknown quantity $\theta$.
- $M_X(t)$ is the moment generating function.
- $\phi_X(u)$ is the characteristic function.
- $t$ is a real argument for an MGF and $u$ is a real argument for a characteristic function.
- $i$ is the imaginary unit with $i^2=-1$.

## 1. Characteristic function vs. moment generating function

For a real-valued random variable $X$:

$$
\phi_X(u) = \mathbb{E}[e^{iuX}], \qquad u \in \mathbb{R}
$$

Where:
- $\mathbb{R}$ denotes the real numbers.
- $\mathbb{E}[\cdot]$ denotes expectation.
- $\phi_X(u)$ is the characteristic function of the random variable $X$ evaluated at $u$.
- $u$ is the real argument of a characteristic function.

is the **characteristic function** and

$$
M_X(t) = \mathbb{E}[e^{tX}], \qquad t \in \mathbb{R}
$$

Where:
- $M_X(t)$ is the moment generating function of the random variable $X$ evaluated at $t$.
- $t$ is a time variable, or the real argument of a moment generating function depending on context.

is the **moment generating function (MGF)**.

A useful relation is:

$$
\phi_X(u) = M_X(iu)
$$

whenever the MGF exists in a neighborhood that allows evaluation on the imaginary axis.

#### Example for 1 — characteristic function of a Bernoulli variable

If $X=c=3$ is constant, then:

$$
\phi_X(u)=\mathbb{E}[e^{iuX}] = e^{3iu}
$$

and

$$
M_X(t)=\mathbb{E}[e^{tX}] = e^{3t}
$$

This is a good sanity check because the expectation disappears when the random variable is deterministic.

### 1.1 Why the characteristic function always exists

The critical point is:

$$
|e^{iuX}| = 1
$$

for every real $u$ and real $X$, because

$$
e^{iuX} = \cos(uX) + i\sin(uX).
$$

Hence:

$$
\mathbb{E}[|e^{iuX}|] = \mathbb{E}[1] = 1 < \infty.
$$

So the expectation defining $\phi_X(u)$ is always finite.

#### Example for 1.1 — why the characteristic function stays bounded

Take $X=3$ and $u=2$.
Then:

$$
e^{iuX}=e^{i\cdot 2 \cdot 3}=e^{6i}=\cos 6 + i\sin 6
$$

Its modulus is:

$$
|e^{6i}| = 1
$$

That is the whole reason the characteristic function always exists: the integrand never explodes in magnitude.

### 1.2 Why the MGF may fail to exist

For the MGF,

$$
|e^{tX}| = e^{tX},
$$

which can grow explosively in the tails. If $X$ has sufficiently heavy tails, then:

$$
\mathbb{E}[e^{tX}] = \infty
$$

for some or all nonzero $t$.

This is the main distinction:
- characteristic function: bounded oscillatory integrand
- MGF: exponentially growing integrand

#### Example for 1.2 — MGF divergence under a heavy-tailed law

Take $X=3$ and $t=2$.
Then:

$$
e^{tX}=e^{2\cdot 3}=e^6 \approx 403.43
$$

So unlike the characteristic-function integrand, the MGF integrand can become very large very quickly. For heavy-tailed distributions that is exactly why the expectation may diverge.

### 1.3 Practical difference

The MGF is often easier for moment manipulations when it exists:

$$
M_X^{(n)}(0) = \mathbb{E}[X^n].
$$

Where:
- $X$ is a generic random variable.
- $n$ is the sample size.
- $0$ denotes the valuation date, or “today,” when it appears in term-structure notation.

For the characteristic function:

$$
\phi_X^{(n)}(0) = i^n \mathbb{E}[X^n]
$$

when the moments exist.

The characteristic function is more robust and more useful in heavy-tailed settings, Fourier methods, and option pricing.

### 1.4 Key properties of the characteristic function

- Uniquely determines the distribution.
- Always exists.
- If $X$ and $Y$ are independent, then:

$$
\phi_{X+Y}(u) = \phi_X(u)\phi_Y(u).
$$

Where:
- $X$ is a generic random variable.

- If $X \sim \mathcal{N}(\mu,\sigma^2)$, then:

$$
\phi_X(u) = \exp\left(iu\mu - \frac{1}{2}\sigma^2 u^2\right).
$$

Where:
- $\mu$ is the true mean, drift, or expected value depending on context.
- $\sigma^2$ is the variance of the underlying random variable or process.
- $\sigma$ is the volatility or standard deviation parameter.

> The characteristic function always exists because $e^{iuX}$ has modulus 1, so its expectation is always finite. The MGF uses $e^{tX}$, which can blow up exponentially in the tails, so it may not exist. The MGF is convenient for moments when it exists, while the characteristic function is more robust and is the natural object for Fourier-based methods.

## 2. Law of Large Numbers: weak vs. strong

Let $X_1, X_2, \dots$ be i.i.d. random variables with finite mean:

$$
\mathbb{E}[X_i] = \mu.
$$

Where:
- $X_i$ is the $i$-th sampled observation or simulated payoff.
- $\mu$ is the true mean, drift, or expected value depending on context.
- $\mathbb{E}[\cdot]$ denotes expectation.

Define the sample average:

$$
\bar{X}_n = \frac{1}{n}\sum_{k=1}^n X_k.
$$

Where:
- $X$ is a generic random variable.
- $n$ is the sample size.

The Law of Large Numbers says that $\bar{X}_n$ approaches $\mu$ as $n$ grows.

### 2.1 Weak Law of Large Numbers (WLLN)

The weak law says:

$$
\bar{X}_n \xrightarrow{P} \mu.
$$

This means **convergence in probability**. For every $\varepsilon > 0$:

$$
\mathbb{P}(|\bar{X}_n - \mu| > \varepsilon) \to 0.
$$

Where:
- $0$ denotes the valuation date, or “today,” when it appears in term-structure notation.

Interpretation:
- for large $n$, the sample mean is very likely to be close to the true mean
- but this does not say what happens on every sample path

### 2.2 Strong Law of Large Numbers (SLLN)

The strong law says:

$$
\bar{X}_n \xrightarrow{a.s.} \mu.
$$

This means **almost sure convergence**. In words:
- with probability 1, the entire sequence of sample means converges to $\mu$
- equivalently, for almost every outcome, eventually the sample average settles toward the true mean

Almost sure convergence is stronger than convergence in probability.

### 2.3 Relationship between them

Strong convergence implies weak convergence:

$$
\bar{X}_n \xrightarrow{a.s.} \mu \quad \Longrightarrow \quad \bar{X}_n \xrightarrow{P} \mu.
$$

But the converse is not true in general.

So your intuition is correct: in some settings the strong law may fail or may require stronger assumptions, while the weak law can still hold under weaker ones.

### 2.4 Why do we keep the weak law if the strong law is stronger?

Because they answer different questions and require different assumptions.

The weak law is still useful because:
- it is often easier to prove
- it can hold under weaker conditions
- for many statistical applications, convergence in probability is enough
- many asymptotic estimation results only need convergence in probability

A central example is Chebyshev's inequality. If the variables are i.i.d. with finite variance $\sigma^2$, then:

$$
\mathrm{Var}(\bar{X}_n) = \frac{\sigma^2}{n}
$$

Where:
- $\sigma^2$ is the variance of the underlying random variable or process.
- $\sigma$ is the volatility or standard deviation parameter.
- $\mathrm{Var}(\cdot)$ denotes variance.

and therefore:

$$
\mathbb{P}(|\bar{X}_n - \mu| > \varepsilon) \leq \frac{\sigma^2}{n\varepsilon^2} \to 0.
$$

Where:
- $0$ denotes the valuation date, or “today,” when it appears in term-structure notation.

That proves the weak law directly.

The strong law is deeper because it gives pathwise convergence, but its proof and assumptions are typically stronger.

### 2.5 Intuition for the difference

Weak law:
- for each large $n$, the probability of being far from $\mu$ is small

Strong law:
- with probability 1, the sequence eventually behaves properly for all sufficiently large $n$

A useful way to phrase it:

> Weak law is about being close with high probability at large $n$. Strong law is about actual sample-path convergence almost surely.

### 2.6 Why this matters in practice

In Monte Carlo and empirical estimation:
- WLLN justifies that estimators become accurate in probability
- SLLN justifies that one almost surely observed simulation path of running averages converges

A concise statement is:
- Monte Carlo consistency is often first expressed via the LLN
- the strong law is a stronger pathwise guarantee
- the weak law is often sufficient for statistical consistency arguments

## 3. Modes of convergence cheat sheet

### 3.1 Convergence in probability

$$
X_n \xrightarrow{P} X
$$

Where:
- $X$ is a generic random variable.

means:

$$
\forall \varepsilon > 0, \quad \mathbb{P}(|X_n - X| > \varepsilon) \to 0.
$$

Where:
- $0$ denotes the valuation date, or “today,” when it appears in term-structure notation.

### 3.2 Almost sure convergence

$$
X_n \xrightarrow{a.s.} X
$$

Where:
- $X$ is a generic random variable.

means that with probability 1, the realized sequence converges pointwise.

### 3.3 Strength ordering

$$
X_n \xrightarrow{a.s.} X \implies X_n \xrightarrow{P} X.
$$

Where:
- $X$ is a generic random variable.

The reverse implication is false in general.

## 4. Concise wording

### 4.1 Characteristic function vs. MGF

> The characteristic function is $\phi_X(u)=\mathbb{E}[e^{iuX}]$ and always exists because the integrand has modulus 1. The MGF is $M_X(t)=\mathbb{E}[e^{tX}]$ and may not exist because the exponential can explode in the tails. The MGF is convenient for moments when it exists; the characteristic function is more robust and is heavily used in Fourier methods and option pricing.

### 4.2 Weak vs. strong LLN

> The weak law says the sample average converges to the true mean in probability, so deviations become unlikely as the sample size grows. The strong law says the sample average converges almost surely, which is a stronger pathwise statement. We keep the weak law because it often holds under weaker assumptions and is already enough for many statistical consistency results.
