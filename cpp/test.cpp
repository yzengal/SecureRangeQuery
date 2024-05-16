#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <memory>
#include <mutex>
#include <thread>
using namespace std;

class A {
public:
    A() {

    }

    void print(int a) {
        std::lock_guard<std::mutex> guard(print_mutex);
        std::cout << a << std::endl;
    }

    
    void test() {
        const int N = 4;
        std::thread myThread[N];
        for (int i=0; i<N; ++i) {
            myThread[i] = std::thread(&A::print, this, i);
        }
        for (int i=0; i<N; ++i) {
            myThread[i].join();
        }
    }

private:
    std::mutex print_mutex;
};

int main() {
    A a;

    a.test();
    cout << endl;

    return 0;
}