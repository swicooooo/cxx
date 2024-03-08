#include <iostream>
#include <string>
#include "grpc/database.grpc.pb.h"
#include <grpcpp/grpcpp.h>

int main(){

    std::shared_ptr<grpc::Channel> channel= grpc::CreateChannel("localhost:50051",grpc::InsecureChannelCredentials());
    std::unique_ptr<Database::Stub> stub(Database::NewStub(channel));
    UserRequest req;
    req.set_uid(123);
    UserInfo resp;
    grpc::ClientContext context;
    grpc::Status status=stub->GetUserInfo(&context,req,&resp);
    if (status.ok()) {
        std::cout << "User Information:" << std::endl;
        std::cout << "Name: " << resp.name() << std::endl;
        std::cout << "Balance: " << resp.balance() << std::endl;
  } else {
        std::cout << "Error: " << status.error_code() << ": " << status.error_message() << std::endl;
  }
}