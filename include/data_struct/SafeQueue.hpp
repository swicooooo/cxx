#pragma once

#include <mutex>
#include <queue>
#include <condition_variable>

namespace safequeue{
template<typename T>
class SafeQueue{
public:
    SafeQueue(){}
    SafeQueue(const SafeQueue&) = delete;
    SafeQueue& operator=(const SafeQueue&) = delete;
    ~SafeQueue(){}

    void push(T data){
        std::lock_guard<std::mutex> lock(mt_);
        queue_.emplace(std::make_shared<T>(std::move(data)));
        cv_lock_.notify_one();
    }
    void wait_pop(T& data){
        std::unique_lock<std::mutex> lock(mt_);
        cv_lock_.wait(lock, [this]{return !queue_.empty();});
        data=std::move(*queue_.front());
        queue_.pop();
    }
    bool try_pop(T& data){
        std::lock_guard<std::mutex> lock(mt_);
        if(queue_.empty())
            return false;
        data=std::move(*queue_.front());
        queue_.pop();
        return true;
    }
    bool empty(){
        std::lock_guard<std::mutex> lock(mt_);
        return queue_.empty();    
    }
private:
    std::mutex mt_;
    std::condition_variable cv_lock_;
    std::queue<std::shared_ptr<T>> queue_;
};

template<typename T>
class SafeQueueEnhance{
public:
    struct QueueNode
    {
        std::shared_ptr<T> data;
        std::unique_ptr<QueueNode> next;
    };

    SafeQueueEnhance(): head_(new QueueNode), tail_(head_.get()){}
    SafeQueueEnhance(const SafeQueueEnhance&) = delete;
    SafeQueueEnhance& operator=(const SafeQueueEnhance&) = delete;
    ~SafeQueueEnhance(){}

    void push(T data){
        auto new_tail=std::make_unique<QueueNode>();
        QueueNode* tail=new_tail.get();
        std::lock_guard<std::mutex> lock(mt_tail_);
        tail_->data=std::make_shared<T>(std::move(data));
        tail_->next=std::move(new_tail);
        tail_=tail;
    }
    void wait_pop(T& data){
        std::unique_lock<std::mutex> lock(mt_head_);
        cv_lock_.wait(lock,[this]{return head_.get()!=tail_;});
       pop(data);
    }
    bool try_pop(T& data){
        std::unique_lock<std::mutex> lock(mt_head_);
        if(head_.get()==tail_)
            return false;
        pop(data); 
        return true;
    }
    void pop(T& data){
        std::unique_ptr<QueueNode> old_head=std::move(head_);
        data=*old_head->data;
        head_=std::move(old_head->next);
    }

private:
    std::mutex mt_head_;
    std::mutex mt_tail_;
    std::unique_ptr<QueueNode> head_;
    QueueNode* tail_;
    std::condition_variable cv_lock_;
};
}
