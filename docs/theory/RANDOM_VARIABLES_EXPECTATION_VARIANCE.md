# Random Variables, Expectation, Variance, Covariance, and Core Identities

## 1. Why random variables matter

A **random variable** is a numerical quantity defined on an experiment with uncertainty.

- In a coin toss, the outcome is heads or tails.
- A random variable can convert this into a number.

$$
X =
\begin{cases}
1, & \text{if heads},\\
0, & \text{if tails}.
\end{cases}
$$

- In finance, $X$ may represent a return, a default indicator, a recovery rate, a cash flow, or the future value of a portfolio.

A random variable is called “random” because its value is unknown before the experiment is realized.

---

## 2. Discrete and continuous random variables

### 2.1 Discrete random variables

A discrete random variable takes values in a finite or countable set:

$$
x_1, x_2, x_3, \dots
$$

Its law is described by the **probability mass function** (pmf):

$$
p_X(x) = \mathbb{P}(X = x).
$$

These probabilities satisfy:

$$
p_X(x) \ge 0,
\qquad \sum_x p_X(x) = 1.
$$

**Example.** For a fair die,

$$
\mathbb{P}(X = k) = \frac{1}{6},
\qquad k = 1, \dots, 6.
$$

### 2.2 Continuous random variables

A continuous random variable is described by a **probability density function** (pdf) $f_X(x)$ such that:

$$
f_X(x) \ge 0,
\qquad \int_{-\infty}^{\infty} f_X(x)\,dx = 1.
$$

Probabilities are obtained by integration:

$$
\mathbb{P}(a \le X \le b) = \int_a^b f_X(x)\,dx.
$$

For a continuous variable,

$$
\mathbb{P}(X = x) = 0
$$

for every single point $x$.

**Example.** If $X \sim \mathcal{N}(0,1)$, then

$$
f_X(x) = \frac{1}{\sqrt{2\pi}} e^{-x^2/2}.
$$

---

## 3. Distribution function

For any random variable $X$, the **cumulative distribution function** (cdf) is

$$
F_X(x) = \mathbb{P}(X \le x).
$$

Properties:

- $F_X$ is non-decreasing.
- $0 \le F_X(x) \le 1$.
- $\lim_{x \to -\infty} F_X(x) = 0$.
- $\lim_{x \to +\infty} F_X(x) = 1$.

The cdf is the most general way to describe the law of a random variable.

---

## 4. Expectation: definition and meaning

The **expectation** or **mean** of a random variable is its average value under repeated sampling.

### 4.1 Discrete case

If $X$ is discrete,

$$
\mathbb{E}[X] = \sum_x x\,\mathbb{P}(X = x).
$$

### 4.2 Continuous case

If $X$ is continuous with density $f_X$,

$$
\mathbb{E}[X] = \int_{-\infty}^{\infty} x f_X(x)\,dx.
$$

### 4.3 Expectation of a function of a random variable

More generally, for any integrable function $g$,

$$
\mathbb{E}[g(X)] =
\begin{cases}
\sum_x g(x)\,\mathbb{P}(X = x), & \text{if } X \text{ is discrete},\\
\int_{-\infty}^{\infty} g(x) f_X(x)\,dx, & \text{if } X \text{ is continuous}.
\end{cases}
$$

This formula is fundamental in probability, statistics, and quantitative finance.

**Example.** If $X$ is a fair die,

$$
\mathbb{E}[X] = \sum_{k=1}^6 k \cdot \frac{1}{6} = \frac{1+2+3+4+5+6}{6} = 3.5.
$$

---

## 5. Linearity of expectation

For constants $a,b$ and random variables $X,Y$,

$$
\mathbb{E}[aX + bY] = a\mathbb{E}[X] + b\mathbb{E}[Y].
$$

More generally,

$$
\mathbb{E}\left[\sum_{i=1}^n a_i X_i\right] = \sum_{i=1}^n a_i \mathbb{E}[X_i].
$$

### Proof in the discrete case

Suppose $X$ and $Y$ are discrete. Then

$$
\mathbb{E}[aX+bY] = \sum_{x,y}(ax+by)\,\mathbb{P}(X=x,Y=y).
$$

Split the sum:

$$
\mathbb{E}[aX+bY]
= a\sum_{x,y} x\,\mathbb{P}(X=x,Y=y)
+ b\sum_{x,y} y\,\mathbb{P}(X=x,Y=y).
$$

