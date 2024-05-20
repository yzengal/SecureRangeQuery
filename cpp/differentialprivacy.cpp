#include "differentialprivacy.h"

namespace DIFFERENTIALPRIVACY {

double LaplaceMechanism(float sensitivity, float epsilon) {
    std::random_device rd;
    std::mt19937 generator(rd());
    //std::uniform_real_distribution<float> uniform(0.0, 1.0);
    double scale = sensitivity/epsilon;

    std::exponential_distribution<double> distribution1(1.0/scale);
    std::exponential_distribution<double> distribution2(1.0/scale);
    double e1 = distribution1(generator);
    double e2 = distribution2(generator);
    return floor(e1 - e2);
    
    // float u1, u2;
    // do {
    //     u1 = uniform(generator);
    //     u2 = uniform(generator);
    // } while (u1 == 0.0);  // Ensure u1 is not zero to avoid infinity in logarithm

    // // Box-Muller transformation
    // float z1 = std::sqrt(-2.0 * std::log(u1)) * std::cos(2.0 * M_PI * u2);
    // float z2 = std::sqrt(-2.0 * std::log(u1)) * std::sin(2.0 * M_PI * u2);

    // // Laplace transformation
    // if (z1 >= 0)
    //     return mean - scale * std::log(1 - z1);
    // else
    //     return mean + scale * std::log(1 + z1);     
}


}  // namespace DIFFERENTIALPRIVACY
