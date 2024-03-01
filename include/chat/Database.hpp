#pragma once

#include <iostream>
#include <string>
#include <memory>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>  // 引入彩色输出的 sink
#include <mysqlx/xdevapi.h>
#include <iomanip>

#include "UserInfo.hpp"

class Database {
public:
    Database(const std::string& host, const std::string& user, const std::string& password, const std::string& database)
        : host_(host), user_(user), password_(password), database_(database),
        session_("localhost", 3306, "sw", "0"), table(session_.getSchema("transfer").getTable("user_log")) {
            if(!table.existsInDatabase()){
                logger_->error("ERROR: DB_TABLE IS NOT EXIST!");
            }
    }

    bool registerUser(const UserInfo& userInfo) {
        try {
            std::time_t time = std::chrono::system_clock::to_time_t(userInfo.createTime);
            std::stringstream ss;
            ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
            // Insert a new user into the 'user_log' table
            table.insert("username", "password_hash", "status", "create_time")
                .values(userInfo.username, userInfo.passwordHash, userInfo.status, ss.str())
                .execute();

            logger_->info("User registered successfully: {}", userInfo.username);
            return true;
        } catch (const mysqlx::Error& e) {
            logger_->error("MySQL X DevAPI Error: {}", e.what());
            return false;
        }
    }

    bool loginUser(const std::string& username, const std::string& passwordHash) {
        try {
            // Search for the user in the 'user_log' table
            mysqlx::RowResult result = table.select("username")
                .where("username = :username AND password_hash = :password_hash")
                .bind("username", username)
                .bind("password_hash", passwordHash)
                .execute();

            if (result.count() > 0) {
                logger_->info("User login successful: {}", username);
                return true;
            } else {
                logger_->info("Invalid username or password.");
                return false;
            }
        } catch (const mysqlx::Error& e) {
            logger_->error("MySQL X DevAPI Error: {}", e.what());
            return false;
        }
    }

private:
    std::string host_;
    std::string user_;
    std::string password_;
    std::string database_;
    mysqlx::Session session_;
    mysqlx::Table table;

    // 创建一个 spdlog logger，输出到控制台并支持彩色显示
    std::shared_ptr<spdlog::logger> logger_ = spdlog::stdout_color_mt("DatabaseLogger");
};
