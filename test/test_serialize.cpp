#include <bits/stdc++.h>
using namespace std;

struct Record_t {
    int ID;
    float x, y;
    Record_t(int _ID=-1, float _x=0., float _y=0.): ID(_ID), x(_x), y(_y) {}
    void print() {
        printf("ID = %d, x = %.2lf, y = %.2lf\n", ID, x, y);
        fflush(stdout);
    }
};

std::vector<unsigned char> SerializeRecord(const Record_t& rec) {
    const int sz = ((sizeof(Record_t)+15)/16) * 16;
    unsigned char des[sz];
    memcpy(des, &rec, sizeof(unsigned char)*sz);
    return std::vector<unsigned char>(std::begin(des), std::end(des));
}

Record_t DeserializeRecord(const std::vector<unsigned char>& charVector) {
    const int sz = ((sizeof(Record_t)+15)/16) * 16;
    unsigned char des[sz];
    for (size_t i=0; i<charVector.size(); ++i) {
        des[i] = charVector[i];
    }
    Record_t ret;
    memcpy(&ret, des, sizeof(Record_t));
    return ret;
}

int main() {
    Record_t rec;

    for (int i=0; i<10; ++i) {
        rec.ID = rand() % 256;
        rec.x = rand() % 256;
        rec.y = rand() % 256;

        rec.print();
        std::vector<unsigned char> vec = SerializeRecord(rec);
        Record_t tmp = DeserializeRecord(vec);
        tmp.print();
    }

    return 0;
}