#ifndef GRPC_COMMON_CPP_ICDE18_GLOBAL_H_
#define GRPC_COMMON_CPP_ICDE18_GLOBAL_H_

#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <memory>
#include <string>

#include "ICDE18.grpc.pb.h"

//#define LOCAL_DEBUG
#define GRID_NUM_PER_SIDE 10

using ICDE18::Point;
using ICDE18::Rectangle;
using ICDE18::Circle;
using ICDE18::CircleQueryRange;
using ICDE18::RectangleQueryRange;
using ICDE18::Record;
using ICDE18::RecordSummary;
using ICDE18::IntVector;
using ICDE18::FloatVector;
using std::chrono::system_clock;

namespace ICDE18 {

enum QueryType_t {
    UNDEFINED,
    RANGE_QUERY,
    RANGE_COUNT,
    DISTANCE_JOIN,
};

struct Rectangle_t {
    QueryType_t qtype;
    float x, y;
    float dx, dy;
};

struct Point_t {
    float x, y;

    Point_t() {}
    Point_t(float x_, float y_): x(x_), y(y_) {}

    Point_t& operator=(const Point_t& other) {  
        if (this != &other) {
            x = other.x;
            y = other.y;
        }  
        return *this;  
    }  

    void Print() const {
        printf("x = %.2lf, y = %.2lf\n", x, y);
        fflush(stdout);
    }
};

struct Circle_t {
    QueryType_t qtype;
    float x, y;
    float rad;

    Circle_t() {}
    Circle_t(QueryType_t qtype_, float x_, float y_, float rad_) :
        qtype(qtype_), x(x_), y(y_), rad(rad_) {}

    Circle_t& operator=(const Circle_t& other) {  
        if (this != &other) {
            qtype = other.qtype;
            x = other.x;
            y = other.y;
            rad = other.rad;  
        }  
        return *this;  
    }  

    void Print() const {
        if (qtype == QueryType_t::RANGE_QUERY)
            printf("type = RANGE_QUERY, ");
        else if (qtype == QueryType_t::RANGE_COUNT)
            printf("type = RANGE_COUNT, ");
        else
            printf("type = UNDEFINED, ");
        printf("x = %.2lf, y = %.2lf, rad = %.2lf\n", x, y, rad);
        fflush(stdout);
    }
};

struct Record_t {
    int ID;
    float x, y;

    Record_t(int _ID=-1, float _x=0., float _y=0.): ID(_ID), x(_x), y(_y) {}

    Record_t& operator=(const Record_t& other) {  
        if (this != &other) {
            ID = other.ID;  
            x = other.x;
            y = other.y;
        }  
        return *this;  
    }  

    bool operator==(const Record_t& other) const {
        return (ID==other.ID) && (x==other.x) && (y==other.y);  
    }  

    bool operator!=(const Record_t& other) const {  
        return (ID!=other.ID) || (x!=other.x) || (y!=other.y); 
    }  

