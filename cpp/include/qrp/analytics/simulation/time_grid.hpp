#pragma once
#include <vector>
#include <cstddef>

namespace qrp::analytics::simulation {

class TimeGrid {
public:
    TimeGrid() = default;
    explicit TimeGrid(std::vector<double> times) : times_(std::move(times)) {}

    std::size_t size() const { return times_.size(); }
    double operator[](std::size_t i) const { return times_[i]; }
    double at(std::size_t i) const { return times_.at(i); }
    const std::vector<double>& times() const { return times_; }

    double dt(std::size_t i) const {
        if (i == 0) return times_[0];
        return times_[i] - times_[i - 1];
    }

private:
    std::vector<double> times_;
};

} // namespace qrp::analytics::simulation
