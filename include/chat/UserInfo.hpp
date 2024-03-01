#pragma once
#include <string>
#include <vector>
#include <chrono>

struct UserInfo {
    std::string username;
    std::string passwordHash;
    std::string status;
    std::vector<std::string> friends;
    std::chrono::system_clock::time_point createTime;
};