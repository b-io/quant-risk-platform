# Statistical Distributions, Maximum Likelihood, Characteristic Functions, and Asymptotic Behavior

## 1. Why distributions matter

A probability distribution specifies how uncertainty is modeled. In applications, it tells us which values a random
variable can take, how likely those values are, and how quantities such as means, variances, tail probabilities, and
transformations should be computed.

In quantitative finance, risk management, econometrics, statistics, and stochastic modeling, distributions appear
everywhere:

- **Bernoulli** variables model binary events such as default vs. survival, breach vs. no breach, success vs. failure,
  or trigger vs. no trigger.
- **Binomial** laws model the number of successes across a fixed number of comparable trials.
- **Poisson** laws model counts of arrivals or events over time, such as transaction arrivals, operational incidents, or
  jump counts.
- **Exponential** laws model waiting times between events in simple constant-intensity settings.
- **Normal** laws model measurement errors, residual shocks, and many large-sample approximations.
- **Lognormal** and **Gamma** laws model strictly positive quantities.
- **Beta** laws model bounded proportions such as recovery rates, utilization ratios, or probabilities.
- **Student's $t$** laws model heavy-tailed standardized shocks when Gaussian tails are too light.

The choice of distribution matters because it affects:

1. the shape of the likelihood function,
2. the behavior of estimators,
3. the accuracy of confidence intervals and tests,
4. the realism of tail-risk assessments,
5. the tractability of simulation and analytical pricing.

This chapter summarizes several important discrete and continuous families, their parameters, supports, moments,
characteristic functions, standard maximum-likelihood estimators when available in closed form, and their main
asymptotic properties.

### How to read the families in this chapter

For several distributions, it is useful to distinguish three things:

1. the **general family**, with its full parameter set;
2. a **special or standardized case**, obtained by fixing some parameters to convenient reference values;
3. the **reason for that standardization**, usually to simplify formulas or to create a canonical benchmark.

For example, the normal family is $\mathcal{N}(\mu,\sigma^2)$, while the **standard normal** is the normalized case
$\mathcal{N}(0,1)$. Similarly, in stochastic-process notes one often works with **standard Brownian motion**, which is
the normalized case of a more general Brownian scaling. Throughout these notes, we try to say explicitly when a formula
is a general definition and when it is a convenient normalization.

---

## 2. Likelihood and maximum likelihood estimation

Suppose that we observe independent data

$$
X_1, \dots, X_n,
$$

where each observation is assumed to come from a distribution belonging to a parametric family indexed by a
parameter $\theta \in \Theta$. We write the realized sample as

$$
X_1=x_1,\dots,X_n=x_n.
$$

If the model has density $f(x;\theta)$, the **likelihood** is

$$
L(\theta; x_1,\dots,x_n)=\prod_{i=1}^n f(x_i;\theta).
$$

If the model is discrete with pmf $p(x;\theta)$, the analogous formula is

$$
L(\theta; x_1,\dots,x_n)=\prod_{i=1}^n p(x_i;\theta).
$$

The likelihood is not a probability in $\theta$; rather, it measures how compatible different parameter values are with
the observed sample.

The **maximum likelihood estimator** (MLE) is defined by

$$
\hat\theta_{\mathrm{MLE}}=\arg\max_{\theta\in\Theta} L(\theta; x_1,\dots,x_n).
$$

In practice, one usually maximizes the **log-likelihood**

$$
\ell(\theta)=\log L(\theta),
$$

because logarithms transform products into sums:

$$
\ell(\theta)=\sum_{i=1}^n \log f(x_i;\theta)
\qquad\text{or}\qquad
\ell(\theta)=\sum_{i=1}^n \log p(x_i;\theta).
$$

This is easier to differentiate and numerically more stable.

### 2.1 Why maximum likelihood is so important

Maximum likelihood plays a central role because it is systematic, general, and often statistically efficient. Once a
probabilistic model is specified, the likelihood gives a principled way to estimate unknown parameters. Under suitable
regularity conditions, MLEs are:

- **consistent**, meaning they converge to the true parameter,
- **asymptotically normal**, meaning their fluctuations are approximately Gaussian in large samples,
- **asymptotically efficient**, meaning they often achieve the lowest possible variance asymptotically among regular
  estimators.

For simple models, the MLE can often be written explicitly. For more complex models, it is computed numerically using
optimization methods.

### 2.2 A general score-equation view

If $\theta$ is one-dimensional and the likelihood is smooth, interior maximizers often satisfy the **score equation**

$$
U(\theta)=\frac{\partial \ell(\theta)}{\partial \theta}=0.
$$

If $\theta$ is vector-valued, the score becomes the gradient

$$
U(\theta)=\nabla_\theta \ell(\theta),
$$

and the MLE often solves

$$
U(\hat\theta)=0.
$$

One must still verify that the solution corresponds to a maximum and lies in the admissible parameter space.

---

## 3. Discrete distributions

## 3.1 Bernoulli distribution

A random variable $X$ has a **Bernoulli distribution** with parameter $p$, written

$$
X \sim \mathrm{Bernoulli}(p),
\qquad 0 \le p \le 1,
$$

if it takes values in the set $\{0,1\}$ with

$$
\mathbb{P}(X=1)=p,
\qquad
\mathbb{P}(X=0)=1-p.
$$

Here $p$ is the success probability.

