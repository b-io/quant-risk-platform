#pragma once

// Declares stochastic-process abstractions and sampled market paths.

#include <qrp/analytics/simulation/time_grid.hpp>
#include <vector>
#include <random>

namespace qrp::analytics::simulation {

class MarketPath {
public:
    MarketPath(std::size_t num_steps, std::size_t dimension)
        : data_(num_steps, std::vector<double>(dimension)) {}

    std::size_t num_steps() const { return data_.size(); }
    std::size_t dimension() const { return data_.empty() ? 0 : data_[0].size(); }

    double& operator()(std::size_t time_index, std::size_t dim_index) {
        return data_[time_index][dim_index];
    }

    double operator()(std::size_t time_index, std::size_t dim_index) const {
        return data_[time_index][dim_index];
    }

    const std::vector<double>& at(std::size_t time_index) const {
        return data_.at(time_index);
    }

private:
    std::vector<std::vector<double>> data_;
};

class StochasticProcess {
public:
    virtual ~StochasticProcess() = default;
    virtual std::size_t dimension() const = 0;
    virtual void simulatePath(
        const TimeGrid& timeGrid,
        std::mt19937& gen,
        MarketPath& outputPath
    ) const = 0;
};

} // namespace qrp::analytics::simulation
