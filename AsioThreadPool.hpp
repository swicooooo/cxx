#pramga once

#include <boost/asio.hpp>
#include <thread>
#include <vector>
#include "Singleton.hpp"
using namespace boost;

class AsioThreadPool: public Singleton<AsioThreadPool>{
public:
    AsioThreadPool(std::size_t thread_num=4):next_io_context_(0){
        for(std::size_t i=0;i<thread_num;++i){
            work_guards_.emplace(std::make_unique<Work>(new Work(io_contexts_[i])));
        }

        for(std::size_t i=0;i<thread_num;++i){
            thread_pool_.emplace([&](){
                io_contexts_[i].run();
            });
        }
    }
    asio::io_context& getIoContext(){
        auto& io_context=io_contexts_[next_io_context_++];
        if(next_io_context_==io_contexts_.size())
            next_io_context_=0;
        return io_context;
    }
    void stop(){
        for(auto& work: work_guards_)
            work.reset();
        for(auto& thread: thread_pool_)
            thread.join();
    }
private:
    using Work=asio::executor_work_guard<asio::io_context::executor_type>;
    std::vector<asio::io_context> io_contexts_;
    std::vector<std::unique_ptr<Work> work_guards_;
    std::vector<std::thread> threads_;
    std::size_t next_io_context_;
};