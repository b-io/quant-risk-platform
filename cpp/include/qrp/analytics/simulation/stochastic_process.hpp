#pragma once

// Declares stochastic-process abstractions and sampled market paths.

#include <qrp/analytics/simulation/time_grid.hpp>

#include <random>
#include <vector>

namespace qrp::analytics::simulation {

/**
 * @brief Matrix-like container for one simulated market path.
 */
class MarketPath {
public:
    /**
     * @brief Allocates a path with fixed time steps and dimensions.
     */
    MarketPath(std::size_t num_steps, std::size_t dimension) : data_(num_steps, std::vector<double>(dimension)) {}

    /**
     * @brief Returns the number of simulated time steps.
     */
    std::size_t num_steps() const {
        return data_.size();
    }

    /**
     * @brief Returns the number of simulated state dimensions.
     */
    std::size_t dimension() const {
        return data_.empty() ? 0 : data_[0].size();
    }

    /**
     * @brief Returns a mutable path value by time and dimension.
     */
    double& operator()(std::size_t time_index, std::size_t dim_index) {
        return data_[time_index][dim_index];
    }

    /**
     * @brief Returns a path value by time and dimension.
     */
    double operator()(std::size_t time_index, std::size_t dim_index) const {
        return data_[time_index][dim_index];
    }

    /**
     * @brief Returns all dimension values at one time index.
     */
    const std::vector<double>& at(std::size_t time_index) const {
        return data_.at(time_index);
    }

private:
    std::vector<std::vector<double>> data_; // Path values by time step and state dimension.
};

/**
 * @brief Interface for stochastic processes that can simulate market paths.
 */
class StochasticProcess {
public:
    /**
     * @brief Allows deletion through the stochastic-process base type.
     */
    virtual ~StochasticProcess() = default;

    /**
     * @brief Returns the number of simulated state dimensions.
     */
    virtual std::size_t dimension() const = 0;

    /**
     * @brief Simulates one path on the supplied time grid.
     */
    virtual void simulatePath(const TimeGrid& timeGrid, std::mt19937& gen, MarketPath& outputPath) const = 0;
};

} // namespace qrp::analytics::simulation
