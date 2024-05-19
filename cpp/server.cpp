#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <memory>
#include <mutex>
#include <thread>
#include <string>


#include <grpc/grpc.h>
#include <grpcpp/channel.h>
#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>

#include "global.h"
#include "ICDE18.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerReader;
using grpc::ServerReaderWriter;
using grpc::ServerWriter;
using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::ClientReaderWriter;
using grpc::ClientWriter;
using grpc::Status;
using ICDE18::Circle_t;
using ICDE18::Rectangle_t;
using ICDE18::Point;
using ICDE18::Rectangle;
using ICDE18::Circle;
using ICDE18::CircleQueryRange;
using ICDE18::RectangleQueryRange;
using ICDE18::Record;
using ICDE18::IntVector;
using ICDE18::RecordSummary;
using ICDE18::FedQueryService;
using ICDE18::QueryLogger;
using std::chrono::system_clock;

int queryNum = 0;

Point MakePoint(float x, float y) {
  Point p;
  p.set_x(x);
  p.set_y(y);
  return p;
}

Circle MakeCircle(float x, float y, float rad) {
  Circle c;
  c.set_rad(rad);
  c.mutable_center()->CopyFrom(MakePoint(x, y));
  return c;
}

Rectangle MakeRectangle(float x, float y, float dx, float dy) {
  Rectangle r;
  r.mutable_lo()->CopyFrom(MakePoint(x-dx, y-dy));
  r.mutable_hi()->CopyFrom(MakePoint(x+dx, y+dy));
  return r;
}

CircleQueryRange MakeCircleQueryRange(float x, float y, float rad) {
  CircleQueryRange ret;
  ret.set_id(queryNum++);
  ret.mutable_range()->CopyFrom(MakeCircle(x, y, rad));
  return ret;
}

RectangleQueryRange MakeRectangleQueryRange(float x, float y, float dx, float dy) {
  RectangleQueryRange ret;
  ret.set_id(queryNum++);
  ret.mutable_range()->CopyFrom(MakeRectangle(x, y, dx, dy));
  return ret;
}

class ServerToSilo {
public:
  ServerToSilo(std::shared_ptr<grpc::Channel> channel, const int id, const std::string& _IPAddress) : stub_(FedQueryService::NewStub(channel)) {
    serverID = id;
    IPAddress = _IPAddress;
  }

  ~ServerToSilo() {
    print();
  }

  void print() {
    printf("\n\nServerToSilo = %d, IPAddress = %s\n", serverID, IPAddress.c_str());
    //printf("The query log is as follows:\n");
    log.Print();
    printf("[Disconnect] ServerToSilo\n");
    fflush(stdout);
  }

  void GetLocalRecord(std::vector<ICDE18::Record>& res) {
    res = record_list_;
  }

  bool GetQueryAnswer(const Rectangle_t& _rect) {
    log.SetStartTimer();

    Rectangle rect = MakeRectangle(_rect.x, _rect.y, _rect.dx, _rect.dy);
    Record record;
    ClientContext context;

    printf("Looking for data records between (%.2lf, %.2lf) and (%.2lf, %.2lf)\n",
            rect.lo().x(), rect.lo().y(), rect.hi().x(), rect.hi().y());
    fflush(stdout);


    std::unique_ptr<ClientReader<Record> > reader(
        stub_->AnswerRectangleRangeQuery(&context, rect));
    record_list_.clear();
    while (reader->Read(&record)) {
      record_list_.emplace_back(record);
    }
    Status status = reader->Finish();
    if (status.ok()) {
      printf("gRPC [AnswerRectangleRangeQuery] succeeded.\n");
    } else {
      printf("gRPC [AnswerRectangleRangeQuery] failed.\n");
      exit(0);
    }

    log.SetEndTimer();
    log.LogOneQuery(CommQueryAnswer(record_list_));

    printf("There are %d objects in the query range:\n", (int)record_list_.size());
    for (auto record : record_list_) {
      if (record.has_p()) {
        printf("  ID = %d, location = (%.2lf,%.2lf)\n", 
                record.id(), record.p().x(), record.p().y());
      } else {
        printf("Data silo returns incomplete record.\n");
      }
    }
    fflush(stdout);

    return true;
  }