> While $p$ can be $0$ or $1$, likelihood derivatives and Fisher information formulas typically assume $p \in (0,1)$ to
> avoid degenerate cases and boundary issues.

### pmf

$$
\mathbb{P}(X=x)=p^x(1-p)^{1-x},
\qquad x\in\{0,1\}.
$$

### Mean and variance

$$
\mathbb{E}[X]=p,
\qquad
\mathrm{Var}(X)=p(1-p).
$$

### Characteristic function

$$
\varphi_X(t)=\mathbb{E}[e^{itX}]=(1-p)+pe^{it},
\qquad t\in\mathbb{R}.
$$

### MLE

Suppose $X_1,\dots,X_n$ are i.i.d. Bernoulli$(p)$ and we observe $x_1,\dots,x_n$. Then

$$
L(p)=\prod_{i=1}^n p^{x_i}(1-p)^{1-x_i}=p^{\sum_{i=1}^n x_i}(1-p)^{n-\sum_{i=1}^n x_i}.
$$

Hence

$$
\ell(p)=\left(\sum_{i=1}^n x_i\right)\log p+
\left(n-\sum_{i=1}^n x_i\right)\log(1-p).
$$

Differentiating gives

$$
\frac{d\ell}{dp}=\frac{\sum x_i}{p}-\frac{n-\sum x_i}{1-p}.
$$

Setting this equal to zero yields

$$
\hat p=\frac{1}{n}\sum_{i=1}^n x_i=\bar X.
$$

So the MLE is simply the sample proportion of successes.

### Use cases

The Bernoulli law is the simplest model for a single yes/no outcome. In credit risk, one may define $X_i=1$ if
issuer $i$ defaults during a given horizon and $X_i=0$ otherwise. In operational-risk or control settings, one may
set $X_i=1$ if a breach occurs during a period. In reliability, it may represent whether a component failed before a
deadline. In experimentation, it is the canonical model for success/failure trials. Even when real systems are more
complex, the Bernoulli variable remains a basic building block because many portfolio models are formed by summing
Bernoulli indicators.

### Asymptotics

If $\hat p=\bar X$, then by the Law of Large Numbers,

$$
\hat p\xrightarrow{p} p.
$$

By the Central Limit Theorem,

$$
\sqrt{n}(\hat p-p)\xrightarrow{d}\mathcal{N}(0,p(1-p)).
$$

This large-sample approximation is the basis for approximate confidence intervals for a proportion.

---

## 3.2 Binomial distribution

A random variable $X$ has a **Binomial distribution** with parameters $m$ and $p$, written

$$
X\sim\mathrm{Binomial}(m,p),
\qquad m\in\mathbb{N},\quad m \ge 1, \quad 0 \le p \le 1,
$$

if it counts the number of successes in $m$ independent Bernoulli trials, each with success probability $p$.

> We use $m$ here for the number of Bernoulli trials in the Binomial distribution, so that $n$ remains available for
> sample size in estimation problems. This avoids a common notational ambiguity.

### pmf

$$
\mathbb{P}(X=k)=\binom{m}{k}p^k(1-p)^{m-k},
\qquad k \in \{0,1,\dots,m\}.
$$

### Mean and variance

$$
\mathbb{E}[X]=mp,
\qquad
\mathrm{Var}(X)=mp(1-p).
$$

### Characteristic function

$$
\varphi_X(t)=\bigl((1-p)+pe^{it}\bigr)^m.
$$

### Interpretation

A Binomial random variable is the sum of $m$ independent Bernoulli variables:

$$
X=\sum_{j=1}^m B_j,
\qquad B_j\overset{\text{i.i.d.}}{\sim}\mathrm{Bernoulli}(p).
$$

This explains both its mean and its characteristic function.

### Use cases

The Binomial model is natural when the total number of opportunities is fixed and each opportunity leads to success or
failure. In finance, it can model the number of names defaulting in a homogeneous portfolio over a fixed horizon under
an independence assumption. In quality control, it can represent the number of defective items in a batch. In compliance
monitoring, it can count how many transactions among a fixed set violate a rule. In clinical or experimental settings,
it is often used for the number of responders out of a fixed number of participants.

### Asymptotics

When $m$ is large and $p$ is not too close to $0$ or $1$, the standardized variable is approximately normal:

$$
\frac{X-mp}{\sqrt{mp(1-p)}}\approx \mathcal{N}(0,1).
$$

In the rare-event regime,

$$
m\to\infty,
\qquad p\to 0,
\qquad mp\to\lambda,
$$

we have the Poisson limit

$$
X\xrightarrow{d}\mathrm{Poisson}(\lambda).
$$

This approximation is especially useful when the number of trials is very large and each success is rare.

---

## 3.3 Poisson distribution

A random variable $X$ has a **Poisson distribution** with parameter $\lambda > 0$, written

$$
X\sim\mathrm{Poisson}(\lambda),
\qquad \lambda > 0,
$$

if

$$
\mathbb{P}(X=k)=e^{-\lambda}\frac{\lambda^k}{k!},
\qquad k \in \{0,1,2,\dots\}.
$$

The parameter $\lambda$ is both the mean and the variance.

### Mean and variance

$$
\mathbb{E}[X]=\lambda,
\qquad
\mathrm{Var}(X)=\lambda.
$$

### Characteristic function

$$
\varphi_X(t)=\exp\bigl(\lambda(e^{it}-1)\bigr).
$$

### MLE

