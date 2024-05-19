#include <bits/stdc++.h>
using namespace std;

template <typename T>
void print(int a, T b) {
    cout << (a+b) << endl;
}

template <typename T>
void print(double a, T b) {
    cout << (a*10+b) << endl;
}

template <typename T1, typename T2>
void printX(T1 a, T2 b) {
    cout << (a*10+b) << endl;
}

int main() {
    print<int>(1, 2);
    print<float>(1.0, 2.0);
    return 0;
}
