#pragma once

#include <mutex>
#include <condition_variable>
#include <queue>

namespace messaging {

struct MessageBase
{
    virtual ~MessageBase()=default;
};

template<typename Msg>
struct WrappedMessage: public MessageBase
{   
    explicit WrappedMessage(Msg const& content):content_(content){}
    Msg content_;
};

class Queue
{
public:
    template<typename T>
    void push(const T& data){
        std::lock_guard<std::mutex> lock(cv_mt_);
        queue_.push(std::make_shared<WrappedMessage<T>>(data));
        cv_lock_.notify_all();
    }

    std::shared_ptr<MessageBase> waitAndPop(){
        std::unique_lock<std::mutex> lock(cv_mt_);
        cv_lock_.wait(lock, [this]{return !queue_.empty();});
        auto msg = queue_.front();
        queue_.pop();
        return msg;
    }
private:
    std::mutex cv_mt_;
    std::condition_variable cv_lock_;
    std::queue<std::shared_ptr<MessageBase>> queue_;
};
}