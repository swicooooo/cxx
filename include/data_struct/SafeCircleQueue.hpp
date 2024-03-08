#pragma once

#include <memory>
#include <atomic>
#include <mutex>
#include <iostream>

namespace safequeue {

template<typename T, std::size_t Cap>
class SafeCircleQueue: private std::allocator<T> {
public:
    SafeCircleQueue():max_size_(Cap+1),data_(allo_traits.allocate(allocator_,max_size_)),head_(0),tail_(0){}
    SafeCircleQueue(const SafeCircleQueue&) = delete;
    SafeCircleQueue& operator=(const SafeCircleQueue&) = delete;
    ~SafeCircleQueue(){
        while (head_ != tail_) {
            allo_traits.destroy(allocator_,data_+head_);
        }
        allo_traits.deallocate(allocator_,data_,max_size_);
        std::cout << "~SafeCircleQueue()" << std::endl;
    }
    template<typename... Args>
    bool emplace(Args&&... args) {
        std::lock_guard<std::mutex> lock(mt_);
        if((tail_+1)%max_size_==head_){
            std::cout << "circular que full ! " << std::endl;
            return false;
        }
        allo_traits.construct(allocator_,data_+tail_,std::forward<Args>(args)...);
        std::cout << "called emplace " << std::endl;
        tail_ = (tail_+1)%max_size_;
        return true;
    }   
    bool pop(T& data) {
        std::lock_guard<std::mutex> lock(mt_);
        if(head_==tail_){
            std::cout << "circular que empty ! " << std::endl;
            return false;
        }
        data=std::move(data_[head_]);
        std::cout << "called pop " << std::endl;
        head_=(head_+1)%max_size_;
        return true;
    }

private:
    std::mutex mt_;
    std::size_t max_size_;
    T* data_;
    std::size_t head_;
    std::size_t tail_;
    std::allocator<T> allocator_;
    std::allocator_traits<std::allocator<T>> allo_traits;
};

template<typename T, std::size_t Cap>
class SafeCircleQueNOLock: private std::allocator<T> {
public:
    SafeCircleQueNOLock():max_size_(Cap+1),data_(allo_traits.allocate(allocator_,max_size_)),head_(0),tail_(0),tail_up_(0){}
    SafeCircleQueNOLock(const SafeCircleQueNOLock&) = delete;
    SafeCircleQueNOLock& operator=(const SafeCircleQueNOLock&) = delete;
    ~SafeCircleQueNOLock(){

        while (head_ != tail_) {
            allo_traits.destroy(allocator_,data_+head_);
        }
        allo_traits.deallocate(allocator_,data_,max_size_);
        std::cout << "~SafeCircleQueNOLock()" << std::endl;
    }
    template<typename... Args>
    bool emplace(Args&&... args) {
        std::size_t tail;
        do
        {   
            tail=tail_.load();
            if((tail+1)%max_size_==head_){
                std::cout << "circular que full ! " << std::endl;
                return false;
            }
        } while (!tail_.compare_exchange_strong(tail,(tail+1)%max_size_));

        allo_traits.construct(allocator_,data_+tail,std::forward<Args>(args)...);
        std::cout << "called emplace " << std::endl;

        std::size_t tail_up;
        do
        {
            tail_up=tail;
        } while (!tail_up_.compare_exchange_strong(tail_up,(tail_up+1)%max_size_));
        return true;
    }   
    bool pop(T& data) {
        std::size_t head;
        do
        {
            head=head_.load();
            if(head==tail_){
                std::cout << "circular que empty ! " << std::endl;
                return false;
            }
            if(head==tail_up_){
                std::cout << "circular que old data on tail with head ! " << std::endl;
                return false;
            }
            data=data_[head];
        } while (!head_.compare_exchange_strong(head,(head_+1)%max_size_));
        std::cout << "called pop " << std::endl;
        return true;
    }

private:
    std::size_t max_size_;
    T* data_;
    std::atomic_size_t head_;
    std::atomic_size_t tail_;
    std::atomic_size_t tail_up_; //avoid old data when work on the front of queue in multithread
    std::allocator<T> allocator_;
    std::allocator_traits<std::allocator<T>> allo_traits;
};
}