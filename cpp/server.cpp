#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <iomanip>
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
#include "differentialprivacy.h"
#include "ICDE18.grpc.pb.h"
#include "AES.h"

#define ENCRYPT_RECORD

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
using google::protobuf::Empty;
using ICDE18::Circle_t;
using ICDE18::Rectangle_t;
using ICDE18::Record_t;
using ICDE18::Point_t;
using ICDE18::Point;
using ICDE18::Rectangle;
using ICDE18::Circle;
using ICDE18::CircleQueryRange;
using ICDE18::RectangleQueryRange;
using ICDE18::Record;
using ICDE18::EncryptRecord;
using ICDE18::IntVector;
using ICDE18::FloatVector;
using ICDE18::ByteVector;
using ICDE18::GridIndexCounts;
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
    #ifdef LOCAL_DEBUG
    print();
    #endif
  }

  void print() {
    printf("\n\nServerToSilo = %d, IPAddress = %s\n", serverID, IPAddress.c_str());
    //printf("The query log is as follows:\n");
    log.Print();
    printf("[Disconnect] ServerToSilo\n");
    fflush(stdout);
  }

  void GetLocalRecord(std::vector<ICDE18::Record>& res) {
    res = m_record_list;
  }

  bool GetQueryAnswer(const Rectangle_t& _rect) {
    log.SetStartTimer();

    Rectangle rect = MakeRectangle(_rect.x, _rect.y, _rect.dx, _rect.dy);
    Record record;
    ClientContext context;

    #ifdef LOCAL_DEBUG
    printf("Looking for data records between (%.2f, %.2f) and (%.2f, %.2f)\n",
            rect.lo().x(), rect.lo().y(), rect.hi().x(), rect.hi().y());
    fflush(stdout);
    #endif

    std::unique_ptr<ClientReader<Record> > reader(
        stub_->AnswerRectangleRangeQuery(&context, rect));
    m_record_list.clear();
    while (reader->Read(&record)) {
      m_record_list.emplace_back(record);
    }
    Status status = reader->Finish();
    if (status.ok()) {
      #ifdef LOCAL_DEBUG
      printf("gRPC [AnswerRectangleRangeQuery] succeeded.\n");
      #endif
    } else {
      #ifdef LOCAL_DEBUG
      printf("gRPC [AnswerRectangleRangeQuery] failed.\n");
      #endif
      exit(-1);
    }

    log.SetEndTimer();
    queryComm += CommQueryAnswer(m_record_list);
    log.LogOneQuery(CommQueryAnswer(m_record_list));

    #ifdef LOCAL_DEBUG
    printf("There are %d objects in the query range:\n", (int)m_record_list.size());
    for (auto record : m_record_list) {
      if (record.has_p()) {
        printf("  ID = %d, location = (%.2f,%.2f)\n", 
                record.id(), record.p().x(), record.p().y());
      } else {
        printf("Data silo returns incomplete record.\n");
      }
    }
    fflush(stdout);
    #endif

    return true;
  }

  bool GetQueryAnswer(const Circle_t& _circ) {
    log.SetStartTimer();

    Circle circ = MakeCircle(_circ.x, _circ.y, _circ.rad);
    Record record;
    ClientContext context;

    #ifdef LOCAL_DEBUG
    printf("Looking for data records within center(%.2f, %.2f) and radius %.2f\n",
            circ.center().x(), circ.center().y(), circ.rad());
    fflush(stdout);
    #endif

    std::unique_ptr<ClientReader<Record> > reader(
        stub_->AnswerCircleRangeQuery(&context, circ));
    m_record_list.clear();
    while (reader->Read(&record)) {
      m_record_list.emplace_back(record);
    }
    Status status = reader->Finish();
    if (status.ok()) {
      #ifdef LOCAL_DEBUG
      printf("gRPC [AnswerCircleRangeQuery] succeeded.\n");
      fflush(stdout);
      #endif
    } else {
      #ifdef LOCAL_DEBUG
      printf("gRPC [AnswerCircleRangeQuery] failed.\n");
      fflush(stdout);
      #endif
      exit(-1);
    }

    log.SetEndTimer();
    queryComm += CommQueryAnswer(m_record_list);
    log.LogOneQuery(CommQueryAnswer(m_record_list));

    #ifdef LOCAL_DEBUG
    printf("There are %d objects in the query range:\n", (int)m_record_list.size());
    for (auto record : m_record_list) {
      if (record.has_p()) {
        printf("  ID = %d, location = (%.2f,%.2f)\n", 
                record.id(), record.p().x(), record.p().y());
      } else {
        printf("Data silo returns incomplete record.\n");
      }
    }
    fflush(stdout);
    #endif

    return true;
  }

  void GetGridIndex() {
    ClientContext context;
    Empty request;
    GridIndexCounts response;
  
    Status status = stub_->GetGridIndex(&context, request, &response); 
    if (status.ok()) {
      #ifdef LOCAL_DEBUG
      printf("gRPC [GetGridIndex] succeeded.\n");
      fflush(stdout);
      #endif
    } else {
      #ifdef LOCAL_DEBUG
      printf("gRPC [GetGridIndex] failed.\n");
      fflush(stdout);
      #endif
      exit(-1);
    }

    queryComm += response.ByteSizeLong();
    log.LogAddComm(response.ByteSizeLong());
    
    m_K = response.k();
    ICDE18::CopyToVector<float>(m_mins, response.mins());
    ICDE18::CopyToVector<float>(m_maxs, response.maxs());
    ICDE18::CopyToVector<float>(m_widths, response.widths());
    ICDE18::CopyToVector<size_t>(m_counts, response.counts());

    #ifdef LOCAL_DEBUG
    size_t sum_counts = 0;
    for (auto cnt : m_counts) sum_counts += cnt;
    printf("There are %d grid in the grid index (K = %d): %d\n", (int)m_counts.size(), m_K, (int)sum_counts);
    for (int i=0,sz=m_counts.size(); i<sz; ++i) {
      if (i == 0)
        printf("  %d", (int)m_counts[i]);
      else
        printf(", %d", (int)m_counts[i]);
    }
    putchar('\n');
    fflush(stdout);
    #endif
  }

  void SendFilterGridIndex(const Circle_t& _circ) {
    ClientContext context;
    IntVector request;
    Empty response;
    size_t request_sz = 0;

    for (size_t i=0, sz=m_counts.size(); i<sz; ++i) {
      if (!this->GridIntersectCircle(i, _circ))
        continue;
      // check whether the grid intersects with the circle range
      if (m_counts[i] != 0) {
        ++request_sz;
        request.add_values(i);
      }
    }
    request.set_size(request_sz);

    Status status = stub_->SendFilterGridIndex(&context, request, &response); 
    if (status.ok()) {
      #ifdef LOCAL_DEBUG
      printf("gRPC [SendFilterGridIndex] succeeded.\n");
      fflush(stdout);
      #endif
    } else {
      #ifdef LOCAL_DEBUG
      printf("gRPC [SendFilterGridIndex] failed.\n");
      fflush(stdout);
      #endif
      exit(-1);
    }

    queryComm += request.ByteSizeLong();
    log.LogAddComm(request.ByteSizeLong());
  }

  float GetQueryComm() {
    return this->queryComm;
  }

  size_t GetRecordFP() {
    return this->m_record_fp;
  }

  void InitQueryComm(float init_value = 0.0f) {
    queryComm = init_value;
  }

  void GetFilterGridRecord() {
    m_record_list.clear();

    Record cand_record;
    ClientContext context;
    Empty request;

    #ifdef LOCAL_DEBUG
    printf("gRPC [GetFilterGridRecord] start.\n");
    fflush(stdout);
    #endif

    #ifdef ENCRYPT_RECORD
    // step 1: get the decrypted keys
    ByteVector response;
    ClientContext context_tmp;

    Status status = stub_->GetEncryptKeys(&context_tmp, request, &response); 
    if (!status.ok()) {
      exit(-1);
    }

    queryComm += response.ByteSizeLong();
    log.LogAddComm(response.ByteSizeLong());

    const int n_keys = response.size();
    const std::string& received_key_data = response.values();  
    std::vector<unsigned char> decrypt_keys(received_key_data.begin(), received_key_data.end());  
    
    // std::cout << n_keys << " " << decrypt_keys.size() << std::endl;
    // for (int i=0; i<decrypt_keys.size(); ++i) {
    //   std::cout << std::hex << std::uppercase; // 设置为十六进制，并输出为大写字母  
    //   std::cout << std::setw(2) << std::setfill('0'); // 设置宽度为2，不足时前面用'0'填充  
    //   std::cout << static_cast<int>(decrypt_keys[i]) << " ";
    // }
    // std::cout << std::endl;
    // fflush(stdout);

    // step 2: get the encrypted records
    AES aes(AESKeyLength::AES_256);
    EncryptRecord encrypt_record;

    std::unique_ptr<ClientReader<EncryptRecord> > reader(
        stub_->GetFilterGridEncryptRecord(&context, request));
    while (reader->Read(&encrypt_record)) {
      // step 2.1: get encrypted bytes
      const std::string& received_record_data = encrypt_record.data();  
      std::vector<unsigned char> encrypt_record_data(received_record_data.begin(), received_record_data.end());  
      // step 2.2: get decrypted bytes
      std::vector<unsigned char> record_data = aes.DecryptECB(encrypt_record_data, decrypt_keys);
      // step 2.3: get decrypted record
      Record_t rec = ICDE18::DeserializeRecord(record_data);
      // step 2.4: transform record_t into record
      cand_record = MakeRecord(rec);
      m_record_list.emplace_back(cand_record);
    }
    status = reader->Finish();
    if (!status.ok()) {
      exit(-1);
    }
    queryComm += encrypt_record.ByteSizeLong() * m_record_list.size();
    log.LogAddComm(encrypt_record.ByteSizeLong() * m_record_list.size());

    #else
    
    std::unique_ptr<ClientReader<Record> > reader(
        stub_->GetFilterGridRecord(&context, request));
    while (reader->Read(&cand_record)) {
      m_record_list.emplace_back(cand_record);
    }
    Status status = reader->Finish();
    if (status.ok()) {
      #ifdef LOCAL_DEBUG
      printf("gRPC [GetFilterGridRecord] succeeded.\n");
      fflush(stdout);
      #endif
    } else {
      #ifdef LOCAL_DEBUG
      printf("gRPC [GetFilterGridRecord] failed.\n");
      fflush(stdout);
      #endif
      exit(-1);
    }
    queryComm += CommQueryAnswer(m_record_list);
    log.LogAddComm(CommQueryAnswer(m_record_list));
    #endif

    #ifdef LOCAL_DEBUG
    printf("There are %d objects in the query range:\n", (int)m_record_list.size());
    for (auto record : m_record_list) {
      if (record.has_p()) {
        printf("  ID = %d, location = (%.2f,%.2f)\n", 
                record.id(), record.p().x(), record.p().y());
      } else {
        printf("Data silo returns incomplete record.\n");
      }
    }
    fflush(stdout);

    #endif   
  }

  void VerifyGridRecord(const Circle_t& circ, std::vector<ICDE18::Record>& res_record_list) {
    res_record_list.clear();
    m_record_fp = 0;
    for (auto record : m_record_list) {
      if (!record.has_p()) {
        continue;
      }
      if (record.id() >= 0) {
        ++m_record_fp;
      }
      ICDE18::Record_t record_tmp(-1, record.p().x(), record.p().y());
      if (ICDE18::IntersectWithRange(record_tmp, circ)) {
        if (record.id() >= 0)
          res_record_list.emplace_back(record);
      }
    }
  }

