#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <memory>
#include <mutex>
#include <thread>
#include <string>

#include <grpc/grpc.h>
#include <grpcpp/security/server_credentials.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>

#include "global.h"
#include "silo.hpp"
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
using ICDE18::Point;
using ICDE18::Rectangle;
using ICDE18::Circle;
using ICDE18::CircleQueryRange;
using ICDE18::RectangleQueryRange;
using ICDE18::Record;
using ICDE18::RecordSummary;
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
  ret.set_ID(queryNum++);
  ret.mutable_range()->CopyFrom(MakeCircle(x, y, rad));
  return ret;
}

RectangleQueryRange MakeRectangleQueryRange(float x, float y, float dx, float dy) {
  RectangleQueryRange ret;
  ret.set_ID(queryNum++);
  ret.mutable_range()->CopyFrom(MakeRectangle(x, y, dx, dy));
  return ret;
}

class FedQueryServiceServer {
public:
  FedQueryServiceServer(std::shared_ptr<Channel> channel, const std::string& _IPAddress) : stub_(FedQueryService::NewStub(channel)) {
    serverID = 1;
    IPAddress = _IPAddress;
  }

  ~FedQueryServiceServer() {
    print();
  }

  void print() {
    printf("server = %d, IPAddress = %s\n", serverID, IPAddress);
    printf("The query log is as follows:\n");
    log.print();
    printf("[Disconnect] Server\n");
    fflush(stdout);
  }

  void GetCircleQueryAnswer(const std::string& fileName) {
    std::vector<Circle_t> circles;

    SetCircleQuery(fileName, circles);
    for (auto circle : circles) {
      GetQueryAnswer(circle);
    }
  }


  void GetRectangleQueryAnswer(const std::string& fileName) {
    std::vector<Rectangle_t> rectangles;
    
    SetRectangleQuery(fileName, rectangles);
    for (auto rectangle : rectangles) {
      GetQueryAnswer(rectangle);
    }
  }

  void SetCircleQuery(const std::string& fileName, std::vector<Circle_t>& circles) {
    GetInputQuery(fileNames, circles);
  }

  void SetRectangleQuery(const std::string& fileName, std::vector<Rectangle_t>& rectangles) {
    GetInputQuery(fileNames, rectangles);
  }


private:
  bool GetQueryAnswer(const Rectangle_t&t _rect) {
    SetStartTimer.SetStartTimer();

    Rectangle rect = MakeRectangle(_rect.x, _rect.y, _rect.dx, _rect.dy);
    Record record;
    ClientContext context;

    printf("Looking for data records between (%.0lf, %.0lf) and (%.0lf, %.0lf)\n",
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
    }

    SetStartTimer.EndStartTimer();
    log.LogOneQuery(record.ByteSizeLong() * record_list_.size());

    printf("There are %d objects in the query range:\n", (int)record_list_.size()),
    for (auto record : record_list_) {
      if (record->has_ID() && record->has_p()) {
        printf("\tID = %d, location = (%.4lf,%.4lf)\n", 
                record->ID(), record->p().x(), record->p().y());
      } else {
        printf("Data silo returns incomplete record.\n");
      }
    }
    fflush(stdout);

    return true;
  }

  bool GetQueryAnswer(const Circle&t _circ) {
    SetStartTimer.SetStartTimer();

    Circle circ = MakeCircle(_circ.x, _circ.y, _circ.rad);
    Record record;
    ClientContext context;

    printf("Looking for data records within center(%.0lf, %.0lf) and radius %.0lf\n",
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
    } else {
      printf("gRPC [AnswerCircleRangeQuery] failed.\n");
    }

    SetStartTimer.EndStartTimer();
    log.LogOneQuery(record.ByteSizeLong() * record_list_.size());

    printf("There are %d objects in the query range:\n", (int)record_list_.size()),
    for (auto record : record_list_) {
      if (record->has_ID() && record->has_p()) {
        printf("\tID = %d, location = (%.4lf,%.4lf)\n", 
                record->ID(), record->p().x(), record->p().y());
      } else {
        printf("Data silo returns incomplete record.\n");
      }
    }
    fflush(stdout);

    return true;
  }

  std::unique_ptr<FedQueryService::Stub> stub_;
  std::vector<record> record_list_;
  queryLogger log;
  int serverID;
  std::string IPAddress;
};

int main(int argc, char** argv) {
  // Expect only arg: --query_path=path/to/route_guide_db.json.
  std::string query_file = ICDE18::GetQueryFilePath(argc, argv);
  std::string IPAddress("localhost:50051");

  std::shared_ptr<grpc::Channel> channel = grpc::CreateChannel(IPAddress, grpc::InsecureChannelCredentials());
  printf("[Connect] Server\n");
  FedQueryServiceServer fedServer(channel);

  printf("-------------- Test Rectangle Range Query --------------\n");
  fedServer.GetRectangleQueryAnswer(query_file);


  return 0;
}

