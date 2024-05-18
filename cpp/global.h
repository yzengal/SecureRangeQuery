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

using ICDE18::Point;
using ICDE18::Rectangle;
using ICDE18::Circle;
using ICDE18::CircleQueryRange;
using ICDE18::RectangleQueryRange;
using ICDE18::Record;
using ICDE18::RecordSummary;
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

struct Circle_t {
    QueryType_t qtype;
    float x, y;
    float rad;
};

struct Record_t {
    int ID;
    float x, y;
};

// process spatial basic function
//
double GetDistance(const Point& a, const Point& b);
double GetDistance(const Record_t& a, const Point& b);
double GetDistance(const Record_t& a, const Record_t& b);
double GetSquareDistance(const Point& a, const Point& b);
double GetSquareDistance(const Record_t& a, const Point& b);
double GetSquareDistance(const Record_t& a, const Record_t& b);
bool IntersectWithRange(const Point& a, const Rectangle& b);
bool IntersectWithRange(const Point& a, const Circle& b);
bool IntersectWithRange(const Record_t& a, const Rectangle& b);
bool IntersectWithRange(const Record_t& a, const Circle& b);
bool IntersectWithRange(const Record_t& a, const Rectangle_t& b);
bool IntersectWithRange(const Record_t& a, const Circle_t& b);

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
double CommRangeQuery(const Rectangle& a);
double CommRangeQuery(const Circle& a);
double CommQueryAnswer(const std::vector<Record_t>& a);
double CommQueryAnswer(const std::vector<Record>& a);
double CommQueryAnswer(const RecordSummary& a);

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

    void LogOneQuery(double _queryComm=0.0f) {
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
        double _queryTime = duration.count();

        queryNum += 1;
        queryTime += _queryTime;
        queryComm += _queryComm;

        endTime = startTime;
    }

    void Print() {
        double AvgQueryTime = (queryNum==0) ? 0 : (queryTime/queryNum);
        double AvgQueryComm = (queryNum==0) ? 0 : (queryComm/queryNum);
        printf("\n\n-------------- Query Log --------------\n");
        printf("QueryNum = %d, AvgQueryTime = %.4lfms, AvgQueryComm = %.4lfbytes\n\n", queryNum, AvgQueryTime, AvgQueryComm);
        fflush(stdout);
    }

private:
    std::chrono::steady_clock::time_point startTime, endTime;
    int queryNum;
    double queryTime;
    double queryComm;
};

}  // namespace ICDE18

#endif  // GRPC_COMMON_CPP_ICDE18_GLOBAL_H_
