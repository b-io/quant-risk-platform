# Characteristic Functions and a Brief Introduction to the Fourier Transform

## 1. Why characteristic functions matter

The characteristic function of a real-valued random variable completely determines its probability distribution.

For a random variable $X$, the characteristic function is defined by

$$
\varphi_X(t) = \mathbb{E}\left[e^{itX}\right],
t \in \mathbb{R}.
$$

Characteristic functions are central because they:

- always exist for real-valued random variables,
- uniquely determine the law of $X$,
- turn sums of independent random variables into products,
- provide elegant proofs of limit theorems such as the CLT,
- connect probability theory to Fourier analysis.

---

## 2. Where they come from

If $X$ has a density $f_X(x)$, then

$$
\varphi_X(t) = \int_{-\infty}^{\infty} e^{itx} f_X(x) dx.
$$

This is essentially the **Fourier transform** of the density, up to sign conventions.

Many texts define the Fourier transform of $f$ as

$$
\widehat{f}(t) = \int_{-\infty}^{\infty} e^{-itx} f(x) dx.
$$

So the characteristic function can be viewed as the Fourier transform evaluated with the opposite sign in the
exponential:

$$
\varphi_X(t) = \widehat{f_X}(-t).
$$

If $X$ is discrete with values $x_k$ and probabilities $p_k$, then

$$
\varphi_X(t) = \sum_k e^{itx_k} p_k.
$$

Thus the same object works for both discrete and continuous distributions.

---

## 3. A brief intuition for the Fourier transform

The Fourier transform rewrites a function as a superposition of oscillatory waves $e^{itx}$.

In probability, it is useful because:

- oscillatory functions remain bounded,
- convolution in real space becomes multiplication in Fourier space,
- sums of independent random variables correspond to products of characteristic functions.

This is the key algebraic simplification:

If $X$ and $Y$ are independent, then

$$
\varphi_{X+Y}(t) = \varphi_X(t)\varphi_Y(t).
$$

Proof:

$$
\varphi_{X+Y}(t)
= \mathbb{E}\left[e^{it(X+Y)}\right]
= \mathbb{E}\left[e^{itX} e^{itY}\right].
$$

By independence,

$$
\mathbb{E}\left[e^{itX} e^{itY}\right]
= \mathbb{E}\left[e^{itX}\right] \mathbb{E}\left[e^{itY}\right]
= \varphi_X(t)\varphi_Y(t).
$$

---

## 4. Basic properties

For any real-valued random variable $X$:

### Normalization

$$
\varphi_X(0) = 1.
$$

### Bound

$$
|\varphi_X(t)| \le 1.
$$

This holds because $|e^{itX}| = 1$.

### Symmetry

$$
\varphi_X(-t) = \overline{\varphi_X(t)}.
$$

If the distribution is symmetric about $0$, the characteristic function is real-valued.

### Affine transformation

If $Y = aX + b$, then

$$
\varphi_Y(t) = e^{itb} \varphi_X(at).
$$

### Independence and sums

If $X_1, \dots, X_n$ are independent, then

$$
\varphi_{X_1 + \cdots + X_n}(t) = \prod_{k=1}^n \varphi_{X_k}(t).
$$

This is one of the main reasons characteristic functions are so useful.

---

## 5. Moments from the characteristic function

If the relevant moments exist, derivatives of $\varphi_X$ at the origin recover them.

Differentiate under the expectation sign:

$$
\varphi_X'(t) = \mathbb{E}\left[iX e^{itX}\right].
$$

So at $t=0$,

$$
\varphi_X'(0) = i \mathbb{E}[X].
$$

Hence

$$
\mathbb{E}[X] = \frac{\varphi_X'(0)}{i} = -i \varphi_X'(0).
$$

Similarly,

$$
\varphi_X''(0) = -\mathbb{E}[X^2].
$$

Thus one may recover variance from the first two derivatives.

---

## 6. Uniqueness and inversion

A characteristic function uniquely determines the distribution of $X$.

This means:

$$
\varphi_X(t) = \varphi_Y(t) \text{ for all } t \implies X \stackrel{d}{=} Y.
$$

When a density exists and is sufficiently regular, one can recover it through an inversion formula of Fourier type.
This is why characteristic functions provide an alternative route to working directly with pdfs or cdfs.

---

## 7. Examples of characteristic functions

### 7.1. Bernoulli$(p)$

If $X \in \{0,1\}$ with

$$
\mathbb{P}(X=1)=p,
\mathbb{P}(X=0)=1-p,
$$

then

$$
\varphi_X(t) = (1-p) + p e^{it}.
$$

### 7.2. Poisson$(\lambda)$

If $X \sim \mathrm{Poisson}(\lambda)$, then

$$
\varphi_X(t) = \exp\bigl(\lambda(e^{it}-1)\bigr).
$$

### 7.3. Normal$(\mu, \sigma^2)$

If $X \sim \mathcal{N}(\mu, \sigma^2)$, then

$$
\varphi_X(t) = \exp\left(it\mu - \frac{1}{2}\sigma^2 t^2\right).
$$

This form is especially important because products of Gaussian characteristic functions remain Gaussian.

### 7.4. Exponential$(\lambda)$

For $X \sim \mathrm{Exponential}(\lambda)$,

$$
\varphi_X(t) = \frac{\lambda}{\lambda - it}.
$$

### 7.5. Uniform$([a,b])$

If $X \sim \mathrm{Uniform}(a,b)$, then

$$
\varphi_X(t) = \frac{e^{itb} - e^{ita}}{it(b-a)},
t \ne 0,
$$

and $\varphi_X(0)=1$.

---

## 8. Characteristic functions and the CLT

Characteristic functions provide one of the cleanest proofs of the Central Limit Theorem.

If

$$
Z_n = \frac{1}{\sqrt{n}}\sum_{k=1}^n Y_k,
$$

where the $Y_k$ are i.i.d. with mean $0$ and variance $1$, then

$$
\varphi_{Z_n}(t) = \left(\varphi_Y\left(\frac{t}{\sqrt{n}}\right)\right)^n.
$$

Using the Taylor expansion near $0$,

$$
\varphi_Y(u) = 1 - \frac{u^2}{2} + o(u^2),
$$

we obtain

$$
\varphi_{Z_n}(t)
= \left(1 - \frac{t^2}{2n} + o\left(\frac{1}{n}\right)\right)^n
\to e^{-t^2/2},
$$

which is the characteristic function of $\mathcal{N}(0,1)$.

---

## 9. Characteristic functions and Monte Carlo

Characteristic functions are useful in simulation and quantitative finance for several reasons.

### 9.1. Understanding sums and aggregation

If a portfolio loss is modeled as a sum of independent or conditionally independent components,

$$
L = L_1 + \cdots + L_n,
$$

then

$$
\varphi_L(t) = \prod_{k=1}^n \varphi_{L_k}(t).
$$

This is often much easier to manipulate analytically than the density of $L$.

### 9.2. Benchmarking Monte Carlo outputs

For models with known characteristic functions, one can compare Monte Carlo estimates against analytical transforms or
semi-analytical pricing formulas.

### 9.3. Fourier-based pricing methods

In some asset-pricing models, the characteristic function is known even when the density is not simple. One can then
price options or other claims via Fourier inversion rather than direct simulation.

---

## 10. Summary

Characteristic functions are powerful because they:

- always exist,
- uniquely identify distributions,
- linearize sums of independent random variables through multiplication,
- connect probability theory to Fourier analysis,
- provide elegant proofs of the CLT,
- support semi-analytical methods in quantitative finance.