Now sum over the second variable:

$$
\sum_y \mathbb{P}(X=x,Y=y) = \mathbb{P}(X=x),
$$

so the first term becomes $a\mathbb{E}[X]$. Similarly, the second becomes $b\mathbb{E}[Y]$.

Hence,

$$
\mathbb{E}[aX+bY] = a\mathbb{E}[X] + b\mathbb{E}[Y].
$$

### Important remark

Linearity does **not** require independence.

---

## 6. Variance: definition and intuition

The **variance** measures dispersion around the mean.

$$
\operatorname{Var}(X) = \mathbb{E}\left[(X - \mathbb{E}[X])^2\right].
$$

If $\mu = \mathbb{E}[X]$, then

$$
\operatorname{Var}(X) = \mathbb{E}[(X-\mu)^2].
$$

The **standard deviation** is

$$
\sigma_X = \sqrt{\operatorname{Var}(X)}.
$$

Interpretation:

- Large variance means outcomes are spread out.
- Small variance means outcomes are concentrated near the mean.

---

## 7. The computational formula for variance

Starting from

$$
\operatorname{Var}(X) = \mathbb{E}[(X-\mu)^2],
\qquad \mu = \mathbb{E}[X],
$$

expand the square:

$$
(X-\mu)^2 = X^2 - 2\mu X + \mu^2.
$$

Take expectations:

$$
\mathbb{E}[(X-\mu)^2]
= \mathbb{E}[X^2] - 2\mu\mathbb{E}[X] + \mathbb{E}[\mu^2].
$$

Since $\mu$ is a constant,

$$
\mathbb{E}[\mu^2] = \mu^2,
$$

and since $\mathbb{E}[X] = \mu$,

$$
-2\mu\mathbb{E}[X] = -2\mu^2.
$$

Therefore,

$$
\operatorname{Var}(X) = \mathbb{E}[X^2] - 2\mu^2 + \mu^2 = \mathbb{E}[X^2] - \mu^2.
$$

Hence,

$$
\boxed{\operatorname{Var}(X) = \mathbb{E}[X^2] - (\mathbb{E}[X])^2.}
$$

---

## 8. Variance under affine transformations

Let $Y = aX + b$. Then

$$
\operatorname{Var}(aX+b) = a^2\operatorname{Var}(X).
$$

### Proof

First,

$$
\mathbb{E}[aX+b] = a\mathbb{E}[X] + b.
$$

Then

$$
(aX+b) - \mathbb{E}[aX+b] = a(X-\mathbb{E}[X]).
$$

So

$$
\operatorname{Var}(aX+b)
= \mathbb{E}\left[(a(X-\mathbb{E}[X]))^2\right]
= a^2 \mathbb{E}[(X-\mathbb{E}[X])^2]
= a^2\operatorname{Var}(X).
$$

Interpretation:

- adding a constant does not change variability,
- multiplying by $a$ scales variability by $a^2$.

---

## 9. Covariance

The **covariance** of $X$ and $Y$ is

$$
\operatorname{Cov}(X,Y) = \mathbb{E}[(X-\mathbb{E}[X])(Y-\mathbb{E}[Y])].
$$

A useful formula is

$$
\operatorname{Cov}(X,Y) = \mathbb{E}[XY] - \mathbb{E}[X]\mathbb{E}[Y].
$$

### Proof

Let $\mu_X = \mathbb{E}[X]$ and $\mu_Y = \mathbb{E}[Y]$. Expand:

$$
(X-\mu_X)(Y-\mu_Y) = XY - \mu_XY - \mu_YX + \mu_X\mu_Y.
$$

Take expectations:

$$
\operatorname{Cov}(X,Y)
= \mathbb{E}[XY] - \mu_X\mathbb{E}[Y] - \mu_Y\mathbb{E}[X] + \mu_X\mu_Y.
$$

Since $\mathbb{E}[X]=\mu_X$ and $\mathbb{E}[Y]=\mu_Y$,

$$
\boxed{\operatorname{Cov}(X,Y) = \mathbb{E}[XY] - \mathbb{E}[X]\mathbb{E}[Y].}
$$

If $X$ and $Y$ are independent, then $\mathbb{E}[XY] = \mathbb{E}[X]\mathbb{E}[Y]$, so

$$
\operatorname{Cov}(X,Y) = 0.
$$

The converse is not always true.

