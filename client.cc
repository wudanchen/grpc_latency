#include <chrono>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <grpcpp/grpcpp.h>
#include "latency.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using latency::LatencyTest;
using latency::TimeRequest;
using latency::TimeResponse;
using namespace std;

class LatencyClient {
public:
  LatencyClient(std::shared_ptr<Channel> channel)
    : stub_(LatencyTest::NewStub(channel)) {}

  void MeasureLatency(int payload_size, int iterations) {    
    for(int i = 0; i < iterations; ++i) {
      TimeRequest request;
      TimeResponse response;
      ClientContext context;

      request.set_payload(string(payload_size, 'a'));
      auto client_send = std::chrono::system_clock::now().time_since_epoch();
      int64_t client_send_us = 
          std::chrono::duration_cast<std::chrono::microseconds>(client_send).count();
      request.set_client_send_time(client_send_us);

      // 发起RPC调用
      Status status = stub_->MeasureTime(&context, request, &response);

      if(status.ok()) {
        // 计算端到端延迟
        auto client_receive = std::chrono::system_clock::now().time_since_epoch();
        int64_t client_receive_us = 
            std::chrono::duration_cast<std::chrono::microseconds>(client_receive).count();
        
        auto round_trip = client_receive_us - client_send_us;
        auto server_processing = response.server_receive_time() - client_send_us;
        const auto client_receive_ms = static_cast<double>(client_receive_us) / 1000;
        cout << "start time: " << to_string(client_receive_ms) << "ms, response size: " << response.payload().size() 
              << ", [client->server->client] 往返时间差: " << round_trip << "us | "
                  << "[client->server] 单向时间差: " << server_processing << "us\n";
      } else {
        cerr << "RPC failed: " << status.error_message() << std::endl;
      }
      
      std::this_thread::sleep_for(std::chrono::microseconds(1000));
    }
  }

private:
  std::unique_ptr<LatencyTest::Stub> stub_;
};

int main(int argc, char *argv[]) {
  if (argc != 3) {
    cout << "client need 3 params." << endl;
    return 0;
  }

  LatencyClient client(grpc::CreateChannel(
      "localhost:50051", grpc::InsecureChannelCredentials()));
  
  const auto payload_size = stoi(argv[1]);
  const auto iterations = stoi(argv[2]);
  cout << "初始化参数，传输字节数：" << payload_size << "B，发送循环次数：" << iterations << "次" << endl;
  client.MeasureLatency(payload_size, iterations);
  return 0;
}