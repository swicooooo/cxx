#include <iostream>
#include <chrono>
#include "ThreadPool.hpp"
#include "CSPChannel.hpp"
#include "atm/Dispatcher.hpp"
#include <functional>
#include "atm/ATM.hpp"
#include "file_transfer/AsyncServer.hpp"

void testThreadPool(){
    int m=0;
    ThreadPool::GetInstance().commit([](int& m){
        std::cout << "thread..." << std::endl;
        m=1024;
    },std::ref(m));
    std::this_thread::sleep_for(std::chrono::seconds(3));
}

void testCSPChannel(){
    CSPChannel<int> channel(10);
    std::thread t1([&](){
        for (int i = 0; i < 5; ++i) {
            channel.send(i);
            std::cout << "Sent: " << i << std::endl;
        }
        channel.stop();
    });
    std::thread t2([&](){
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        int data;
        while (channel.recv(data)) {
            std::cout << "Received: " << data << std::endl;
        } 
    });
    t1.join();
    t2.join();
}

struct Message1 {};
struct Message2 {};

void testDispatcher() {
    messaging::Receiver receiver;
    auto dispatcher = receiver.wait();

    auto handle_message1 = [](Message1 const& msg) {
        std::cout << "Handling Message1... " << std::endl;
    };
    auto handle_message2 = [](Message2 const& msg) {
        std::cout << "Handling Message2... " << std::endl;
    };
    // per node only process a piece of message
    std::thread t1([&]() {
        while (true) {
            dispatcher.handle<Message1>(handle_message1).handle<Message2>(handle_message2);
        }
    });

    messaging::Sender sender(receiver);
    std::thread t2([&](){sender.send(Message1{});});
    std::thread t3([&](){sender.send(Message2{});});

    t1.join();
    t2.join();
    t3.join();
}

void testATM(){
    BankMachine bank;
    InterfaceMachine interface;
    ATM atm(bank.getSender(), interface.getSender());

    std::thread t1(&BankMachine::run, &bank);
    std::thread t2(&InterfaceMachine::run, &interface);
    std::thread t3(&ATM::run, &atm);

    messaging::Sender atmqueue(atm.getSender());
    bool quit_pressed = false;
	while (!quit_pressed)
	{
		char c = getchar();
		switch (c)
		{
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			atmqueue.send(digit_pressed(c));
			break;
		case 'b':
			atmqueue.send(balance_pressed());
			break;
		case 'w':
			atmqueue.send(withdraw_pressed(50));
			break;
		case 'c':
			atmqueue.send(cancel_pressed());
			break;
		case 'q':
			quit_pressed = true;
			break;
		case 'i':
			atmqueue.send(card_inserted("acc1234"));
			break;
		}
	}

    bank.done();
    interface.done();
    atm.done();

    t1.join();
    t2.join();
    t3.join();
}

void testAsyncServeer() {
    try{
        AsyncServer server(8080);
        server.start();
        std::printf("Start[]: ...\n");
        server.join();
    }catch(std::exception& e){

    }
}

int main()
{
    testAsyncServeer();
}