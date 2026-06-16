# Analytics Framework - Generic LSMC Engine

This module provides a generic framework for solving optimal decision problems under uncertainty using the Least Squares Monte Carlo (LSMC) method.

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
  - `regressionFeatures`: Basis functions for continuation value estimation.

### 3. Regression (`qrp::analytics::regression`)
- `RegressionModel`: Interface for fitting and prediction (default: `OrdinaryLeastSquares`).
- `BasisFunction`: Extensible basis function generation (e.g., `PolynomialBasis`).

### 4. LSMC Engine (`qrp::analytics::lsmc`)
- `LsmcEngine`: The core backward induction algorithm.
- `LsmcResult`: Contains value, standard error, and risk metrics (VaR, ES).

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
4. Provide relevant `regressionFeatures` (e.g., price, charge level, interactions).
