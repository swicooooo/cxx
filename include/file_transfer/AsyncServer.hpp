#pragma once
#include <boost/asio.hpp>
#include <coroutine>
#include <iostream>
#include <memory>

#include "FileManager.hpp"

namespace asio = boost::asio;
using asio::ip::tcp;
class AsyncServer {
public:
    AsyncServer(uint16_t port)
        : io_context_(),
          work_guard_(io_context_.get_executor()),
          strand_(io_context_.get_executor()),
          acceptor_(io_context_, tcp::endpoint(tcp::v4(), port)),
          thread_pool_(std::max(1u, std::thread::hardware_concurrency() - 1)){
        acceptor_.set_option(asio::socket_base::reuse_address(true));}

    void start() {
        co_spawn(strand_, [&]()->asio::awaitable<void> {
            while(true) {
                tcp::socket socket = co_await acceptor_.async_accept(asio::use_awaitable);
                    co_spawn(strand_,[&]() -> asio::awaitable<void>{
                        co_await std::make_shared<FileManager>(io_context_,strand_)->handle_connection(std::move(socket));
                    },asio::detached);
                    std::printf("Connection[]: ...\n");
            }}, asio::detached);      
        asio::post(thread_pool_, [&]() { io_context_.run(); });
    }

    void join() {
        thread_pool_.join();
    }

private:
    asio::io_context io_context_;
    asio::strand<asio::io_context::executor_type> strand_;
    tcp::acceptor acceptor_;
    asio::thread_pool thread_pool_;
    asio::executor_work_guard<asio::io_context::executor_type> work_guard_;
};
