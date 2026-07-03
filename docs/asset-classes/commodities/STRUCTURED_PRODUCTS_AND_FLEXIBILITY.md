# Structured Products and Flexibility

## Plain vanilla vs structured/physical

Plain vanilla products:

- futures;
- forwards;
- standard options.

Structured/physical products:

- gas storage;
- swing contracts;
- CCGT/gas plant;
- hydro reservoir;
- battery;
- PPA;
- structured supply contract;
- virtual power plant.

The main difference:

```text
Plain vanilla = mostly market curve and option pricing.
Structured/physical = value of future decisions under uncertainty and constraints.
```

## Flexibility

Flexibility means the right or ability to choose future actions.

Examples:

- inject or withdraw gas;
- run or stop a gas plant;
- charge or discharge a battery;
- generate with hydro now or later;
- take more or less volume under a swing contract.

## Generic value function

$$
V_t(x_t) =
\max_{a_t \in A(x_t)}
\left[
CF_t(a_t) +
\mathbb{E}_t
\left[
D_{t,t+1}V_{t+1}(x_{t+1})
\right]
\right]
$$

where:

- $x_t$ is the state;
- $a_t$ is the action;
- $A(x_t)$ is the feasible action set;
- $CF_t$ is cashflow;
- $D$ is discount factor.

## Gas storage

State:

$$
x_t = \text{inventory level}
$$

Actions:

```text
Inject, withdraw, hold.
```

Inventory dynamics:

$$
x_{t+1} =
x_t +
\eta_{\text{inj}}q_t^+ -
\frac{1}{\eta_{\text{wd}}}q_t^-
$$

Constraints:

$$
x_{\min} \leq x_t \leq x_{\max}
$$

$$
0 \leq q_t^+ \leq q_{\max}^{inj}(x_t)
$$

$$
0 \leq q_t^- \leq q_{\max}^{wd}(x_t)
$$

## Swing contract

A swing contract allows variable volumes within limits.

Decision:

$$
q_t \in [q_{\min}, q_{\max}]
$$

Total volume constraint:

$$
Q_{\min} \leq
\sum_t q_t \leq
Q_{\max}
$$

Value:

$$
V_t(S_t,R_t) =
\max_{q_t}
\left[
q_t(S_t-K) +
\mathbb{E}_t
\left[
D V_{t+1}(S_{t+1},R_t-q_t)
\right]
\right]
$$

## CCGT / gas plant

Economic driver:

$$
CSS =
P_{\text{power}} -
hP_{\text{gas}} -
eP_{\text{CO2}} -
VOM
$$

Simple dispatch rule:

$$
\text{Run if } CSS > 0
$$

Real plants also have:

- start-up costs;
- ramping constraints;
- minimum up/down times;
- outages;
- efficiency curves.

## Hydro reservoir

State:

```text
Reservoir level, inflow, time, power price.
```

Decision:

```text
Generate now or save water.
```

Value comes from time-shifting water to higher-value hours/seasons.

## Battery

State:

```text
State of charge.
```

Decision:

```text
Charge, discharge, hold.
```

Constraints:

- capacity;
- charging power;
- discharging power;
- round-trip efficiency;
- degradation cost.

## PPA

Power Purchase Agreement.

Types:

- fixed-price PPA;
- floating/indexed PPA;
- pay-as-produced renewable PPA;
- baseload PPA;
- shaped PPA.

Main risks:

- price risk;
- volume risk;
- profile risk;
- imbalance risk;
- counterparty risk.

## Least Squares Monte Carlo for flexibility

Continuation value:

$$
C_t(X_t) =
\mathbb{E}
\left[
D_{t,t+1}V_{t+1}(X_{t+1})
\mid X_t
\right]
$$

Approximation:

$$
C_t(X_t) \approx
\sum_{k=1}^{K} a_k \phi_k(X_t)
$$

State variables can include:

```text
Price, spread, inventory, remaining volume, time, forward-curve factors.
```

## Method Selection

Structured energy products are valued as flexibility under uncertainty and operational constraints. The numerical method
depends on dimensionality, exercise structure, and constraint detail:

- dynamic programming is natural for low-dimensional state spaces;
- LSMC or scenario-based stochastic optimization handles higher-dimensional flexible assets;
- mixed-integer optimization is useful when detailed dispatch constraints dominate;
- analytic approximations are useful only when the product can be reduced to a simple spread or option payoff.

The implementation should make state variables, actions, constraints, and exercise policy explicit, because these define
the economic product more precisely than the product label alone.