  bool GetQueryAnswer(const Circle_t& _circ) {
    log.SetStartTimer();

    Circle circ = MakeCircle(_circ.x, _circ.y, _circ.rad);
    Record record;
    ClientContext context;

    printf("Looking for data records within center(%.2lf, %.2lf) and radius %.2lf\n",
            circ.center().x(), circ.center().y(), circ.rad());
    fflush(stdout);

    std::unique_ptr<ClientReader<Record> > reader(
        stub_->AnswerCircleRangeQuery(&context, circ));
    record_list_.clear();
    while (reader->Read(&record)) {
      record_list_.emplace_back(record);
    }
    Status status = reader->Finish();
    if (status.ok()) {
      printf("gRPC [AnswerCircleRangeQuery] succeeded.\n");
      fflush(stdout);
    } else {
      printf("gRPC [AnswerCircleRangeQuery] failed.\n");
      fflush(stdout);
      exit(0);
    }

    log.SetEndTimer();
    log.LogOneQuery(CommQueryAnswer(record_list_));

    #ifdef LOCAL_DEBUG
    printf("There are %d objects in the query range:\n", (int)record_list_.size());
    for (auto record : record_list_) {
      if (record.has_p()) {
        printf("  ID = %d, location = (%.2lf,%.2lf)\n", 
                record.id(), record.p().x(), record.p().y());
      } else {
        printf("Data silo returns incomplete record.\n");
      }
    }
    fflush(stdout);
    #endif

    return true;
  }

  bool GetGridIndexCount() {
    ClientContext context;
    Empty request;
    IntVector response;
  
    Status Res = stub_->sumRequest(&context, request, &response); 
    if (status.ok()) {
      printf("gRPC [GetGridIndexCount] succeeded.\n");
      fflush(stdout);
    } else {
      printf("gRPC [GetGridIndexCount] failed.\n");
      fflush(stdout);
      exit(0);
    }

    log.LogAddComm(response.ByteSizeLong());

    m_GridIndexCounts.resize(response.size());
    for (int i=0,sz=response.size(); i<sz; ++i) {
      m_GridIndexCounts.emplace_back(response.values(i));
    }

    #ifdef LOCAL_DEBUG
    printf("There are %d grid in the grid index:\n", (int)response.size());
    for (int i=0,sz=m_GridIndexCounts.size(); i<sz; ++i) {
      if (i == 0)
        printf("  %d", m_GridIndexCounts[i]);
      else
        printf(", %d", m_GridIndexCounts[i]);
    }
    putchar('\n');
    fflush(stdout);
    #endif

    return true;
  }

private:
  std::unique_ptr<FedQueryService::Stub> stub_;
  std::vector<ICDE18::Record> record_list_;
  std::vector<size_t>> m_GridIndexCounts;
  QueryLogger log;
  int serverID;
  std::string IPAddress;
};

class FedQueryServiceServer {
public:
  FedQueryServiceServer(const std::string& fileName) {
    ICDE18::GetIPAddresses(fileName, m_IPAddresses);
    if (m_IPAddresses.empty()) {
      printf("%s contains no ip address\n", fileName.c_str());
      exit(0);
    }

    // bridge the channel between server and each data silo
    m_ServerToSilos.resize(m_IPAddresses.size());
    for (int i=0; i<m_IPAddresses.size(); ++i) {
      std::string IPAddress = m_IPAddresses[i];
      std::shared_ptr<grpc::Channel> channel = grpc::CreateChannel(IPAddress, grpc::InsecureChannelCredentials());
      printf("[Connect] channel with Silo %d at ip %s\n", i+1, IPAddress.c_str());
      m_ServerToSilos[i] = std::make_shared<ServerToSilo>(channel, i, IPAddress);
    }  
    m_GridIndexCounts.resize(m_IPAddresses.size());
  }

