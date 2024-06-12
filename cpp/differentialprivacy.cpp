#include <boost/math/special_functions/lambert_w.hpp>
#include <boost/random.hpp>  
#include <boost/random/laplace_distribution.hpp>  
#include <cmath>
#include <random>

#include "differentialprivacy.h"


namespace DIFFERENTIALPRIVACY {

double LaplaceMechanism(float sensitivity, float epsilon) {
    static boost::random::mt19937 rng;
    const double mu = 0.0;
    double beta = sensitivity / epsilon;
    boost::random::laplace_distribution<double> laplace_dist(mu, beta);
    double sample = laplace_dist(rng);

    return floor(sample);
    // std::random_device rd;
    // std::mt19937 generator(rd());
    // double scale = sensitivity/epsilon;

    // std::exponential_distribution<double> distribution1(1.0/scale);
    // std::exponential_distribution<double> distribution2(1.0/scale);
    // double e1 = distribution1(generator);
    // double e2 = distribution2(generator);
    // return floor(e1 - e2);
}

// basic mechanism to perturb locations by using Laplace twice
//
std::pair<double,double> TwiceLaplaceMechanism(float x, float y, float sensitivity_x, float sensitivity_y, float epsilon) {
    double dx = LaplaceMechanism(sensitivity_x, epsilon*0.5);
    double dy = LaplaceMechanism(sensitivity_y, epsilon*0.5);
    return std::make_pair(x+dx, y+dy);
}

// basic function to sample a radius based on Geo-I mechanism
//
double GeoI_SampleRad(double epsilon, double prob) {
    double w0 = (prob - 1.0) / std::exp(1.0);
    double w1 = boost::math::lambert_wm1(w0);
    return -1.0 * (w1 + 1.0) / epsilon;
}


// existing mechanism to perturb locations by using Plannar Laplace
//
std::pair<double,double> PlanarLaplaceMechanism(float x, float y, float epsilon) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> theta_distribution(0, 2*M_PI);
    std::uniform_real_distribution<> prob_distribution(0, 1);

    double theta = theta_distribution(gen);
    double prob = prob_distribution(gen);
    double r = GeoI_SampleRad(epsilon, prob);
    double dx = r * std::cos(theta);
    double dy = r * std::sin(theta);

    return std::make_pair(x+dx, y+dy);
}

// our mechanism to perturb locations by using Bounded Plannar Laplace
//
std::pair<double,double> BoundedPlanarLaplaceMechanism(float x, float y, float epsilon, float delta) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> theta_distribution(0, 2 * M_PI);
    std::uniform_real_distribution<> prob_distribution(0, 1);

    const double step_delta = 1e-3;
    double theta = theta_distribution(gen);
    double prob = prob_distribution(gen);
    double big_delta = 0;

    while (true) {
        big_delta += step_delta;
        double right = delta * M_PI * std::pow(GeoI_SampleRad(epsilon, 1-big_delta), 2);
        if (big_delta >= right)
            break;
    }

    double r;
    if (prob > 1 - big_delta) {
        std::uniform_real_distribution<> r_dist(0, std::pow(GeoI_SampleRad(epsilon, 1-big_delta), 2));
        r = std::sqrt(r_dist(gen));
    } else {
        r = GeoI_SampleRad(epsilon, prob);
    }

    double dx = r * std::cos(theta);
    double dy = r * std::sin(theta);

    return std::make_pair(x+dx, y+dy);
}

}  // namespace DIFFERENTIALPRIVACY