Suppose $X_1,\dots,X_n$ are i.i.d. Poisson$(\lambda)$. Then

$$
L(\lambda)=\prod_{i=1}^n e^{-\lambda}\frac{\lambda^{x_i}}{x_i!}
=e^{-n\lambda}\lambda^{\sum_{i=1}^n x_i}\prod_{i=1}^n\frac{1}{x_i!}.
$$

Therefore,

$$
\ell(\lambda)=-n\lambda+\left(\sum_{i=1}^n x_i\right)\log\lambda+C,
$$

where $C$ does not depend on $\lambda$. Differentiating gives

$$
\frac{d\ell}{d\lambda}=-n+\frac{\sum x_i}{\lambda}.
$$

Hence the MLE is

$$
\hat\lambda=\frac{1}{n}\sum_{i=1}^n x_i=\bar X.
$$

### Use cases

The Poisson law is a basic model for event counts over a fixed interval when events occur independently and with a
constant average rate. It is widely used for transaction arrivals, insurance claims in a simple setting, system
incidents, jump counts in reduced-form or jump-diffusion approximations, and default counts when defaults are rare and
weakly dependent. It is attractive because it is analytically convenient and serves as the canonical counting
distribution associated with the Poisson process.

### Asymptotics and approximations

For large $\lambda$,

$$
\frac{X-\lambda}{\sqrt{\lambda}}\xrightarrow{d}\mathcal{N}(0,1).
$$

The Poisson law also arises as a limit of Binomial laws in rare-event settings, which helps explain its ubiquity in
applications involving many opportunities for a rare event.

---

## 3.4 Geometric distribution

A random variable $X$ has a **Geometric distribution** with parameter $p$, written

$$
X\sim\mathrm{Geometric}(p),
\qquad 0 < p \le 1,
$$

if it counts the number of trials up to and including the first success:

$$
\mathbb{P}(X=k)=(1-p)^{k-1}p,
\qquad k \in \{1,2,\dots\}.
$$

> This is one of two common conventions. Under this convention, $X \in \{1,2,\dots\}$ and counts the trial on which the
> first success occurs. Another convention counts the number of *failures* before the first success, in which case the
> support is $\{0,1,2,\dots\}$.

### Mean and variance

$$
\mathbb{E}[X]=\frac{1}{p},
\qquad
\mathrm{Var}(X)=\frac{1-p}{p^2}.
$$

### Characteristic function

$$
\varphi_X(t)=\frac{pe^{it}}{1-(1-p)e^{it}},
\qquad t\in\mathbb{R}.
$$

### A key property: memorylessness

The geometric distribution is the only discrete distribution with the memoryless property:

$$
\mathbb{P}(X>m+k\mid X>m)=\mathbb{P}(X>k).
$$

This makes it the discrete-time analogue of the exponential distribution.

### Use cases

The geometric law is useful for simple discrete-time waiting-time questions: the number of periods until the first
default, the number of monitoring dates until the first breach, the number of customer contacts until the first
response, or the number of iterations until an event is triggered. It is often a pedagogical stepping stone before
moving to continuous-time intensity models, where the exponential distribution plays the analogous role.

### Asymptotics

Under suitable rescaling as the time grid becomes finer and $p$ becomes small, geometric waiting times converge to
exponential waiting times.

---

## 4. Continuous distributions

## 4.1 Uniform distribution

A random variable $X$ has a **Uniform distribution** on the interval $[a,b]$, written

$$
X\sim\mathrm{Uniform}(a,b),
\qquad a < b, \quad a,b \in \mathbb{R},
$$

if its density is constant on that interval and zero elsewhere:

$$
f_X(x)=
\begin{cases}
\dfrac{1}{b-a}, & x \in [a,b], \\[1ex]
0, & x \notin [a,b].
\end{cases}
$$

### Mean and variance

$$
\mathbb{E}[X]=\frac{a+b}{2},
\qquad
\mathrm{Var}(X)=\frac{(b-a)^2}{12}.
$$

### Characteristic function

$$
\varphi_X(t)=
\begin{cases}
\dfrac{e^{itb}-e^{ita}}{it(b-a)}, & t\neq 0,\\[1ex]
1, & t=0.
\end{cases}
$$

### Use cases

The uniform law is a simple model for bounded uncertainty when all values in a range are treated as equally plausible.
It is often used in elementary simulation, in randomized algorithms, and as the base distribution for inverse-transform
methods. In practice it is rarely the best model for real economic or financial variables, but it is an important
reference case because it is simple, bounded, and easy to simulate.

---

## 4.2 Exponential distribution

A random variable $X$ has an **Exponential distribution** with rate parameter $\lambda > 0$, written

$$
X\sim\mathrm{Exponential}(\lambda),
\qquad \lambda > 0,
$$

if its density is

$$
f_X(x)=
\begin{cases}
\lambda e^{-\lambda x}, & x \in [0,\infty), \\[1ex]
0, & x < 0.
\end{cases}
$$

Here $\lambda$ is the rate, so the mean waiting time is $1/\lambda$.

### Mean and variance

$$
\mathbb{E}[X]=\frac{1}{\lambda},
\qquad
\mathrm{Var}(X)=\frac{1}{\lambda^2}.
$$

### Characteristic function

$$
\varphi_X(t)=\frac{\lambda}{\lambda-it},
\qquad t\in\mathbb{R}.
$$

### MLE

