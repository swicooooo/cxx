#pragma once

#include <mutex>
#include <condition_variable>
#include <queue>
#include <future>
#include <iostream>
#include <functional>
#include <iostream>
#include <thread>

class ThreadPool {
public:
    static ThreadPool& GetInstance(){
        static ThreadPool instance;
        return instance;
    }
    ThreadPool(const ThreadPool&)=delete;
    ThreadPool& operator=(const ThreadPool&)=delete;
    ThreadPool(ThreadPool&&)=delete;
    ~ThreadPool(){stop();}

    // build and commit task
    template<typename F, typename... Args>
    auto commit(F&& f, Args&&... args)->std::future<decltype(f(args...))>{
        using RetType=decltype(f(args...));
        if(stop_.load())
            return std::future<RetType>{};
        auto task=std::make_shared<std::packaged_task<RetType()>>
            (std::bind(std::forward<F>(f), std::forward<Args>(args)...));
        auto future=task->get_future();
        {
            std::lock_guard<std::mutex> lock(cv_mt_);
            tasks_.emplace([task]{(*task)();});
        }
        cv_lock_.notify_one();
        return future;
    }

    using Task=std::packaged_task<void()>;
private:
    std::mutex cv_mt_;
    std::condition_variable cv_lock_;
    std::atomic_bool stop_;
    std::atomic_int thread_num_;
    std::queue<Task> tasks_;
    std::vector<std::thread> threads_;

    ThreadPool():stop_(false){
        thread_num_=std::thread::hardware_concurrency()>1?std::thread::hardware_concurrency():1;
        start();
    }
    
    void start(){
        for(int i=0;i<thread_num_;++i) {
            threads_.emplace_back([this](){
                while(!stop_.load()) {
                    Task task;
                    {
                        std::unique_lock<std::mutex> lock(cv_mt_);
                        cv_lock_.wait(lock,[this]{return stop_.load() || !tasks_.empty();});
                        if(tasks_.empty())
                            return;
                        task=std::move(tasks_.front());
                        tasks_.pop();
                    }
                    thread_num_--;
                    task();
                    thread_num_++;
                }
            });
        }
    }
    void stop(){
        stop_.store(true);
        cv_lock_.notify_all();
        for(auto& thread: threads_) {
            if(thread.joinable()){
                std::cout << "join thread " << thread.get_id() << std::endl;
                thread.join();
            }
        }
    }

};