#include "grpc/DataBaseImpl.hpp"
#include <grpcpp/grpcpp.h>

int main() {
    DatabaseImpl service;
    grpc::ServerBuilder builder;
    builder.AddListeningPort("127.0.0.1:50051",grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    std::cout << "Server listening on " << grpc::server_address << std::endl; 
    server->Wait();

    return 0;
}