Suppose $X_1,\dots,X_n$ are i.i.d. Exponential$(\lambda)$. Then

$$
L(\lambda)=\prod_{i=1}^n \lambda e^{-\lambda x_i}=\lambda^n e^{-\lambda\sum_{i=1}^n x_i}.
$$

Hence

$$
\ell(\lambda)=n\log\lambda-\lambda\sum_{i=1}^n x_i.
$$

Differentiating and setting equal to zero gives

$$
\hat\lambda=\frac{n}{\sum_{i=1}^n x_i}=\frac{1}{\bar X}.
$$

### A key property: memorylessness

The exponential distribution is the only continuous distribution with the memoryless property:

$$
\mathbb{P}(X>s+t\mid X>s)=\mathbb{P}(X>t),
\qquad s,t \ge 0.
$$

This is why it is central in constant-intensity event modeling.

### Use cases

The exponential law is the natural waiting-time distribution associated with a Poisson process. If events arrive at
constant intensity $\lambda$, then the gap between successive arrivals is exponential. In finance and risk, it appears
in simple reduced-form default-time models where the default intensity is constant. In reliability, it models lifetimes
of components that do not age. In queueing and operational systems, it gives a baseline model for inter-arrival or
service times when the hazard rate is constant.

### Asymptotics

The MLE is consistent and asymptotically normal:

$$
\sqrt{n}(\hat\lambda-\lambda)\xrightarrow{d}\mathcal{N}(0,\lambda^2).
$$

---

## 4.3 Normal distribution

A random variable $X$ has a **Normal distribution** with mean $\mu\in\mathbb{R}$ and variance $\sigma^2 > 0$, written

$$
X\sim\mathcal{N}(\mu,\sigma^2),
\qquad \mu\in\mathbb{R}, \quad \sigma > 0,
$$

if its density is

$$
f_X(x)=\frac{1}{\sqrt{2\pi\sigma^2}}\exp\left(-\frac{(x-\mu)^2}{2\sigma^2}\right),
\qquad x\in\mathbb{R}.
$$

The **standard normal** distribution is the normalized special case

$$
Z\sim\mathcal{N}(0,1).
$$

This distinction matters: $\mathcal{N}(\mu,\sigma^2)$ is the general family, while $\mathcal{N}(0,1)$ is the
reference case used to simplify formulas, tables, and limit theorems.

Indeed, if $X\sim\mathcal{N}(\mu,\sigma^2)$ and $\sigma>0$, then

$$
Z=\frac{X-\mu}{\sigma} \sim \mathcal{N}(0,1).
$$

So centering by the mean and scaling by the standard deviation removes the location and scale parameters. This is why
the CLT is usually stated with a standard normal limit rather than a general Gaussian one.

### Mean and variance

$$
\mathbb{E}[X]=\mu,
\qquad
\mathrm{Var}(X)=\sigma^2.
$$

### Characteristic function

$$
\varphi_X(t)=\exp\left(it\mu-\frac{1}{2}\sigma^2 t^2\right).
$$

### MLEs

Suppose $X_1,\dots,X_n$ are i.i.d. $\mathcal{N}(\mu,\sigma^2)$. Then the MLEs are

$$
\hat\mu=\bar X,
\qquad
\hat\sigma^2_{\mathrm{MLE}}=\frac{1}{n}\sum_{i=1}^n (X_i-\bar X)^2.
$$

Note that the MLE of the variance uses the divisor $n$, not $n-1$. The estimator with divisor $n-1$ is unbiased, but it
is not the MLE.

### Use cases

The normal law is fundamental because it is both mathematically tractable and asymptotically universal. It is the
standard model for measurement errors, residuals in regression models, and aggregate shocks formed by summing many small
effects. In quantitative finance, Gaussian factors often appear in linear risk models, term-structure models, and
approximations to aggregated portfolio PnL. Even when underlying risks are not exactly Gaussian, normal approximations
are often used because of the Central Limit Theorem.

### Asymptotics

The normal family is stable under affine transformations and sums of independent normal variables. More broadly, the
Central Limit Theorem explains why many normalized sample averages are approximately normal even when the underlying
variables are not.

---

## 4.4 Lognormal distribution

A positive random variable $X$ has a **Lognormal distribution** if its natural logarithm is normally distributed:

$$
\log X\sim\mathcal{N}(\mu,\sigma^2),
\qquad \mu \in \mathbb{R}, \quad \sigma > 0.
$$

Equivalently, $X=e^Y$ where $Y\sim\mathcal{N}(\mu,\sigma^2)$.

### Density

$$
f_X(x)=
\begin{cases}
\dfrac{1}{x\sigma\sqrt{2\pi}} \exp\left(-\dfrac{(\log x-\mu)^2}{2\sigma^2}\right), & x \in (0,\infty), \\[2ex]
0, & x \le 0.
\end{cases}
$$

### Mean and variance

$$
\mathbb{E}[X]=e^{\mu+\sigma^2/2},
\qquad
\mathrm{Var}(X)=\left(e^{\sigma^2}-1\right)e^{2\mu+\sigma^2}.
$$

### Characteristic function

There is no simple closed form in elementary functions. The moment-generating function does not exist for positive
arguments.

### Use cases

The lognormal distribution is appropriate for strictly positive variables generated by multiplicative effects.
In Black-Scholes-type models, stock prices are lognormal because log-prices are Gaussian. In economics and engineering,
it also appears for quantities that evolve through repeated proportional changes, such as growth factors, multiplicative
noise, or some forms of income and size distributions. A practical advantage is that positivity is automatic.

