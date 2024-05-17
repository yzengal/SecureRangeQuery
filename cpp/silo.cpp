#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <thread>
#include <signal.h>
#include <unistd.h>

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

  ret.set_id(r.ID);
  p.set_x(r.x);
  p.set_y(r.y);
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
            if (IntersectWithRange(rec, circ)) {
                ans.emplace_back(rec);
            }
        }

        log.SetEndTimer();
        log.LogOneQuery(CommRangeQuery(range)+CommQueryAnswer(ans));
    }

    void AnswerRectangleRangeQuery(const Rectangle& range, std::vector<Record_t>& ans) {
        log.SetStartTimer();
        ICDE18::Rectangle_t rect;

        rect.x = (range.lo().x() + range.hi().x()) * 0.5;
        rect.y = (range.lo().y() + range.hi().y()) * 0.5;
        rect.dx = std::abs(range.hi().x() - range.lo().x()) * 0.5;
        rect.dy = std::abs(range.hi().y() - range.lo().y()) * 0.5;

        ans.clear();
        for (auto rec : data) {
            if (IntersectWithRange(rec, rect)) {
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
            if (IntersectWithRange(rec, range)) {
                ++ret;
            }
        }
        ans.set_point_count(ret);
        log.SetEndTimer();
        log.LogOneQuery(CommRangeQuery(range)+CommQueryAnswer(ans));

        return ret;
    }

    int AnswerRectangleRangeCount(const Rectangle& range, RecordSummary& ans) {
        int ret = 0;

        log.SetStartTimer();
        for (auto rec : data) {
            if (IntersectWithRange(rec, range)) {
                ++ret;
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

    void SetSiloIPAddress(std::string _siloIP) {
        siloIP = _siloIP;
    }

    void Print() {
        printf("SiloID = %d, IPAddress = %s, DataSize = %d\n", siloID, siloIP.c_str(), (int)data.size());
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
    explicit FedQueryServiceImpl(const int siloID, const std::string& fileName) {
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
           writer->Write(record);
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

        #ifdef LOCAL_DEBUG
        printf("Silo %d: CircleRangeQuery, center=(%.2lf,%.2lf), rad=%.2lf\n", 
                m_silo->GetSiloID(), circle->center().x(), circle->center().y(), circle->rad());
        fflush(stdout);
        std::this_thread::sleep_for(std::chrono::seconds(1));
        #endif

        std::vector<Record_t> ans;
        m_silo->AnswerCircleRangeQuery(*circle, ans);
        for (auto record_ : ans) {
           record = MakeRecord(record_);
           m_RecordVector.emplace_back(record_);
           writer->Write(record);
        }

        log.SetEndTimer();
        log.LogOneQuery(record.ByteSizeLong() * ans.size());

        return Status::OK;
    }

    void Print() {
        m_silo->Print();
    }

private:
    std::vector<Record_t> m_RecordVector;
    std::unique_ptr<Silo> m_silo;
    QueryLogger log;
};

std::unique_ptr<FedQueryServiceImpl> siloService_ptr;

void RunSilo(const int siloID, const std::string& IPAddress, const std::string& data_file) {
    std::string server_address(IPAddress);

    siloService_ptr = std::make_unique<FedQueryServiceImpl>(siloID, data_file);
    // FedQueryServiceImpl siloService(siloID, data_file);

    ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(siloService_ptr.get());
    std::unique_ptr<Server> server(builder.BuildAndStart());
    std::cout << "Server listening on " << server_address << std::endl;
    server->Wait();
}


// Ensure the log file is output, when the program is terminated.
void SignalHandler(int signal) {
    if (siloService_ptr) {
        siloService_ptr->Print();
    }
    exit(0);
}

void ResetSignalHandler() {
    signal(SIGINT, SignalHandler);
    signal(SIGQUIT, SignalHandler);
    signal(SIGTERM, SignalHandler);
    signal(SIGKILL, SignalHandler);
}

int main(int argc, char** argv) {
    ResetSignalHandler();

    // Expect two args: --ip=0.0.0.0:50051 --data_path=../../data/data_01.txt --silo_id=1
    std::string IPAddress = ICDE18::GetIPAddress(argc, argv);
    std::string data_file = ICDE18::GetDataFilePath(argc, argv);
    int siloID = ICDE18::GetSiloID(argc, argv);

    RunSilo(siloID, IPAddress, data_file);

    return 0;
}

