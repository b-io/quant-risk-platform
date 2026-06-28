#pragma once

// Declares basis functions used to expand state variables for regression fitting.

#include <vector>
#include <cmath>

namespace qrp::analytics::regression {

class BasisFunction {
public:
    virtual ~BasisFunction() = default;
    virtual std::vector<double> evaluate(const std::vector<double>& x) const = 0;
};

class PolynomialBasis : public BasisFunction {
public:
    explicit PolynomialBasis(int degree) : degree_(degree) {}

    std::vector<double> evaluate(const std::vector<double>& x) const override {
        std::vector<double> features;
        features.push_back(1.0); // Constant term
        for (double val : x) {
            double p = 1.0;
            for (int d = 1; d <= degree_; ++d) {
                p *= val;
                features.push_back(p);
            }
        }
        return features;
    }
private:
    int degree_;
};

} // namespace qrp::analytics::regression
