#pragma once

#include <string>
#include <unordered_map>
#include <boost/asio.hpp>
#include <coroutine>
#include <chrono>

#include "WebSocketSession.h"
#include "Global.h"

namespace asio = boost::asio;
using asio::ip::tcp;



/**
 * @brief 
    在 Boost.Asio 中，`io_context` 是负责任务调度的中心，而线程池的线程主要负责从 `io_context` 中获取任务并执行。
    当一个异步操作完成时，相关的回调会被放入 `io_context` 的任务队列中。
    线程池中的线程会不断地从 `io_context` 中获取这些任务并执行。
    因此，所有的任务被线程池处理，而不是被特定的线程直接处理。
    线程池中的每个线程都可以从 `io_context` 中获取任务执行，这就实现了任务的并发执行。
    而使用 `strand` 可以保证一些特定任务按照顺序执行，但不同的 `strand` 中的任务可能仍然并发执行。
* 
*/

class WebSocketServer {
public:
    WebSocketServer(std::size_t num_threads, std::uint16_t port)
        : acceptor_(io_context_, tcp::endpoint(tcp::v4(), port)),
          socket_(io_context_),
          strand_(io_context_.get_executor()),
          thread_pool_(num_threads) {

        asio::post(thread_pool_, [this]() {
            co_spawn(strand_, [this]() -> asio::awaitable<void> {
                while (true) {
                    co_await acceptor_.async_accept(socket_, asio::use_awaitable);
                    asio::post(strand_, [this]() {
                        // std::make_shared<WebSocketSession>(std::move(socket_), "localhost", "sw", "0", "transfer")->start();
                    });
                }
            }, asio::detached);

            thread_pool_.join();
        });
    }

    inline void run() {io_context_.run();}

private:
    asio::io_context io_context_;
    tcp::acceptor acceptor_;
    tcp::socket socket_;
    asio::strand<asio::io_context::executor_type> strand_;
    asio::thread_pool thread_pool_;
};