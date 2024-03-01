#pragma once

// #include <openssl/md5.h>
#include <chrono>
#include <string>
#include <iomanip>

class MyUtility {
public:
    static std::string sha256(const std::string& input) {
        return "";
    }

    static std::string generateUniqueId(uint16_t dataCenterId, uint16_t machineId) {
        if (dataCenterId > maxDataCenterId || machineId > maxMachineId) {
            throw std::invalid_argument("Invalid data center or machine ID");
        }

        std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
        auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();

        if (timestamp == lastTimestamp_) {
            sequence_ = (sequence_ + 1) & sequenceMask;
            if (sequence_ == 0) {
                // If the sequence overflows within the same millisecond, wait for the next millisecond
                while (timestamp <= lastTimestamp_) {
                    now = std::chrono::system_clock::now();
                    timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
                }
            }
        } else {
            sequence_ = 0;
        }
        lastTimestamp_ = timestamp;
        uint64_t uniqueId = ((timestamp - epoch) << timestampLeftShift) |
                            (dataCenterId << dataCenterIdShift) |
                            (machineId << machineIdShift) |
                            sequence_;

        return std::to_string(uniqueId);
    }

private:
    const static uint64_t epoch = 1609459200000; // 2021-01-01 00:00:00 UTC
    const static uint16_t maxDataCenterId = 31;
    const static uint16_t maxMachineId = 31;
    const static uint64_t sequenceMask = 4095;
    const static int64_t timestampLeftShift = 22;
    const static int64_t dataCenterIdShift = 17;
    const static int64_t machineIdShift = 12;

    static uint16_t sequence_;
    static uint64_t lastTimestamp_;
};

// Initialize static members
uint16_t MyUtility::sequence_ = 0;
uint64_t MyUtility::lastTimestamp_ = 0;
