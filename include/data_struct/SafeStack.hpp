#pragma once

#include <mutex>
#include <stack>
#include <condition_variable>

template<typename T>
class SafeStack{
public:
    SafeStack(){}
    SafeStack(const SafeStack&) = delete;
    SafeStack& operator=(const SafeStack&) = delete;
    ~SafeStack(){}

    void push(T data){
        std::lock_guard<std::mutex> lock(mt_);
        stack_.push(std::make_shared<T>(std::move(data)));
        cv_lock_.notify_one();
    }
    void wait_pop(T& data){
        std::unique_lock<std::mutex> lock(mt_);
        cv_lock_.wait(lock, [this]{return !stack_.empty();});
        data=std::move(*stack_.top());
        stack_.pop();
    }
    bool try_pop(T& data){
        std::lock_guard<std::mutex> lock(mt_);
        if(stack_.empty())
            return false;
        data=std::move(*stack_.top());
        stack_.pop();
        return true;
    }
    bool empty(){
        std::lock_guard<std::mutex> lock(mt_);
        return stack_.empty();    
    }
private:
    std::mutex mt_;
    std::condition_variable cv_lock_;
    std::stack<std::shared_ptr<T>> stack_;
};