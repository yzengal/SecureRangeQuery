#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <memory>
#include <mutex>
#include <thread>
#include <random>

#include "global.h"
#include "grid.hpp"

using namespace std;
using ICDE18::Record_t;
using ICDE18::Circle_t;

bool IntersectWith(Record_t& a, Circle_t& b) {
    std::cout << "In this function" << std::endl;
    fflush(stdout);
    Record_t center;
    center.ID = 0;
    center.x = b.x;
    center.y = b.y;
    return (center.x-b.x)*(center.x-b.x)+(center.y-b.y)*(center.y-b.y) <= b.rad*b.rad;
}

vector<ICDE18::Record_t> alldata;
ICDE18::Circle_t circ;

int main(int argc, char** argv) {
    std::default_random_engine gen;
    std::uniform_int_distribution<> distrib(-100, 100);

    const int N = 500;

    alldata.resize(N);
    cout << "alldata:" << endl;
    cout << alldata.size() << endl;
    for (int i=0; i<N; ++i) {
        alldata[i].ID = i;
        alldata[i].x = distrib(gen);
        alldata[i].y = distrib(gen);
        cout << alldata[i].ID << " " << alldata[i].x << " " << alldata[i].y << endl;
        fflush(stdout);
    }

    circ.x = distrib(gen);
    circ.y = distrib(gen);
    circ.rad = 60;

    cout << "circ" << endl;
    cout << circ.x << " " << circ.y << " " << circ.rad << endl;

    vector<int> ans, res;
    for (int i=0; i<N; ++i) {
        // cout << alldata[i].ID << " " << alldata[i].x << " " << alldata[i].y << endl;
        // cout.flush();
        if (ICDE18::IntersectWithRange(alldata[i], circ)) {
            ans.emplace_back(alldata[i].ID);
        }
    }

    std::shared_ptr<std::vector<ICDE18::Record_t>> ptr = std::make_shared<std::vector<ICDE18::Record_t>>(alldata);
    INDEX::GridIndex<5> grid(ptr);
    vector<ICDE18::Record_t> tmp = grid.range_query(circ);
    for (auto rec : tmp) {
        res.emplace_back(rec.ID);
    }
    sort(res.begin(), res.end());

    cout << "----------------------------------" << endl;
    cout << ans.size() << " " << res.size() << endl;
    assert(ans.size() == res.size());

    for (int i=0; i<ans.size(); ++i)
        cout << i << ": " << ans[i] << " " << res[i] << endl;

    return 0;
}
