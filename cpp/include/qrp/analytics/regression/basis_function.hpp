#pragma once

// Declares basis functions used to expand state variables for regression fitting.

#include <cmath>
#include <vector>

namespace qrp::analytics::regression {

/**
 * @brief Interface for expanding state variables into regression features.
 */
class BasisFunction {
public:
    /**
     * @brief Allows deletion through the basis-function base type.
     */
    virtual ~BasisFunction() = default;

    /**
     * @brief Evaluates the basis for one state vector.
     */
    virtual std::vector<double> evaluate(const std::vector<double>& x) const = 0;
};

/**
 * @brief Polynomial basis with a constant term and per-variable powers.
 */
class PolynomialBasis : public BasisFunction {
public:
    /**
     * @brief Creates a polynomial basis up to the supplied degree.
     */
    explicit PolynomialBasis(int degree) : degree_(degree) {}

    /**
     * @brief Evaluates constant and univariate polynomial features.
     */
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
    int degree_; // Maximum univariate polynomial degree.
};

} // namespace qrp::analytics::regression
