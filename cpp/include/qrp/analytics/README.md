# Analytics Framework

This module contains the analytics services and reusable model components used by the C++ core and Python bindings.
The current public services include valuation, deterministic risk, stress/HVaR, PnL explain, Monte Carlo simulation,
historical VaR/ES contribution analytics, covariance estimation, and a stateful `RevaluationSession` for repeated quote
updates and scenario revaluation over cached C++ instruments.

The same directory also contains a generic framework for solving optimal decision problems under uncertainty using the
Least Squares Monte Carlo (LSMC) method.

## Service Components

- `ValuationService`: Prices supported portfolio trades from a `PricingContext`.
- `RiskService`: Computes deterministic bump-and-revalue risk from factor bindings.
- `StressEngine`: Replays factor scenarios over the current market state.
- `PnlExplainService`: Reconciles previous/current valuation changes into business components.
- `MonteCarloEngine`: Runs factor-shock simulations in horizon-shock or aged-horizon modes.
- `VarContributionService`: Calculates historical VaR/ES contributions by reporting group.
- `RevaluationSession`: Owns one built market state and instrument cache for repeated quote/scenario revaluation.
  It also exposes opt-in dependency-graph snapshots, impact previews, and candidate-only revaluation diffs for Python
  reporting workflows.

## Key Components

### 1. Simulation (`qrp::analytics::simulation`)
- `TimeGrid`: Defines the discrete points in time.
- `StochasticProcess`: Interface for market models (e.g., `GeometricBrownianMotion`).
- `MarketPath`: Stores simulated market variables over time.

### 2. Decision Problem (`qrp::analytics::dynamic_programming`)
- `DecisionProblem`: Interface to define the logic of the asset/strategy.
  - `feasibleActions`: Returns allowed decisions at a given state and time.
  - `immediateCashflow`: Returns the cashflow for a given action.
  - `nextState`: Defines how the state evolves based on an action and market movement.
  - `isTerminalAction`: Marks actions that stop continuation value, such as option exercise.
  - `regressionFeatures`: Basis functions for continuation value estimation.
- `ExercisePolicy`: Product-level exercise contract for immediate exercise value and continuation features.
- `ExercisePolicyDecisionProblem`: Adapter that maps an exercise policy into the generic decision-problem interface.

### 3. Regression (`qrp::analytics::regression`)
- `RegressionModel`: Interface for fitting and prediction (default: `OrdinaryLeastSquares`).
- `BasisFunction`: Extensible basis function generation (e.g., `PolynomialBasis`).

### 4. LSMC Engine (`qrp::analytics::lsmc`)
- `LsmcEngine`: The core backward induction algorithm.
- `LsmcResult`: Contains value, standard error, exercise-grid times, path values, risk metrics (VaR, ES), basis labels,
  configuration tags, and regression diagnostics.
- `price_american_option`: C++-managed American option helper exposed to Python as `price_american_option_lsmc(...)`.

## Example Usage: American Put Option

```cpp
class AmericanPutProblem : public dynamic_programming::DecisionProblem {
    // Implement feasibleActions, immediateCashflow, etc.
};

// Setup simulation and engine
simulation::TimeGrid grid(times);
simulation::GeometricBrownianMotion gbm(s0, r, vol);
AmericanPutProblem problem(strike);
lsmc::LsmcEngine engine(config);

// Run valuation
auto result = engine.run(grid, gbm, problem, initial_state);
std::cout << "Value: " << result.value << std::endl;
```

## Adding a New Problem
To add a new asset type (e.g., Battery Storage):
1. Inherit from `DecisionProblem`.
2. Define the operational state (e.g., charge level).
3. Implement `nextState` with efficiency losses and constraints.
4. Override `isTerminalAction` for exercise/settlement actions that should not receive continuation value.
5. Provide relevant `regressionFeatures` (e.g., price, charge level, interactions).
