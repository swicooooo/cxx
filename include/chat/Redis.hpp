#include <iostream>
#include <string>
#include <hiredis/hiredis.h>

class RedisClient {
public:
    RedisClient() {
        // Connect to Redis server
        context_ = redisConnect("localhost", 6379);

        if (context_ == nullptr || context_->err) {
            if (context_) {
                std::cerr << "Connection error: " << context_->errstr << std::endl;
                redisFree(context_);
            } else {
                std::cerr << "Connection error: can't allocate redis context" << std::endl;
            }
            // Handle connection error
        }
    }

    ~RedisClient() {
        // Disconnect from Redis server
        if (context_) {
            redisFree(context_);
        }
    }

    bool insertContact(const std::string& uuid, const std::string& contactInfo) {
        redisReply* reply = (redisReply*)redisCommand(context_, "HSET contacts %s %s", uuid.c_str(), contactInfo.c_str());

        if (reply == nullptr) {
            std::cerr << "Failed to execute Redis command: HSET" << std::endl;
            return false;
        }

        bool success = (reply->type == REDIS_REPLY_INTEGER && reply->integer > 0);
        freeReplyObject(reply);
        return success;
    }

    std::string queryContact(const std::string& uuid) {
        redisReply* reply = (redisReply*)redisCommand(context_, "HGET contacts %s", uuid.c_str());

        if (reply == nullptr) {
            std::cerr << "Failed to execute Redis command: HGET" << std::endl;
            return "";
        }

        std::string result;
        if (reply->type == REDIS_REPLY_STRING) {
            result = reply->str;
        }

        freeReplyObject(reply);
        return result;
    }

private:
    redisContext* context_;
};
