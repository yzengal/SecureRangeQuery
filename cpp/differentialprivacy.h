#ifndef GRPC_COMMON_CPP_DP_H_
#define GRPC_COMMON_CPP_DP_H_

#include <algorithm>
#include <vector>
#include <random>
#include <cmath>
#include <utility>


namespace DIFFERENTIALPRIVACY {
constexpr float SPATIAL_DP_EPSILON = 1.0;

// classic mechanism in differential privacy
//
double LaplaceMechanism(float sensitivity, float epsilon);

// basic mechanism to perturb locations by using Laplace twice
//
std::pair<double,double> TwiceLaplaceMechanism(float x, float y, float sensitivity_x, float sensitivity_y, float epsilon=0.5);

// existing mechanism to perturb locations by using Plannar Laplace
//
std::pair<double,double> PlanarLaplaceMechanism(float x, float y, float epsilon=0.5);

// basic function to sample a radius based on Geo-I mechanism
//
double GeoI_SampleRad(double epsilon, double prob);

// basic function to sample a theta based on Geo-I mechanism
//
double GeoI_SampleTheta(double ep, double prob);

// our mechanism to perturb locations by using Bounded Plannar Laplace
//
std::pair<double,double> BoundedPlanarLaplaceMechanism(float x, float y, float epsilon=0.5, float delta=1e-4);

}  // namespace DIFFERENTIALPRIVACY

#endif  // GRPC_COMMON_CPP_DP_H_
