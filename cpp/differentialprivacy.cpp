#include "differentialprivacy.h"

namespace DIFFERENTIALPRIVACY {

double LaplaceMechanism(double sensitivity, double epsilon) {
    std::random_device rd;
    std::mt19937 generator(rd());
    std::uniform_real_distribution<double> uniform(0.0, 1.0);
    double mean = 0, scale = sensitivity/epsilon;

    double u1, u2;
    do {
        u1 = uniform(generator);
        u2 = uniform(generator);
    } while (u1 == 0.0);  // Ensure u1 is not zero to avoid infinity in logarithm

    // Box-Muller transformation
    double z1 = std::sqrt(-2.0 * std::log(u1)) * std::cos(2.0 * M_PI * u2);
    double z2 = std::sqrt(-2.0 * std::log(u1)) * std::sin(2.0 * M_PI * u2);

    // Laplace transformation
    if (z1 >= 0)
        return mean - scale * std::log(1 - z1);
    else
        return mean + scale * std::log(1 + z1);    
}


}  // namespace DIFFERENTIALPRIVACY