private:
  Record MakeRecord(const Record_t& r) {
      Record ret;
      Point p;

      ret.set_id(r.ID);
      p.set_x(r.x);
      p.set_y(r.y);
      ret.mutable_p()->CopyFrom(p);
      
      return ret;
  }

  bool GridIntersectCircle(const size_t& gid, const Circle_t& circ) {
    size_t idx_x = gid % this->m_K;
    size_t idx_y = gid / this->m_K;

    float lo_x = m_mins[0] + idx_x * this->m_widths[0];
    float hi_x = lo_x + this->m_widths[0];
    float lo_y = m_mins[1] + idx_y * this->m_widths[1];
    float hi_y = lo_y + this->m_widths[1];

    // check if the circle center is inside the rectangle;
    if (lo_x<=circ.x && circ.x<=hi_x && lo_y<=circ.y && circ.y<=hi_y) {
      return true;
    } 

    // check if any corner of the rectangle is inside the circle
    const int temporal_record_id = -1;
    if (ICDE18::IntersectWithRange(ICDE18::Record_t(temporal_record_id, lo_x, lo_y), circ) || 
        ICDE18::IntersectWithRange(ICDE18::Record_t(temporal_record_id, lo_x, hi_y), circ) ||
        ICDE18::IntersectWithRange(ICDE18::Record_t(temporal_record_id, hi_x, lo_y), circ) ||
        ICDE18::IntersectWithRange(ICDE18::Record_t(temporal_record_id, hi_x, hi_y), circ) ) {
      return true;
    }

    return false;
  }

  std::unique_ptr<FedQueryService::Stub> stub_;
  std::vector<ICDE18::Record> m_record_list;
  std::vector<size_t> m_counts;
  std::vector<float> m_mins, m_maxs, m_widths;
  QueryLogger log;
  int serverID, m_K;
  size_t m_record_fp;
  std::string IPAddress;
  float queryComm = 0;
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
    grpc::ChannelArguments args;  
    args.SetInt(GRPC_ARG_MAX_SEND_MESSAGE_LENGTH, INT_MAX);  
    args.SetInt(GRPC_ARG_MAX_RECEIVE_MESSAGE_LENGTH, INT_MAX); 
    for (int i=0; i<m_IPAddresses.size(); ++i) {
      std::string IPAddress = m_IPAddresses[i];
      std::shared_ptr<grpc::Channel> channel = grpc::CreateCustomChannel(IPAddress, grpc::InsecureChannelCredentials(), args);
      m_ServerToSilos[i] = std::make_shared<ServerToSilo>(channel, i, IPAddress);
      
      #ifdef LOCAL_DEBUG
      printf("[Connect] channel with Silo %d at ip %s\n", i+1, IPAddress.c_str());
      fflush(stdout);
      #endif
    }  
    m_GridIndexCounts.resize(m_IPAddresses.size());
  }

  void SetCircleQuery(const std::string& fileName, std::vector<Circle_t>& circles) {
    GetInputQuery(fileName, circles);
  }

  void SetRectangleQuery(const std::string& fileName, std::vector<Rectangle_t>& rectangles) {
    GetInputQuery(fileName, rectangles);
  }

  void GetQueryAnswer(const std::string& fileName) {
    std::vector<Circle_t> circles;

    SetCircleQuery(fileName, circles);
    for (int i=0,sz=circles.size(); i<sz; ++i) {
      GetQueryAnswer(circles[i]);
    }

    log.Print();
  }

  void GetQueryAnswer_byGridIndex(const std::string& fileName) {
    std::vector<Circle_t> circles;

    SetCircleQuery(fileName, circles);
    printf("%zu\n", circles.size());
    for (int i=0,sz=circles.size(); i<sz; ++i) {
      m_GetQueryAnswer_byGridIndex(circles[i], i+1);
    }

    log.Print();
  }
  
