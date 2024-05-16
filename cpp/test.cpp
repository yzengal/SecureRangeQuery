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
        std::thread myThread[10];
        for (int i=0; i<10; ++i) {
            myThread[i].(print, this, i);
        }
    }

    void print(int a) {
        std::lock_guard<std::mutex> guard(print_mutex);
        std::cout << a << std::endl;
    }

private:
    std::mutex print_mutex;
};

int main() {
    A a();
    return 0;
}