### Remarks on inference

Inference is often easier on the log scale. If $Y_i=\log X_i$ are approximately Gaussian, then standard normal-theory
methods can be applied to $Y_i$ and translated back to the original scale.

---

## 4.5 Gamma distribution

A random variable $X$ has a **Gamma distribution** with shape parameter $\alpha > 0$ and rate parameter $\beta > 0$,
written

$$
X\sim\mathrm{Gamma}(\alpha,\beta),
\qquad \alpha > 0, \quad \beta > 0,
$$

if its density is

$$
f_X(x)=
\begin{cases}
\dfrac{\beta^\alpha}{\Gamma(\alpha)}x^{\alpha-1}e^{-\beta x}, & x \in (0,\infty), \\[2ex]
0, & x \le 0.
\end{cases}
$$

> The parameterization above uses the **rate** $\beta$. Some authors use the **scale** $\theta = 1/\beta$, in which case
> the density is $f_X(x) = \frac{1}{\Gamma(\alpha)\theta^\alpha} x^{\alpha-1} e^{-x/\theta}$.

### Mean and variance

$$
\mathbb{E}[X]=\frac{\alpha}{\beta},
\qquad
\mathrm{Var}(X)=\frac{\alpha}{\beta^2}.
$$

### Characteristic function

$$
\varphi_X(t)=\left(1-\frac{it}{\beta}\right)^{-\alpha}.
$$

### Use cases

The Gamma family is flexible for positive and skewed quantities. It is widely used for waiting-time aggregates,
severities, stochastic intensities, and scale parameters in hierarchical models. Because a sum of independent
exponential variables with the same rate is Gamma, it is also natural in queueing and reliability theory. Compared with
the lognormal distribution, the Gamma law is often easier to manipulate analytically.

### Remarks

When $\alpha$ is large, the Gamma distribution becomes more symmetric and can often be approximated by a normal law
after suitable standardization.

---

## 4.6 Beta distribution

A random variable $X$ has a **Beta distribution** with parameters $\alpha > 0$ and $\beta > 0$, written

$$
X\sim\mathrm{Beta}(\alpha,\beta),
\qquad \alpha > 0, \quad \beta > 0,
$$

if its density is

$$
f_X(x)=
\begin{cases}
\dfrac{\Gamma(\alpha+\beta)}{\Gamma(\alpha)\Gamma(\beta)}x^{\alpha-1}(1-x)^{\beta-1}, & x \in (0,1), \\[2ex]
0, & x \notin (0,1).
\end{cases}
$$

### Mean and variance

$$
\mathbb{E}[X]=\frac{\alpha}{\alpha+\beta},
\qquad
\mathrm{Var}(X)=\frac{\alpha\beta}{(\alpha+\beta)^2(\alpha+\beta+1)}.
$$

### Characteristic function

$$
\varphi_X(t)={}_1F_1(\alpha;\alpha+\beta;it),
$$

where ${}_1F_1$ denotes the confluent hypergeometric function.

### Use cases

The Beta law is a natural model for random quantities constrained to lie between $0$ and $1$. In credit risk, recovery
rates and loss-given-default fractions are often modeled using Beta-type distributions because the support is bounded.
In utilization modeling, it can describe ratios such as capacity usage or bounded proportions. In Bayesian statistics,
it is the conjugate prior for a Bernoulli or Binomial success probability, which makes posterior updating especially
simple.

### Shape flexibility

Depending on $(\alpha,\beta)$, the Beta density can be symmetric, skewed left or right, bell-shaped, nearly uniform, or
even U-shaped. This flexibility makes it a standard bounded distribution in applied work.

---

## 4.7 Chi-square distribution

If

$$
Z_1,\dots,Z_k\overset{\text{i.i.d.}}{\sim}\mathcal{N}(0,1)
$$

and

$$
X=\sum_{i=1}^k Z_i^2,
$$

then $X$ has a **Chi-square distribution** with $k$ degrees of freedom, written

$$
X\sim\chi_k^2,
\qquad k \in \mathbb{N}, \quad k \ge 1,
$$

and its support is $x \in [0,\infty)$.

### Mean and variance

$$
\mathbb{E}[X]=k,
\qquad
\mathrm{Var}(X)=2k.
$$

### Characteristic function

$$
\varphi_X(t)=(1-2it)^{-k/2}.
$$

### Use cases

The chi-square distribution plays a central role in Gaussian inference. Sample variances, quadratic forms, and
likelihood-ratio statistics often reduce to chi-square laws or asymptotically chi-square laws. It is fundamental in
variance estimation, goodness-of-fit procedures, and many classical test statistics.

### Asymptotics

For large $k$, the standardized chi-square distribution is approximately normal:

$$
\frac{X-k}{\sqrt{2k}}\xrightarrow{d}\mathcal{N}(0,1).
$$

---

## 4.8 Student's $t$ distribution

Let

$$
Z\sim\mathcal{N}(0,1),
\qquad
U\sim\chi_\nu^2,
$$

be independent, where $\nu > 0$ is the number of degrees of freedom. Then

$$
T=\frac{Z}{\sqrt{U/\nu}}
$$

has a **Student's $t$ distribution** with $\nu$ degrees of freedom, written $T \sim t_\nu$.

### Mean and variance

