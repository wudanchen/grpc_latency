#include <chrono>
#include <iostream>
#include <memory>
#include <string>
#include <grpcpp/grpcpp.h>
#include "latency.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using latency::LatencyTest;
using latency::TimeRequest;
using latency::TimeResponse;

class LatencyServiceImpl final : public LatencyTest::Service {
  Status MeasureTime(ServerContext* context, 
                     const TimeRequest* request,
                     TimeResponse* response) override {
    // 记录服务器接收时间
    auto server_receive = std::chrono::system_clock::now().time_since_epoch();
    int64_t server_receive_us = 
        std::chrono::duration_cast<std::chrono::microseconds>(server_receive).count();
    
    response->set_server_receive_time(server_receive_us);
    response->set_payload(request->payload());
    return Status::OK;
  }
};

void RunServer() {
  std::string server_address("0.0.0.0:50051");
  LatencyServiceImpl service;

  ServerBuilder builder;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);

  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;
  server->Wait();
}

int main() {
  RunServer();
  return 0;
}