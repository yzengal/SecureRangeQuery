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


#include "ICDE18.grpc.pb.h"
#include "global.h"
#include "AES.h"
#include "grid.hpp"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerReader;
using grpc::ServerReaderWriter;
using grpc::ServerWriter;
using grpc::Status;
using google::protobuf::Empty;
using ICDE18::Record_t;
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
using ICDE18::QueryLogger;
using ICDE18::FedQueryService;
using std::chrono::system_clock;
using INDEX::GridIndex;


class Silo {
using COUNT_TYPE = int;

public:
    Silo(const int _siloID=0, const std::string& fileName="", const float _epsilon=1.0) : siloID(_siloID) {
        SetDataRecord(fileName);
        SetGridIndex(_epsilon);
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

    size_t GetK() {
        return m_grid_ptr->GetK();
    }

    void GetMins(std::vector<float>& _mins) {
        m_grid_ptr->GetMins(_mins);
    }

    void GetMaxs(std::vector<float>& _maxs) {
        m_grid_ptr->GetMaxs(_maxs);
    }

    void GetWidths(std::vector<float>& _widths) {
        m_grid_ptr->GetWidths(_widths);
    }

    void GetIndexCounts(std::vector<size_t>& _counts) {
        m_grid_ptr->publish_index_counts(_counts);
    }

    void SetFileterGridIDs(const std::vector<size_t>& grid_list) {
        m_grid_id_list.clear();
        m_grid_id_list.insert(m_grid_id_list.end(), grid_list.begin(), grid_list.end());
    }

    void GetFilterGridRecord(std::vector<Record_t>& ans) {
        ans.clear();
        std::default_random_engine rng;

        for (size_t gid : m_grid_id_list) {
            COUNT_TYPE perturb_count = m_grid_ptr->get_index_perturb_count(gid);
            COUNT_TYPE true_count = m_grid_ptr->get_index_true_count(gid);
            
            #ifdef LOCAL_DEBUG
            printf("gid = %zu, perturb_count = %d, true_count = %d", gid, perturb_count, true_count);
            fflush(stdout);
            assert(perturb_count != 0);
            #endif
            if (perturb_count < 0) {
                perturb_count = -perturb_count; // equivalent to overflow array
            }
            
            std::vector<Record_t> record_list_tmp;
            m_grid_ptr->get_index_record(gid, record_list_tmp);
            
            #ifdef LOCAL_DEBUG
            printf(", record_list_tmp.size() = %zu", record_list_tmp.size());
            fflush(stdout);
            assert(record_list_tmp.size() == true_count);
            #endif
            
            if (perturb_count < true_count) {// randomly remove some record
                std::shuffle(record_list_tmp.begin(), record_list_tmp.end(), rng);
                /**
                ** The following line is based on the original paper, 
                ** remove some record when meeting negative noise in the grid cunt
                **/
                record_list_tmp.resize(perturb_count);
            } else if (perturb_count > true_count) {
                ICDE18::Record_t dummy_record_tmp(-1, -1e8, -1e8);
                for (size_t i=true_count; i<perturb_count; ++i) {
                    record_list_tmp.emplace_back(dummy_record_tmp);
                }
            }
            ans.insert(ans.end(), record_list_tmp.begin(), record_list_tmp.end());

            #ifdef LOCAL_DEBUG
            assert(perturb_count == record_list_tmp.size());
            printf(", record_list_local.size() = %zu", record_list_tmp.size());
            printf(", ans.size() = %zu\n", ans.size());
            fflush(stdout);
            #endif
        }
        std::shuffle(ans.begin(), ans.end(), rng);
    }

private:
    void SetGridIndex(float epsilon) {
        std::shared_ptr<std::vector<ICDE18::Record_t>> data_ptr = std::make_shared<std::vector<ICDE18::Record_t>>(this->data);
        m_grid_ptr = std::make_unique<GridIndex<GRID_NUM_PER_SIDE>>(data_ptr);
        m_grid_ptr->perturb_index_counts(epsilon);
    }

    int siloID;
    QueryLogger log;
    std::vector<Record_t> data;
    std::vector<size_t> m_grid_id_list;
    std::string siloIP;
    std::unique_ptr<GridIndex<GRID_NUM_PER_SIDE>> m_grid_ptr;
};

class FedQueryServiceImpl final : public FedQueryService::Service {
public:
    explicit FedQueryServiceImpl(const int siloID, const std::string& fileName) {
        m_silo = std::make_unique<Silo>(siloID, fileName);  
        
        const size_t n_keys = 256 / 8;
        m_EncryptKeys.resize(n_keys);

        for (size_t i=0; i<n_keys; ++i) {
            m_EncryptKeys[i] = rand() % (1 + UCHAR_MAX);
        }
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


    Status GetGridIndex(ServerContext* context,
                        const Empty* request,
                        GridIndexCounts* grid_counts) override {
        #ifdef LOCAL_DEBUG
        auto startTime = std::chrono::steady_clock::now();
        #endif

        std::vector<float> _mins, _maxs, _widths;
        m_silo->GetMins(_mins);
        m_silo->GetMaxs(_maxs);
        m_silo->GetWidths(_widths);

        #ifdef LOCAL_DEBUG
        std::cout << _mins.size() << " " << _maxs.size() << " " << _widths.size() << std::endl;
        assert(_mins.size()==_maxs.size() && _widths.size()==_maxs.size());
        #endif

        FloatVector mins, maxs, widths;
        ICDE18::CopyFromVector<float>(mins, _mins);
        ICDE18::CopyFromVector<float>(maxs, _maxs);
        ICDE18::CopyFromVector<float>(widths, _widths);

        #ifdef LOCAL_DEBUG
        std::cout << mins.size() << " " << maxs.size() << " " << widths.size() << std::endl;
        assert(mins.size()==maxs.size() && maxs.size()==widths.size());
        #endif

        std::vector<size_t> _counts;
        m_silo->GetIndexCounts(_counts);
        IntVector counts;
        ICDE18::CopyFromVector<size_t>(counts, _counts);
        
        grid_counts->set_k(m_silo->GetK());
        grid_counts->mutable_mins()->CopyFrom(mins);
        grid_counts->mutable_maxs()->CopyFrom(maxs);
        grid_counts->mutable_widths()->CopyFrom(widths);
        grid_counts->mutable_counts()->CopyFrom(counts);

        log.LogAddComm(grid_counts->ByteSizeLong());

        #ifdef LOCAL_DEBUG
        auto endTime = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
        float runTime = duration.count();
        printf("Silo %d: PublishGridIndex, index.length()=%d, comm=%.0lf, time=%.2f\n", 
                m_silo->GetSiloID(), grid_counts->k(), (float)grid_counts->ByteSizeLong(), runTime);
        fflush(stdout);
        #endif

        return Status::OK;
    }

    Status SendFilterGridIndex(ServerContext* context, const IntVector* request, 
        Empty* response) override {
        std::vector<size_t> grid_ids_list;

        for (size_t i=0, sz=request->size(); i<sz; ++i) {
            grid_ids_list.emplace_back(request->values(i));
        }
        m_silo->SetFileterGridIDs(grid_ids_list);
        
        log.LogAddComm(request->ByteSizeLong());

        return Status::OK;
    }

    Status GetFilterGridRecord(ServerContext* context,
                        const Empty* circle,
                        ServerWriter<Record>* writer) override {
        Record record;
        m_RecordVector.clear();

        #ifdef LOCAL_DEBUG
        printf("Silo %d: GetFilterGridRecord START\n", m_silo->GetSiloID());
        fflush(stdout);
        std::this_thread::sleep_for(std::chrono::seconds(1));
        #endif

        std::vector<Record_t> ans;
        m_silo->GetFilterGridRecord(ans);

        #ifdef LOCAL_DEBUG
        printf("Silo %d: GetFilterGridRecord DONE\n", m_silo->GetSiloID());
        fflush(stdout);
        #endif
        for (auto record_ : ans) {
           m_RecordVector.emplace_back(record_);
           record = MakeRecord(record_);
           writer->Write(record);
        }
        log.LogOneQuery(record.ByteSizeLong() * ans.size());

        #ifdef LOCAL_DEBUG
        printf("There are %zu objects in the query range:\n", ans.size());
        for (auto record_ : ans) {
            printf("  ID = %d, location = (%.2f,%.2f)\n", 
                    record_.ID, record_.x, record_.y);
        }
        fflush(stdout);
        #endif  

        return Status::OK;
    }

    Status GetFilterGridEncryptRecord(ServerContext* context,
                        const Empty* empty_request,
                        ServerWriter<EncryptRecord>* writer) override {
        EncryptRecord record;
        int record_id = 0;
        m_RecordVector.clear();

        std::vector<Record_t> ans;
        m_silo->GetFilterGridRecord(ans);

        for (auto record_ : ans) {
           m_RecordVector.emplace_back(record_);
           record = MakeEncryptRecord(record_id, record_);
           writer->Write(record);
           record_id++;
        }
        log.LogOneQuery(record.ByteSizeLong() * ans.size());

        return Status::OK;
    }

    Status GetEncryptKeys(ServerContext* context,
                        const Empty* empty_request,
                        ByteVector* bytes) override {
        const size_t n_keys = m_EncryptKeys.size();

        std::string data_str(reinterpret_cast<const char*>(m_EncryptKeys.data()), m_EncryptKeys.size());
        bytes->set_size(n_keys);
        bytes->set_values(data_str);

        log.LogAddComm(bytes->ByteSizeLong());

        return Status::OK;
    }

    Status AnswerCircleRangeQuery(ServerContext* context,
                        const ICDE18::Circle* circle,
                        ServerWriter<Record>* writer) override {
        log.SetStartTimer();
        Record record;
        m_RecordVector.clear();

        #ifdef LOCAL_DEBUG
        printf("Silo %d: CircleRangeQuery, center=(%.2f,%.2f), rad=%.2f\n", 
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
        log.Print();
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

    EncryptRecord MakeEncryptRecord(const int _id, const Record_t& r) {
        AES aes(AESKeyLength::AES_256);

        std::vector<unsigned char> plain_data = SerializeRecord(r);
        std::vector<unsigned char> encrypt_data = aes.EncryptECB(plain_data, m_EncryptKeys);
        std::string data_str(reinterpret_cast<const char*>(encrypt_data.data()), encrypt_data.size());
       
        EncryptRecord ret;
        ret.set_id(_id);
        ret.set_data(data_str);
        
        return ret;
    }

    std::vector<Record_t> m_RecordVector;
    std::vector<unsigned char> m_EncryptKeys;
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