Existence conditions for the moments are important:

- **Mean**: $\mathbb{E}[T]=0$ if and only if **$\nu > 1$**.
- **Variance**: $\mathrm{Var}(T)=\dfrac{\nu}{\nu-2}$ if and only if **$\nu > 2$**.

For small $\nu$, the tails are much heavier than Gaussian tails.

### Characteristic function

There is no short elementary closed form. It can be expressed using modified Bessel functions.

### Use cases

The Student's $t$ law is widely used when data exhibit heavier tails than the normal law can capture. In finance, it is
common in return modeling and parametric VaR work because extreme movements occur more frequently than the Gaussian
model predicts. In statistics, it arises naturally when a mean is standardized using an estimated variance rather than a
known variance. This is why classical $t$-tests are based on it.

### Asymptotics

As the degrees of freedom increase,

$$
T\xrightarrow{d}\mathcal{N}(0,1)
\qquad\text{as }\nu\to\infty.
$$

Thus the $t$ family provides a bridge between heavy-tailed models and the Gaussian benchmark.

---

## 5. Fisher information and asymptotic normality of the MLE

### 5.1 Regularity and boundary cases

Before analyzing the score and information, it is important to note that standard asymptotic results for the MLE assume
**regularity conditions**. Crucially:

- The true parameter $\theta_0$ is assumed to lie in the **interior** of the parameter space $\Theta$.
- Asymptotic formulas (like the inverse Fisher information) may fail or require separate treatment if the true parameter
  is on the **boundary** (e.g., Bernoulli $p=0$ or $p=1$, or variance $\sigma^2=0$).

## 5.2 The score function

Let $X$ have density or pmf $f(x;\theta)$, and let

$$
\ell(\theta)=\log L(\theta)
$$

be the log-likelihood based on a sample. The **score** is the derivative of the log-likelihood:

$$
U(\theta)=\frac{\partial \ell(\theta)}{\partial\theta}
$$

in the one-parameter case, or the gradient $\nabla_\theta \ell(\theta)$ in the multiparameter case.

The score measures how sensitive the log-likelihood is to small changes in the parameter. At an interior MLE, one
typically has

$$
U(\hat\theta)=0.
$$

## 5.2 What is Fisher information?

The **Fisher information** measures how much information an observation carries about an unknown parameter.
Roughly speaking, it quantifies how sharply the likelihood is peaked around the true parameter value.
A model with large Fisher information allows more precise estimation;
a model with small Fisher information makes the parameter harder to estimate accurately.

For one observation, the Fisher information is defined by

$$
I(\theta)=\mathbb{E}\left[\left(\frac{\partial}{\partial\theta}\log f(X;\theta)\right)^2\right],
$$

provided the expectation exists and regularity conditions hold.

Under standard regularity conditions, it can also be written as

$$
I(\theta)=-\mathbb{E}\left[\frac{\partial^2}{\partial\theta^2}\log f(X;\theta)\right].
$$

For an i.i.d. sample of size $n$, Fisher information adds:

$$
I_n(\theta)=nI(\theta).
$$

This additivity is one reason why estimator precision typically improves at the rate $1/\sqrt{n}$.

## 5.3 Intuition

If changing $\theta$ slightly causes a large change in the likelihood, then the data are informative about $\theta$, so
Fisher information is large. If the likelihood hardly changes when $\theta$ changes, then many values of $\theta$ fit
the data similarly well, and Fisher information is small.

A useful informal interpretation is:

- **high Fisher information** $\Rightarrow$ parameter is easier to estimate precisely,
- **low Fisher information** $\Rightarrow$ parameter is harder to estimate precisely.

## 5.4 Cramér-Rao lower bound

Fisher information is closely connected to the fundamental lower bound on estimator variance. Under suitable conditions,
any unbiased estimator $T$ of $\theta$ satisfies

$$
\mathrm{Var}(T)\ge \frac{1}{nI(\theta)}.
$$

This is the **Cramér-Rao lower bound**. It says that no unbiased estimator can have variance below the inverse
information. The larger the information, the lower the best achievable variance.

## 5.5 Asymptotic normality of the MLE

Under standard regularity conditions and if the true parameter is $\theta_0$,

$$
\sqrt{n}(\hat\theta-\theta_0)
\xrightarrow{d}
\mathcal{N}\left(0,\frac{1}{I(\theta_0)}\right)
$$

in the one-parameter case.

Equivalently,

$$
\hat\theta\approx \mathcal{N}\left(\theta_0,\frac{1}{nI(\theta_0)}\right)
$$

for large $n$.

This result explains why MLEs are often approximately normal in large samples and why Fisher information governs their
asymptotic precision.

## 5.6 Observed information

In applications, the true Fisher information is usually unknown because it depends on the unknown parameter. A common
practical substitute is the **observed information**, defined by

$$
J_n(\theta)=-\frac{\partial^2\ell(\theta)}{\partial\theta^2}.
$$

Evaluated at the MLE, it is often used to estimate the asymptotic variance:

$$
\widehat{\mathrm{Var}}(\hat\theta)\approx J_n(\hat\theta)^{-1}
$$

in the one-parameter case, or the inverse Hessian in the multiparameter case.

## 5.7 Simple examples of Fisher information

### Bernoulli$(p)$

For one observation,

$$
\log f(X;p)=X\log p+(1-X)\log(1-p).
$$

Differentiating gives