  void SetCircleQuery(const std::string& fileName, std::vector<Circle_t>& circles) {
    GetInputQuery(fileName, circles);
  }

  void SetRectangleQuery(const std::string& fileName, std::vector<Rectangle_t>& rectangles) {
    GetInputQuery(fileName, rectangles);
  }

  void GetCircleQueryAnswer(const std::string& fileName) {
    std::vector<Circle_t> circles;

    SetCircleQuery(fileName, circles);
    for (auto circ : circles) {
      GetQueryAnswer(circ);
    }

    log.Print();
  }


private:
  void _localRangeQuery(int siloID, const Circle_t& circ) {
    m_ServerToSilos[siloID]->GetQueryAnswer(circ);
  }

  void _localGetGridIndexCount(int siloID) {
    m_ServerToSilos[siloID]->GetQueryAnswer(circ);
  }

  void GetGridIndexCount() {
    std::vector<std::thread> thread_list(m_ServerToSilos.size());

    // execute local range query
    for (int i=0; i<m_ServerToSilos.size(); ++i) {
      thread_list[i] = std::thread(&FedQueryServiceServer::_localGetGridIndexCount, this, i);
    }
    for (int i=0; i<m_ServerToSilos.size(); ++i) {
      thread_list[i].join();
    }


  }

  void GetQueryAnswer(const Circle_t& circ) {
    log.SetStartTimer();

    std::vector<std::thread> thread_list(m_ServerToSilos.size());

    // execute local range query
    for (int i=0; i<m_ServerToSilos.size(); ++i) {
      thread_list[i] = std::thread(&FedQueryServiceServer::_localRangeQuery, this, i, circ);
    }
    for (int i=0; i<m_ServerToSilos.size(); ++i) {
      thread_list[i].join();
    }

    // execute secure aggregation
    record_list_.clear();
    std::vector<ICDE18::Record> tmp_list;
    for (int i=0; i<m_ServerToSilos.size(); ++i) {
      m_ServerToSilos[i]->GetLocalRecord(tmp_list);
      record_list_.insert(record_list_.end(), tmp_list.begin(), tmp_list.end());
    }

    log.SetEndTimer();
    log.LogOneQuery(CommQueryAnswer(record_list_));

    printf("There are %d objects in the query range:\n", (int)record_list_.size());
    for (auto record : record_list_) {
      if (record.has_p()) {
        printf("  ID = %d, location = (%.2lf,%.2lf)\n", 
                record.id(), record.p().x(), record.p().y());
      }
    }
    fflush(stdout);
  }

  std::vector<std::shared_ptr<ServerToSilo>> m_ServerToSilos;
  std::vector<std::string> m_IPAddresses;
  std::vector<ICDE18::Record> record_list_;
  std::vector<std::vector<size_t>> m_GridIndexCounts;
  QueryLogger log;
};

int main(int argc, char** argv) {
  // Expect only arg: --query_path=../../data/query.txt --ip_path=../../data/ip.txt
  std::string query_file = ICDE18::GetQueryFilePath(argc, argv);
  std::string ip_file = ICDE18::GetSiloIPFilePath(argc, argv);
  
  // std::string IPAddress("localhost:50051");
  // std::shared_ptr<grpc::Channel> channel = grpc::CreateChannel(IPAddress, grpc::InsecureChannelCredentials());
  
  printf("[Connect] Server\n");
  FedQueryServiceServer fedServer(ip_file);

  printf("-------------- Test Circle Range Query --------------\n");
  fedServer.GetCircleQueryAnswer(query_file);

  // printf("-------------- Test Rectangle Range Query --------------\n");
  // fedServer.GetRectangleQueryAnswer(query_file);


  return 0;
}

