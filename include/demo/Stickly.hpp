#pragma once

#include <iostream>
#include <boost/asio.hpp>

using namespace boost;
using boost::asio::ip::tcp;

class Session : public std::enable_shared_from_this<Session> {
public:
    Session(tcp::socket socket) : socket_(std::move(socket)) {}

    void start() {
        do_read();
    }

private:
    void do_read() {
         std::cout << "Received data start... " << data_ << std::endl;
        while (1)
        {
            memset(data_,0,max_length);
         std::this_thread::sleep_for(std::chrono::seconds(1));
         // build a struct message (data,total_len) to avoid sticky packages problem
         socket_.async_receive(asio::buffer(data_, std::rand()%4000),
             [this, self=shared_from_this()](std::error_code ec, std::size_t length) {
            
         });
         std::cout << "Received message:  " << data_ << "\n\n\n";
        
        }
        
    }

    tcp::socket socket_;
    enum { max_length = 4096 };
    char data_[max_length];
};

class Server {
public:
    Server(asio::io_context& io_context, short port)
        : acceptor_(io_context, tcp::endpoint(tcp::v4(), port)) {
        do_accept();
    }

private:
    void do_accept() {
        acceptor_.async_accept(
            [this](std::error_code ec, tcp::socket socket) {
                if (!ec) {
                    std::make_shared<Session>(std::move(socket))->start();
                }
                do_accept();
            });
    }

    tcp::acceptor acceptor_;
};

