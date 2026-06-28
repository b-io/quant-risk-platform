#pragma once

// Declares geometric Brownian motion process dynamics for simulation examples and tests.

#include <qrp/analytics/simulation/stochastic_process.hpp>
#include <cmath>

namespace qrp::analytics::simulation {

/**
 * @brief One-dimensional geometric Brownian motion process.
 */
class GeometricBrownianMotion : public StochasticProcess {
public:
    /**
     * @brief Creates a GBM process from initial value, drift, and volatility.
     */
    GeometricBrownianMotion(double initial_value, double drift, double volatility)
        : initial_value_(initial_value), drift_(drift), volatility_(volatility) {}

    /**
     * @brief Returns the single simulated state dimension.
     */
    std::size_t dimension() const override { return 1; }

    /**
     * @brief Simulates one GBM path on the supplied time grid.
     */
    void simulatePath(
        const TimeGrid& timeGrid,
        std::mt19937& gen,
        MarketPath& outputPath
    ) const override {
        std::normal_distribution<double> dist(0.0, 1.0);
        double current_val = initial_value_;
        outputPath(0, 0) = current_val;

        for (std::size_t i = 1; i < timeGrid.size(); ++i) {
            double dt = timeGrid.dt(i);
            double dw = dist(gen) * std::sqrt(dt);
            current_val *= std::exp((drift_ - 0.5 * volatility_ * volatility_) * dt + volatility_ * dw);
            outputPath(i, 0) = current_val;
        }
    }

private:
    double initial_value_;
    double drift_;
    double volatility_;
};

} // namespace qrp::analytics::simulation