$$
\frac{\partial}{\partial p}\log f(X;p)=\frac{X}{p}-\frac{1-X}{1-p}.
$$

Hence the Fisher information for one observation is

$$
I(p)=\frac{1}{p(1-p)}.
$$

So for $n$ i.i.d. observations,

$$
I_n(p)=\frac{n}{p(1-p)}.
$$

This shows that estimation is hardest when $p$ is near $0$ or $1$ in the sense that very asymmetric samples can carry
less local variability information than those around the middle, though boundary effects also complicate inference.

### Poisson$(\lambda)$

For one observation,

$$
\log f(X;\lambda)=-\lambda+X\log\lambda-\log(X!).
$$

Differentiating twice yields

$$
I(\lambda)=\frac{1}{\lambda}.
$$

Thus for $n$ observations,

$$
I_n(\lambda)=\frac{n}{\lambda}.
$$

This implies the asymptotic variance of the MLE $\hat\lambda=\bar X$ is approximately $\lambda/n$, which matches the
exact variance of the sample mean.

### Exponential$(\lambda)$

For one observation,

$$
\log f(X;\lambda)=\log\lambda-\lambda X.
$$

Then

$$
I(\lambda)=\frac{1}{\lambda^2}.
$$

Hence

$$
I_n(\lambda)=\frac{n}{\lambda^2},
$$

and the asymptotic variance of the MLE is $\lambda^2/n$.

---

## 6. Why characteristic functions matter

The **characteristic function** of a real-valued random variable $X$ is defined by

$$
\varphi_X(t)=\mathbb{E}[e^{itX}],
\qquad t\in\mathbb{R}.
$$

It always exists because $|e^{itX}|=1$.

### 6.1 Why characteristic functions are useful

Characteristic functions are central for both theory and applications:

- They **uniquely determine the distribution** of $X$.
- They behave particularly well under sums of independent variables.
- They are the natural objects in Fourier-analytic pricing methods.
- They are often easier to manipulate than densities, especially for limiting arguments.

If $X$ and $Y$ are independent, then

$$
\varphi_{X+Y}(t)=\varphi_X(t)\varphi_Y(t).
$$

This multiplicative property is one of the main reasons characteristic functions are so powerful.

### 6.2 Link with densities

If $X$ has density $f_X$, then

$$
\varphi_X(t)=\int_{-\infty}^{\infty} e^{itx}f_X(x)\,dx.
$$

So the characteristic function is the Fourier transform of the density under the standard probability convention.

### 6.3 Why they matter in asymptotics

Characteristic functions are especially useful for proving convergence in distribution. Many classical limit theorems,
including versions of the Central Limit Theorem, are proved by showing that the characteristic functions converge
pointwise to the characteristic function of the limiting law.

### 6.4 Why they matter in finance

In option pricing and affine or Lévy models, one often works with transforms rather than densities directly. If the
characteristic function of log-prices is known explicitly, one can recover prices or distributions numerically through
Fourier inversion. This is one reason characteristic functions are much more than a purely theoretical tool.

---

## 7. Worked examples

## Example 1: Bernoulli default model

For each issuer $i$, define the default indicator

$$
X_i=
\begin{cases}
1, & \text{if issuer } i \text{ defaults during the horizon},\\
0, & \text{otherwise}.
\end{cases}
$$

If all issuers are assumed independent with the same default probability $p$, then

$$
X_i\sim\mathrm{Bernoulli}(p),
\qquad i=1,\dots,m,
$$

and the total number of defaults is

$$
N=\sum_{i=1}^m X_i.
$$

Therefore,

$$
N\sim\mathrm{Binomial}(m,p).
$$

This is the simplest portfolio default-count model. It is easy to analyze, but it ignores heterogeneity across obligors
and dependence between defaults. More advanced portfolio credit models enrich this Bernoulli building block by allowing
varying probabilities, factor dependence, or stochastic intensities.

## Example 2: Waiting time to a jump

Suppose jumps arrive according to a Poisson process with intensity $\lambda > 0$. Let $T$ denote the waiting time to the
first jump. Then

$$
T\sim\mathrm{Exponential}(\lambda).
$$

This reflects a fundamental connection:

- Poisson laws describe **how many** events occur over a time interval,
- Exponential laws describe **how long one waits** until the next event.

This count/waiting-time duality is one of the most important structural links in stochastic-process modeling.

## Example 3: Equity price model

In the Black-Scholes model,

$$
S_T=S_0\exp\left(\left(r-\frac{1}{2}\sigma^2\right)T+\sigma W_T\right),
$$

where $W_T\sim\mathcal{N}(0,T)$. Hence

$$
\log S_T
=\log S_0+\left(r-\frac{1}{2}\sigma^2\right)T+\sigma W_T
$$

is Gaussian, which means that $S_T$ is lognormal. This explains why the lognormal distribution appears naturally in
simple diffusion-based equity models.

## Example 4: Recovery-rate modeling

Suppose a recovery rate $R$ is measured as a fraction of par value recovered after default, so that always

$$
0 \le R \le 1.
$$

A Beta distribution is often a reasonable parametric choice because it respects the support automatically and can
represent many shapes: concentrated around a central value, skewed, or even bimodal-like near the edges when parameters
are below one. This makes it much better suited than an unbounded Gaussian model for bounded fractions.

---

## 8. Summary table