    void Print() const {
        printf("ID = %d, x = %.2lf, y = %.2lf\n", ID, x, y);
        fflush(stdout);
    }
};

// process spatial basic function
//
float GetDistance(const Point_t& a, const Point_t& b);
float GetDistance(const Point& a, const Point& b);
float GetDistance(const Record_t& a, const Point& b);
float GetDistance(const Record_t& a, const Record_t& b);
float GetSquareDistance(const Point& a, const Point& b);
float GetSquareDistance(const Record_t& a, const Point& b);
float GetSquareDistance(const Record_t& a, const Record_t& b);
float GetSquareDistance(const Point_t& a, const Point_t& b);
bool IntersectWithRange(const Point& a, const Rectangle& b);
bool IntersectWithRange(const Point& a, const Circle& b);
bool IntersectWithRange(const Record_t& a, const Rectangle& b);
bool IntersectWithRange(const Record_t& a, const Circle& b);
bool IntersectWithRange(const Record_t& a, const Rectangle_t& b);
bool IntersectWithRange(const Record_t& a, const Circle_t& b);

// Serialize & De-serialize
std::vector<unsigned char> SerializeRecord(const Record_t& rec);
Record_t DeserializeRecord(const std::vector<unsigned char>& charVector);


// process vector basic function
// void CopyFromVector(IntVector& des, const std::vector<int>& des);
// void CopyFromVector(FloatVector& des, const std::vector<float>& des);
template <typename T>
void CopyFromVector(IntVector& des, const std::vector<T>& src) {
    des.set_size(src.size());
    des.clear_values();
    for (T v : src) {
        des.add_values((int) v);
    }
}

template <typename T>
void CopyFromVector(FloatVector& des, const std::vector<T>& src) {
    des.set_size(src.size());
    des.clear_values();
    for (T v : src) {
        des.add_values((float) v);
    }    
}

template <typename T>
void CopyToVector(std::vector<T>& des, const IntVector& src) {
    des.clear();
    for (size_t i=0,sz=src.size(); i<sz; ++i)
        des.emplace_back((T) src.values(i));
}

template <typename T>
void CopyToVector(std::vector<T>& des, const FloatVector& src) {
    des.clear();
    for (size_t i=0,sz=src.size(); i<sz; ++i)
        des.emplace_back((T) src.values(i)); 
}

// process read and write file
//
//
std::string GetDataFilePath(int argc, char** argv);
std::string GetQueryFilePath(int argc, char** argv);
std::string GetIPAddress(int argc, char** argv);
std::string GetSiloIPFilePath(int argc, char** argv);
int GetSiloID(int argc, char** argv);
void GetInputData(const std::string& fileName, std::vector<Record_t>& recordVector);
QueryType_t GetQueryType(const std::string& str);
void GetInputQuery(const std::string& fileName, std::vector<Rectangle_t>& queries);
void GetInputQuery(const std::string& fileName, std::vector<Circle_t>& queries);
void GetIPAddresses(const std::string& fileName, std::vector<std::string>& ip_addresses);


// process communication cost
//
//
float CommRangeQuery(const Rectangle& a);
float CommRangeQuery(const Circle& a);
float CommQueryAnswer(const std::vector<Record_t>& a);
float CommQueryAnswer(const std::vector<Record>& a);
float CommQueryAnswer(const RecordSummary& a);

class QueryLogger {
public:
    QueryLogger() {
        Init();
    }

    void Init() {
        queryNum = 0;
        queryTime = 0;
        queryComm = 0;    
        startTime = std::chrono::steady_clock::now();   
        endTime = startTime; 
    }

    void SetStartTimer() {
        startTime = std::chrono::steady_clock::now(); 
    }

    void SetEndTimer() {
        endTime = std::chrono::steady_clock::now(); 
    }

    void LogAddComm(float _queryComm=0.0f) {
        queryComm += _queryComm;
    }

    void LogOneQuery(float _queryComm=0.0f) {
        LogAddComm(_queryComm);

        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
        float _queryTime = duration.count();

        queryNum += 1;
        queryTime += _queryTime;

        endTime = startTime;
    }

    void Print() {
        float AvgQueryTime = (queryNum==0) ? 0 : (queryTime/queryNum); // ms
        float AvgQueryComm = (queryNum==0) ? 0 : (queryComm/queryNum); // bytes
        AvgQueryTime /= 1;
        AvgQueryComm /= 1024;
        printf("-------------- Query Log --------------\n");
        printf("QueryNum = %d, AvgQueryTime = %.6f [ms], AvgQueryComm = %.6f [KB]\n\n", queryNum, AvgQueryTime, AvgQueryComm);
        fflush(stdout);
    }

private:
    std::chrono::steady_clock::time_point startTime, endTime;
    int queryNum;
    float queryTime;
    float queryComm;
};

}  // namespace ICDE18

#endif  // GRPC_COMMON_CPP_ICDE18_GLOBAL_H_
