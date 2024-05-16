#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include <grpc/grpc.h>
#include <grpcpp/security/server_credentials.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>

#include "global.h"
#include "ICDE18.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerReader;
using grpc::ServerReaderWriter;
using grpc::ServerWriter;
using grpc::Status;
using ICDE18::Record_t;
using ICDE18::Point;
using ICDE18::Rectangle;
using ICDE18::Circle;
using ICDE18::CircleQueryRange;
using ICDE18::RectangleQueryRange;
using ICDE18::Record;
using ICDE18::RecordSummary;
using ICDE18::QueryLogger;
using ICDE18::FedQueryService;
using std::chrono::system_clock;

Record MakeRecord(const Record_t& r) {
  Record ret;
  Point p;

  p.set_x(r.x);
  p.set_y(r.y);
  ret.set_ID(r.ID);
  ret.mutable_p()->CopyFrom(p);
  
  return ret;
}

class Silo {
public:
    Silo(const int _siloID=0, const std::string& fileName="") : siloID(_siloID) {
        SetDataRecord(fileName);
    }

    ~Silo() {
        Print();
    }

    void AnswerCircleRangeQuery(const Circle& range, std::vector<Record_t>& ans) {
        log.SetStartTimer();
        ICDE18::Circle_t circ;

        circ.rad = range.rad();
        circ.x = range.center().x();
        circ.y = range.center().y();

        ans.clear();
        for (auto rec : data) {
            if (IntersectWithRange(rec.p, circ)) {
                ans.emplace_back(rec);
            }
        }

        log.SetEndTimer();
        log.LogOneQuery(CommRangeQuery(range)+CommQueryAnswer(ans));
    }

    void AnsweRectangleRangeQuery(const Rectangle& range, std::vector<Record_t>& ans) {
        log.SetStartTimer();
        ICDE18::Rectangle_t rect;

        rect.x = (range.lo().x() + range.hi().x()) * 0.5;
        rect.y = (range.lo().y() + range.hi().y()) * 0.5;
        rect.dx = std::abs(range.hi().x() - range.lo().x()) * 0.5;
        rect.dy = std::abs(range.hi().y() - range.lo().y()) * 0.5;

        ans.clear();
        for (auto rec : data) {
            if (IntersectWithRange(rec.p, rect)) {
                ans.emplace_back(rec);
            }
        }

        log.SetEndTimer();
        log.LogOneQuery(CommRangeQuery(range)+CommQueryAnswer(ans));
    }

    int AnswerCircleRangeCount(const Circle& range, RecordSummary& ans) {
        int ret = 0;

        log.SetStartTimer();
        for (auto rec : data) {
            if (IntersectWithRange(rec.p, range)) {
                ++ret;
            }
        }
        ans.set_point_count(ret);
        log.SetEndTimer();
        log.LogOneQuery(CommRangeQuery(range)+CommQueryAnswer(ans));

        return ret;
    }

    int AnsweRectangleRangeCount(const Rectangle& range, RecordSummary& ans) {
        int ret = 0;

        log.SetStartTimer();
        for (auto rec : data) {
            if (IntersectWithRange(rec.p, range)) {
                ans.emplace_back(rec);
            }
        }
        ans.set_point_count(ret);
        log.SetEndTimer();
        log.LogOneQuery(CommRangeQuery(range)+CommQueryAnswer(ans));

        return ret;
    }

    int GetDataNum() { 
        return data.size(); 
    }

    int GetSiloID() {
        return siloID;
    }

    std::string GetSiloIP() {
        return siloIP;
    }

    void SetDataRecord(const std::string& fileName) {
        data.clear();
        ICDE18::GetInputData(fileName, data);
        printf("-------------- Silo %d Load Data --------------\n", siloID);
        fflush(stdout);
    }

    void SetSiloID(int _siloID) {
        siloID = _siloID;
    }

    void SetSiloIPAddress(string _siloIP) {
        siloIP = _siloIP;
    }

    void Print() {
        printf("SiloID = %d, IPAddress = %s:%d, DataSize = %d\n", siloID, siloIP, siloPort, (int)data.size());
        printf("The query log is as follows:\n");
        log.Print();
        printf("\n\n");
        fflush(stdout);
    }

private:
    int siloID;
    QueryLogger log;
    std::vector<Record_t> data;
    std::string siloIP;
};

class FedQueryServiceImpl final : public FedQueryService::Service {
public:
    explicit FedQueryServiceImpl(const int siloID, const std::sting& fileName) {
        m_silo = std::make_unique<Silo>(siloID, fileName);  
    }

    Status AnswerRectangleRangeQuery(ServerContext* context,
                        const ICDE18::Rectangle* rectangle,
                        ServerWriter<Record>* writer) override {
        log.SetStartTimer();
        Record record;
        m_RecordVector.clear();

        std::vector<Record_t> ans;
        m_silo->AnswerRectangleRangeQuery(*rectangle, ans);
        for (auto record_ : ans) {
           record = MakeRecord(record_);
           m_RecordVector.emplace_back(record_);
           writer->Write(&record);
        }

        log.SetEndTimer();
        log.LogOneQuery(record.ByteSizeLong() * ans.size());

        return Status::OK;
    }

    Status AnswerCircleRangeQuery(ServerContext* context,
                        const ICDE18::Circle* circle,
                        ServerWriter<Record>* writer) override {
        log.SetStartTimer();
        Record record;
        m_RecordVector.clear();

        std::vector<Record_t> ans;
        m_silo->AnswerCircleRangeQuery(*circle, ans);
        for (auto record_ : ans) {
           record = MakeRecord(record_);
           m_RecordVector.emplace_back(record_);
           writer->Write(&record);
        }

        log.SetEndTimer();
        log.LogOneQuery(record.ByteSizeLong() * ans.size());

        return Status::OK;
    }

private:
    std::vector<Record_t> m_RecordVector;
    std::unique_ptr<Silo> m_silo;
    QueryLogger log;
};

void RunSilo(const std::string& IPAddress, const std::string& db_path) {
    std::string server_address(IPAddress);
    int siloID = 1;

    FedQueryServiceImpl siloService(siloID, db_path);

    ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&siloService);
    std::unique_ptr<Server> server(builder.BuildAndStart());
    std::cout << "Server listening on " << server_address << std::endl;
    server->Wait();
}

int main(int argc, char** argv) {
    // Expect only arg: --data_path=path/to/route_guide_db.json.
    std::string db = ICDE18::GetDataFilePath(argc, argv);
    std::string IPAddress("0.0.0.0:50051");

    RunSilo(db);

    return 0;
}