---

## 10. Variance of a sum

For random variables $X_1,\dots,X_n$,

$$
\operatorname{Var}\left(\sum_{i=1}^n X_i\right)
=
\sum_{i=1}^n \operatorname{Var}(X_i)
+ 2\sum_{1 \le i < j \le n} \operatorname{Cov}(X_i,X_j).
$$

### Proof

Let

$$
S = \sum_{i=1}^n X_i.
$$

Then

$$
S - \mathbb{E}[S] = \sum_{i=1}^n (X_i - \mathbb{E}[X_i]).
$$

Therefore,

$$
\operatorname{Var}(S)
= \mathbb{E}\left[\left(\sum_{i=1}^n (X_i-\mathbb{E}[X_i])\right)^2\right].
$$

Expand the square:

$$
\left(\sum_{i=1}^n a_i\right)^2
= \sum_{i=1}^n a_i^2 + 2\sum_{1 \le i < j \le n} a_i a_j.
$$

With $a_i = X_i - \mathbb{E}[X_i]$, we get

$$
\operatorname{Var}(S)
=
\sum_{i=1}^n \mathbb{E}[(X_i-\mathbb{E}[X_i])^2]
+ 2\sum_{1 \le i < j \le n}
\mathbb{E}[(X_i-\mathbb{E}[X_i])(X_j-\mathbb{E}[X_j])].
$$

Recognizing variance and covariance yields

$$
\boxed{
\operatorname{Var}\left(\sum_{i=1}^n X_i\right)
=
\sum_{i=1}^n \operatorname{Var}(X_i)
+ 2\sum_{1 \le i < j \le n} \operatorname{Cov}(X_i,X_j).
}
$$

### Independent case

If the $X_i$ are independent, then all covariance terms vanish, so

$$
\boxed{\operatorname{Var}\left(\sum_{i=1}^n X_i\right) = \sum_{i=1}^n \operatorname{Var}(X_i).}
$$

This is why for a random walk

$$
S_n = X_1 + \cdots + X_n
$$

with independent increments satisfying $\operatorname{Var}(X_i)=1$, we get

$$
\operatorname{Var}(S_n)=n.
$$

---

## 11. Correlation

The **correlation coefficient** is the normalized covariance:

$$
\rho_{X,Y} =
\frac{\operatorname{Cov}(X,Y)}{\sqrt{\operatorname{Var}(X)\operatorname{Var}(Y)}}.
$$

It satisfies

$$
-1 \le \rho_{X,Y} \le 1.
$$

Interpretation:

- $\rho \approx 1$: strong positive linear dependence,
- $\rho \approx -1$: strong negative linear dependence,
- $\rho \approx 0$: weak linear dependence.

---

## 12. Conditional expectation and variance decomposition

If $Y$ contains information about $X$, the **conditional expectation** $\mathbb{E}[X \mid Y]$ is the best mean prediction of $X$ given $Y$.

### Discrete definition

If $\mathbb{P}(Y=y) > 0$, then

$$
\mathbb{E}[X \mid Y=y] = \sum_x x\,\mathbb{P}(X=x \mid Y=y).
$$

### Law of total expectation

$$
\boxed{\mathbb{E}[X] = \mathbb{E}[\mathbb{E}[X \mid Y]].}
$$

### Law of total variance

$$
\boxed{\operatorname{Var}(X) = \mathbb{E}[\operatorname{Var}(X \mid Y)] + \operatorname{Var}(\mathbb{E}[X \mid Y]).}
$$

Interpretation:

- total uncertainty
- = average unexplained uncertainty after conditioning
- + uncertainty explained by the conditioning information.

---

## 13. Indicator variables

If $A$ is an event, define the indicator

$$
\mathbf{1}_A =
\begin{cases}
1, & \text{if } A \text{ occurs},\\
0, & \text{otherwise}.
\end{cases}
$$

Then

$$
\mathbb{E}[\mathbf{1}_A] = \mathbb{P}(A).
$$

This identity is used constantly in counting arguments, default indicators, and Monte Carlo estimators.

---

## 14. Why these identities matter in quantitative finance

These formulas are the foundation for:

- pricing by expected discounted payoff,
- variance and covariance risk decomposition,
- Monte Carlo estimation,
- VaR and Expected Shortfall approximation,
- factor models and scenario aggregation,
- the Law of Large Numbers and Central Limit Theorem.
