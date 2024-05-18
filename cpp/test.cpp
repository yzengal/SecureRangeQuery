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

    void print(const shared_ptr<int> p1) {
        p2 = p1;
        std::cout << *p2 << std::endl;
        std::cout << p2.use_count() << std::endl;
    }

private:
    std::shared_ptr<int> p2;
};

int main() {
    A a;

    shared_ptr<int> p1(new int(200));

    a.print(p1);

    return 0;
}

// step1. create spatial index: grid index
// step2. add laplace noise in each grid
// step3. each silo send the grid index to server
// step4. server computes the intersected grids and send their IDs to each silo
// step5. server sends the encrpted query point and radius to the server
// step6. silo computes the encrpted distance, and put it into a polynoimal term, along with the radius
// step7. silo sends them to the server, server decrypts, filter, and send them back.
// step8. silo gets the filter result, and send the encrypt data to server,
// step9. server collect the data, and send them to client
// step10. client receive the encrypted data, decrypted them, and filter out those with dummy ID