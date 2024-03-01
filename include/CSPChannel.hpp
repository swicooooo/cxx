#pragma once

#include <condition_variable>
#include <deque>

// share memory by communicating
template<typename T>
class CSPChannel{
public:
    CSPChannel(std::size_t capacity=0):capacity_(capacity),stop_(false){}
    CSPChannel(const CSPChannel&) = delete;
    CSPChannel& operator=(const CSPChannel&) = delete;
    CSPChannel(CSPChannel&&) = default;
    ~CSPChannel(){}
    
    bool send(T data){
        std::unique_lock<std::mutex> lock(cv_mt_);
        cv_producer_.wait(lock,[this]{return capacity_==0&&queue_.empty() || queue_.size()<capacity_ || stop_;});
        if(stop_)
            return false;
        queue_.push_back(data);
        cv_consumer_.notify_one();
        return true;
    }

    bool recv(T& data){
        std::unique_lock<std::mutex> lock(cv_mt_);
        cv_consumer_.wait(lock,[this]{return !queue_.empty() || stop_;});
        if(stop_&&queue_.empty())
            return false;
        data=std::move(queue_.front());
        queue_.pop_front();
        cv_producer_.notify_one();
        return true;
    }
    void stop(){
        stop_ = true;
        cv_producer_.notify_all();
        cv_consumer_.notify_all();
    }
private:
    std::mutex cv_mt_;
    std::condition_variable cv_producer_;
    std::condition_variable cv_consumer_;
    std::deque<T> queue_;
    std::size_t capacity_;
    std::atomic_bool stop_;
};