private:
  void _localRangeQuery(int siloID, const Circle_t& circ) {
    m_ServerToSilos[siloID]->GetQueryAnswer(circ);
  }

  void _localGetGridIndex(int siloID) {
    m_ServerToSilos[siloID]->GetGridIndex();
  }

  void _localSendFilterGridIndex(int siloID, const Circle_t& circ) {
    m_ServerToSilos[siloID]->SendFilterGridIndex(circ);
  }

  void _localGetFilterGridRecord(int siloID) {
     m_ServerToSilos[siloID]->GetFilterGridRecord();
  }

  void GetGridIndex() {
    std::vector<std::thread> thread_list(m_ServerToSilos.size());

    for (int i=0; i<m_ServerToSilos.size(); ++i) {
      thread_list[i] = std::thread(&FedQueryServiceServer::_localGetGridIndex, this, i);
    }
    for (int i=0; i<m_ServerToSilos.size(); ++i) {
      thread_list[i].join();
    }
  }

  void SendFilterGridIndex(const Circle_t& circ) {
    std::vector<std::thread> thread_list(m_ServerToSilos.size());

    for (int i=0; i<m_ServerToSilos.size(); ++i) {
      thread_list[i] = std::thread(&FedQueryServiceServer::_localSendFilterGridIndex, this, i, circ);
    }
    for (int i=0; i<m_ServerToSilos.size(); ++i) {
      thread_list[i].join();
    }   
  }

  void GetFilterGridRecord() {
    std::vector<std::thread> thread_list(m_ServerToSilos.size());

    for (int i=0; i<m_ServerToSilos.size(); ++i) {
      thread_list[i] = std::thread(&FedQueryServiceServer::_localGetFilterGridRecord, this, i);
    }
    for (int i=0; i<m_ServerToSilos.size(); ++i) {
      thread_list[i].join();
    }       
  }

  void m_GetQueryAnswer_byGridIndex(const Circle_t& circ, const int qid=-1) {
    // step0. initialization
    log.SetStartTimer();
    for (int i=0; i<m_ServerToSilos.size(); ++i) {
      m_ServerToSilos[i]->InitQueryComm();
    }

    // step1. Get Grid Index
    GetGridIndex();

    // step2. Filter Grid Index
    std::pair<double,double> perturb_point = DIFFERENTIALPRIVACY::PlanarLaplaceMechanism(circ.x, circ.y, DIFFERENTIALPRIVACY::SPATIAL_DP_EPSILON);
    Circle_t perturb_circ(circ.qtype, perturb_point.first, perturb_point.second, circ.rad);
    perturb_circ.rad += ICDE18::GetDistance(Point_t(circ.x, circ.y), Point_t(perturb_circ.x, perturb_circ.y));

    SendFilterGridIndex(perturb_circ);

    // step3. Receive records in the filtered grids
    GetFilterGridRecord();

    // step4. Verify the data records
    m_record_list.clear();
    size_t m_record_fp = 0;
    for (int i=0; i<m_ServerToSilos.size(); ++i) {
      std::vector<ICDE18::Record> record_list_tmp;
      m_ServerToSilos[i]->VerifyGridRecord(circ, record_list_tmp);
      m_record_fp += m_ServerToSilos[i]->GetRecordFP();
      m_record_list.insert(m_record_list.end(), record_list_tmp.begin(), record_list_tmp.end());
    }

    log.SetEndTimer();
    for (int i=0; i<m_ServerToSilos.size(); ++i) {
      log.LogAddComm(m_ServerToSilos[i]->GetQueryComm());
    }
    log.LogOneQuery(CommQueryAnswer(m_record_list));


    /*
    *   Dump the query result
    *
    */
    printf("%d %zu %zu\n", qid, m_record_list.size(), m_record_fp);
    std::vector<int> ids_list_tmp;
    for (auto record : m_record_list) {
      if (record.has_p()) {
        ids_list_tmp.emplace_back(record.id());
      }
    }
    sort(ids_list_tmp.begin(), ids_list_tmp.end());
    for (int i=0; i<ids_list_tmp.size(); ++i) {
      if (i == 0)
        printf("%d", ids_list_tmp[i]);
      else
        printf(" %d", ids_list_tmp[i]);
    }
    putchar('\n');
    fflush(stdout);
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
    m_record_list.clear();
    std::vector<ICDE18::Record> tmp_list;
    for (int i=0; i<m_ServerToSilos.size(); ++i) {
      m_ServerToSilos[i]->GetLocalRecord(tmp_list);
      m_record_list.insert(m_record_list.end(), tmp_list.begin(), tmp_list.end());
    }

    log.SetEndTimer();
    log.LogOneQuery(CommQueryAnswer(m_record_list));

    #ifdef LOCAL_DEBUG
    // printf("There are %d objects in the query range:\n", (int)m_record_list.size());
    // for (auto record : m_record_list) {
    //   if (record.has_p()) {
    //     printf("  ID = %d, location = (%.2f,%.2f)\n", 
    //             record.id(), record.p().x(), record.p().y());
    //   }
    // }
    // fflush(stdout);

    printf("%d\n", (int)m_record_list.size());
    std::vector<int> ids_list_tmp;
    for (auto record : m_record_list) {
      if (record.has_p()) {
        ids_list_tmp.emplace_back(record.id());
      }
    }
    for (int i=0; i<ids_list_tmp.size(); ++i) {
      if (i == 0)
        printf("%d", ids_list_tmp[i]);
      else
        printf(" %d", ids_list_tmp[i]);
    }
    putchar('\n');
    fflush(stdout);
    #endif
  }

  std::vector<std::shared_ptr<ServerToSilo>> m_ServerToSilos;
  std::vector<std::string> m_IPAddresses;
  std::vector<ICDE18::Record> m_record_list;
  std::vector<std::vector<size_t>> m_GridIndexCounts;
  QueryLogger log;
};

int main(int argc, char** argv) {
  // Expect only arg: --query_path=../../data/query.txt --ip_path=../../data/ip.txt
  #ifdef LOCAL_DEBUG
  std::cout << argc << std::endl;
  for (int i=0; i<argc; ++i)
    std::cout << argv[i] << std::endl;
  #endif

  std::string query_file = ICDE18::GetQueryFilePath(argc, argv);
  std::string ip_file = ICDE18::GetSiloIPFilePath(argc, argv);
  
  #ifdef LOCAL_DEBUG
  printf("--query_path=%s --ip_path=%s\n", query_file.c_str(), ip_file.c_str());
  fflush(stdout);
  #endif
  
  #ifdef LOCAL_DEBUG
  printf("[Connect] Server\n");
  fflush(stdout);
  #endif
  FedQueryServiceServer fedServer(ip_file);

  #ifdef LOCAL_DEBUG
  printf("-------------- Test Circle Range Query --------------\n");
  fflush(stdout);
  #endif
  fedServer.GetQueryAnswer_byGridIndex(query_file);


  return 0;
}

