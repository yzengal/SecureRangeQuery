#include <algorithm>
#include <cctype>
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "global.h"
#define LOCAL_DEBUG

namespace ICDE18 {

inline double GetSquareDistance(const Point& a, const Point& b) {
    double deltax = a.x()-b.x();
    double deltay = a.y()-b.y();
    return deltax*deltax + deltay*deltay;
}

inline double GetSquareDistance(const Record_t& a, const Point& b) {
    double deltax = a.x-b.x();
    double deltay = a.y-b.y();
    return deltax*deltax + deltay*deltay;
}

inline double GetSquareDistance(const Record_t& a, const Record_t& b) {
    double deltax = a.x-b.x;
    double deltay = a.y-b.y;
    return deltax*deltax + deltay*deltay;
}

inline double GetDistance(const Point& a, const Point& b) {
    return std::sqrt(GetSquareDistance(a, b));
}

inline double GetDistance(const Record_t& a, const Point& b) {
    return std::sqrt(GetSquareDistance(a, b));
}

inline double GetDistance(const Record_t& a, const Record_t& b) {
    return std::sqrt(GetSquareDistance(a, b));
}

bool IntersectWithRange(const Point& a, const Rectangle& b) {
    double ax = a.x(), ay = a.y();
    return (b.lo().x()<=ax && ax<=b.hi().x()) && (b.lo().y()<=ay && ay<=b.hi().y());
}

bool IntersectWithRange(const Point& a, const Circle& b) {
    double rad = b.rad();
    return GetSquareDistance(a, b.center()) <= rad*rad;
}

bool IntersectWithRange(const Record_t& a, const Rectangle& b) {
    double ax = a.x, ay = a.y;
    return (b.lo().x()<=ax && ax<=b.hi().x()) && (b.lo().y()<=ay && ay<=b.hi().y());
}

bool IntersectWithRange(const Record_t& a, const Circle& b) {
    double rad = b.rad();
    return GetSquareDistance(a, b.center()) <= rad*rad;
}

bool IntersectWithRange(const Record_t& a, const Rectangle_t& b) {
    double ax = a.x, ay = a.y;
    double b_lo_x = b.x - b.dx;
    double b_hi_x = b.x + b.dx;
    double b_lo_y = b.y - b.dy;
    double b_hi_y = b.y + b.dy;
    return (b_lo_x<=ax && ax<=b_hi_x) && (b_lo_y<=ay && ay<=b_hi_y);
}

bool IntersectWithRange(const Record_t& a, const Circle_t& b) {
    Record_t center;
    center.x = b.x;
    center.y = b.y;
    return GetSquareDistance(a, center) <= b.rad*b.rad;
}

double CommRangeQuery(const Rectangle& a) {
    return a.ByteSizeLong();
}

double CommRangeQuery(const Circle& a) {
    return a.ByteSizeLong();
}

double CommQueryAnswer(const std::vector<Record>& a) {
    if (a.size() == 0)
        return sizeof(a);
    else
        return sizeof(a) + a.size() * a[0].ByteSizeLong();
}


double CommQueryAnswer(const std::vector<Record_t>& a) {
    Record tmp;
    Point p;

    tmp.set_id(0);
    p.set_x(0);
    p.set_y(0);
    tmp.mutable_p()->CopyFrom(p);

    if (a.size() == 0)
        return sizeof(a);
    else
        return sizeof(a) + a.size() * tmp.ByteSizeLong();
}

double CommQueryAnswer(const RecordSummary& a) {
    return a.ByteSizeLong();
}

std::string GetDataFilePath(int argc, char** argv) {
    std::string db_path;
    std::string arg_str("--data_path");
    if (argc > 1) {
        std::string argv_1 = argv[1];
        size_t start_position = argv_1.find(arg_str);
        if (start_position != std::string::npos) {
            start_position += arg_str.size();
            if (argv_1[start_position] == ' ' || argv_1[start_position] == '=') {
                db_path = argv_1.substr(start_position + 1);
            }
        }
    } else {
        db_path = "/home/yzengal/SecureRangeQuery/data/data_01.txt";
    }
    
    return db_path;
}

std::string GetQueryFilePath(int argc, char** argv) {
    std::string db_path;
    std::string arg_str("--query_path");
    if (argc > 1) {
        std::string argv_1 = argv[1];
        size_t start_position = argv_1.find(arg_str);
        if (start_position != std::string::npos) {
            start_position += arg_str.size();
            if (argv_1[start_position] == ' ' || argv_1[start_position] == '=') {
                db_path = argv_1.substr(start_position + 1);
            }
        }
    } else {
        db_path = "/home/yzengal/SecureRangeQuery/data/query.txt";
    }
    
    return db_path;
}

void GetInputData(const std::string& fileName, std::vector<Record_t>& recordVector) {
    std::ifstream fin(fileName);
    if (!fin.is_open()) {
        printf("Failed to open %s\n", fileName.c_str());
        abort();
    }

    int n;

    fin >> n;
    if (fin.fail()) {
        printf("Failed to parse the data record %s\n", fileName.c_str());
        abort();
    }

    recordVector.resize(n);
    for (int i=0; i<n; ++i) {
        fin >> recordVector[i].ID >> recordVector[i].x >> recordVector[i].y; 
        #ifdef LOCAL_DEBUG
        printf("Record #%d: ID=%d, location=(%.2lf, %.2lf)\n", i+1, recordVector[i].ID, recordVector[i].x, recordVector[i].y);
        #endif
    }
    fin.close();
}

QueryType_t GetQueryType(const std::string& str) {
    if (str == "RangeQuery") {
        return QueryType_t::RANGE_QUERY;
    } else if (str == "RangeCount") {
        return QueryType_t::RANGE_COUNT;
    } else if (str == "DistanceJoin") {
        return QueryType_t::DISTANCE_JOIN;
    } else {
        return QueryType_t::UNDEFINED;
    }
}

void GetInputQuery(const std::string& fileName, std::vector<Rectangle_t>& queries) {
    std::ifstream fin(fileName);
    if (!fin.is_open()) {
        printf("Failed to open %s\n", fileName.c_str());
        abort();
    }

    int qn;
    std::string queryType;

    fin >> qn;
    if (fin.fail()) {
        printf("Failed to parse the query number, %s\n", fileName.c_str());
        abort();
    }

    queries.resize(qn);
    for (int i=0; i<qn; ++i) {
        fin >> queryType >> queries[i].x >> queries[i].y >> queries[i].dx >> queries[i].dy;
        if (fin.fail()) {
            std::cout << "Failed to parse the %d-th query " << i+1 << std::endl;
            abort();
        }  
        queries[i].qtype = GetQueryType(queryType);
    }
    

    fin.close();
}

void GetInputQuery(const std::string& fileName, std::vector<Circle_t>& queries) {
    std::ifstream fin(fileName);
    if (!fin.is_open()) {
        printf("Failed to open %s\n", fileName.c_str());
        abort();
    }

    int qn;
    std::string queryType;

    fin >> qn;
    if (fin.fail()) {
        printf("Failed to parse the query number %s\n", fileName.c_str());
        abort();
    }

    queries.resize(qn);
    for (int i=0; i<qn; ++i) {
        fin >> queryType >> queries[i].x >> queries[i].y >> queries[i].rad;
        if (fin.fail()) {
            std::cout << "Failed to parse the %d-th query " << i+1 << std::endl;
            abort();
        }  
        queries[i].qtype = GetQueryType(queryType);
    }

    fin.close();
}

}  // namespace ICDE18