| Distribution  | Type       | Parameters                                   | Support           | Mean                    | Variance                                         | Characteristic function           | Typical use                                   | Large-sample / limit behavior                                  |
|---------------|------------|----------------------------------------------|-------------------|-------------------------|--------------------------------------------------|-----------------------------------|-----------------------------------------------|----------------------------------------------------------------|
| Bernoulli     | Discrete   | $0 \le p \le 1$                              | $\{0,1\}$         | $p$                     | $p(1-p)$                                         | $(1-p)+pe^{it}$                   | Single binary event or default indicator      | Sample mean $\to p$; CLT for estimator                         |
| Binomial      | Discrete   | $m\in\mathbb{N}, m \ge 1$, $0 \le p \le 1$   | $\{0,\dots,m\}$   | $mp$                    | $mp(1-p)$                                        | $((1-p)+pe^{it})^m$               | Number of successes in $m$ trials             | Standardized form $\to$ Normal; rare-event limit $\to$ Poisson |
| Poisson       | Discrete   | $\lambda > 0$                                | $\{0,1,2,\dots\}$ | $\lambda$               | $\lambda$                                        | $\exp(\lambda(e^{it}-1))$         | Counts of arrivals, incidents, jumps          | For large $\lambda$, approximately Normal                      |
| Geometric     | Discrete   | $0 < p \le 1$                                | $\{1,2,\dots\}$   | $1/p$                   | $(1-p)/p^2$                                      | $pe^{it}/(1-(1-p)e^{it})$         | Discrete waiting time to first event          | Suitable rescalings $\to$ Exponential                          |
| Uniform       | Continuous | $a < b$                                      | $[a,b]$           | $(a+b)/2$               | $(b-a)^2/12$                                     | $(e^{itb}-e^{ita})/(it(b-a))$     | Bounded uncertainty, simulation primitive     | LLN and CLT for sample mean                                    |
| Exponential   | Continuous | $\lambda > 0$                                | $[0,\infty)$      | $1/\lambda$             | $1/\lambda^2$                                    | $\lambda/(\lambda-it)$            | Waiting times, constant-intensity models      | MLE asymptotically Normal                                      |
| Normal        | Continuous | $\mu\in\mathbb{R}$, $\sigma > 0$             | $\mathbb{R}$      | $\mu$                   | $\sigma^2$                                       | $\exp(it\mu-0.5\sigma^2 t^2)$     | Errors, shocks, asymptotic approximations     | Stable under sums; CLT limit                                   |
| Lognormal     | Continuous | $\mu\in\mathbb{R}$, $\sigma > 0$ on $\log X$ | $(0,\infty)$      | $e^{\mu+\sigma^2/2}$    | $(e^{\sigma^2}-1)e^{2\mu+\sigma^2}$              | No simple elementary closed form  | Positive prices and multiplicative effects    | Inference often done on log scale                              |
| Gamma         | Continuous | $\alpha > 0$, $\beta > 0$                    | $(0,\infty)$      | $\alpha/\beta$          | $\alpha/\beta^2$                                 | $(1-it/\beta)^{-\alpha}$          | Positive skewed quantities, waiting-time sums | Approx. Normal for large shape                                 |
| Beta          | Continuous | $\alpha > 0$, $\beta > 0$                    | $(0,1)$           | $\alpha/(\alpha+\beta)$ | $\alpha\beta/((\alpha+\beta)^2(\alpha+\beta+1))$ | ${}_1F_1(\alpha;\alpha+\beta;it)$ | Recovery rates, bounded proportions           | Concentrates near mean as $\alpha+\beta$ grows                 |
| Chi-square    | Continuous | $k\in\mathbb{N}, k \ge 1$                    | $[0,\infty)$      | $k$                     | $2k$                                             | $(1-2it)^{-k/2}$                  | Variance inference, quadratic diagnostics     | Standardized form $\to$ Normal                                 |
| Student's $t$ | Continuous | $\nu > 0$                                    | $\mathbb{R}$      | $0$ if $\nu > 1$        | $\nu/(\nu-2)$ if $\nu > 2$                       | Bessel-function form              | Heavy-tailed returns, robust inference        | $\to$ Normal as $\nu\to\infty$                                 |

---

## 9. Final remarks

These distributions are not isolated formulas; they are building blocks for:

- Monte Carlo simulation,
- risk-factor and portfolio models,
- default and intensity-based models,
- pricing under uncertainty,
- Value-at-Risk and Expected Shortfall calculations,
- statistical calibration and model validation,
- large-sample inference.

A useful way to organize them conceptually is the following:

- **Bernoulli, Binomial, Poisson, Geometric**: event indicators, event counts, and waiting times in discrete settings;
- **Exponential, Gamma**: waiting times and positive durations in continuous settings;
- **Normal, Student's $t$**: centered shocks and standardized fluctuations, with the $t$ family allowing heavier tails;
- **Lognormal, Gamma, Beta**: positive or bounded variables where support matters;
- **Chi-square**: quadratic forms and classical inference for variances and likelihood-ratio statistics.

The next natural topics after this chapter are:

1. the Law of Large Numbers,
2. the Central Limit Theorem,
3. convergence in probability, distribution, and almost surely,
4. stochastic processes and Poisson processes,
5. Brownian motion and Itô calculus,
6. transform methods and Fourier pricing.

If you want this chapter to read even more like a textbook, the next step would be to add short derivations of the means
and variances, plus a small section on moment-generating functions and probability-generating functions.

