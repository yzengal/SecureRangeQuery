#ifndef GRPC_COMMON_CPP_DP_H_
#define GRPC_COMMON_CPP_DP_H_

#include <algorithm>
#include <vector>
#include <random>
#include <cmath>


namespace DIFFERENTIALPRIVACY {


// classic mechanism in differential privacy
//
float LaplaceMechanism(float sensitivity, float epsilon);



}  // namespace DIFFERENTIALPRIVACY

#endif  // GRPC_COMMON_CPP_DP_H_
