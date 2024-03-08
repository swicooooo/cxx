#include "database.grpc.pb.h"

class DatabaseImpl final: public Database::Service{
    grpc::Status GetUserInfo(grpc::ServerContext* context, const UserRequest* request, UserInfo* response) override{

        // process database data
        response->set_name("John Doe");
        response->set_balance(100000);

        return grpc::Status::OK;
    }
};  