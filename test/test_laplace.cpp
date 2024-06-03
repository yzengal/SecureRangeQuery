#include <bits/stdc++.h>
using namespace std;

int sign(double u) {
    if (u > 0) return 1;
    if (u < 0) return 1;
    return 0;
}

float LaplaceMechanism(float sensitivity, float epsilon) {
    std::random_device rd;
    std::mt19937 generator(rd());
    std::uniform_real_distribution<float> uniform(0.0, 1.0);
    float scale = sensitivity/epsilon;

    std::exponential_distribution<float> distribution1(1.0/scale);
    std::exponential_distribution<float> distribution2(1.0/scale);
    float e1 = distribution1(generator);
    float e2 = distribution2(generator);
    return floor(e1-e2);

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

int main() {
    const int N = 100;
    ofstream fout;

    fout.open("log");
    vector<float> vec;
    float sensitivity = 1, epsilon = 0.01;

    for (int i=0; i<N; ++i) {
        vec.emplace_back(LaplaceMechanism(sensitivity, epsilon));
    }

    fout << N << endl;
    for (int i=0; i<N; ++i) {
        if (i > 0) fout << " ";
        fout << fixed << setprecision(7) << vec[i];
    }
    fout << endl;

    float dev = 0;
    for (int i=0; i<N; ++i) {
        dev += vec[i]*vec[i];
    }
    dev /= N;
    float lambda = sensitivity / epsilon;
    fout << fixed << setprecision(7) << dev << " " << (2.0*lambda*lambda) << endl;

    fout.close();
    return 0;
}