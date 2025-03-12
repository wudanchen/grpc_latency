#include <chrono>
#include <iostream>
#include <memory>
#include <string>
#include <grpcpp/grpcpp.h>
#include "latency.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using latency::LatencyTest;
using latency::TimeRequest;
using latency::TimeResponse;

class LatencyClient {
public:
  LatencyClient(std::shared_ptr<Channel> channel)
      : stub_(LatencyTest::NewStub(channel)) {}

  void MeasureLatency(int iterations) {    
    for(int i = 0; i < iterations; ++i) {
      TimeRequest request;
      TimeResponse response;
      ClientContext context;

      // 记录客户端发送时间
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
        
        std::cout << "[client->server->client] 往返时间差: " << round_trip << "us | "
                  << "[client->server] 单向时间差: " << server_processing << "us\n";
      } else {
        std::cerr << "RPC failed: " << status.error_message() << std::endl;
      }
    }
  }

private:
  std::unique_ptr<LatencyTest::Stub> stub_;
};

int main() {
  LatencyClient client(grpc::CreateChannel(
      "localhost:50051", grpc::InsecureChannelCredentials()));
  
  const int TEST_ITERATIONS = 10;
  client.MeasureLatency(TEST_ITERATIONS);
  return 0;
}