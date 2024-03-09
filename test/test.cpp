#include <iostream>
#include <chrono>
#include <set>
#include "ThreadPool.hpp"
#include "CSPChannel.hpp"
#include "atm/Dispatcher.hpp"
#include <functional>
#include "atm/ATM.hpp"
#include "file_transfer/AsyncServer.hpp"
#include "data_struct/SafeCircleQueue.hpp"
#include "data_struct/SafeStack.hpp"
#include "data_struct/SafeQueue.hpp"
#include "data_struct/SafeMap.hpp"
#include "data_struct/SafeList.hpp"
#include "asio/Singleton.h"
#include "AsioThreadPool.hpp"

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

void testSafeQueue() {
    safequeue::SafeCircleQueue<int,5> queue;
    queue.emplace(0);
    std::thread t1([&](){
        for (size_t i = 0; i < 5; i++)
        {
            auto res=queue.emplace(i);
            if(!res){
                std::cout << "error emplace" << std::endl;
                break;
            }
        }
    });

    std::thread t2([&](){
        for (size_t i = 0; i < 5; i++)
            {
                int c   ;
                auto res=queue.pop(c);
                if(!res){
                    std::cout << "error pop" << std::endl;
                    break;
                }
            } 
    });
    t1.join();
    t2.join();

}
void testSafeStack(){
    SafeStack<int> stack;
    stack.push(0);
    std::thread t1([&](){
        for (size_t i = 0; i < 5; i++)
        {
            stack.push(i);
        }
    });

    std::thread t2([&](){
        for (size_t i = 0; i < 5; i++)
        {
            int c;
            stack.wait_pop(c);
            std::cout << "pop: " << c << std::endl;
        } 
    });
    t1.join();
    t2.join();
}
void testSafeQueueEnhance(){
    safequeue::SafeQueueEnhance<int> queue;
    queue.push(0);
    std::cout << "push: 0" << std::endl;
    std::thread t1([&](){
        for (size_t i = 1; i < 5; i++)
        {
            queue.push(i);
            std::cout << "push: " << i << std::endl;
        }
    });

    std::thread t2([&](){
        for (size_t i = 0; i < 5; i++)
        {
            int c;
            queue.wait_pop(c);
            std::cout << "pop: " << c << std::endl;
        } 
    });
    
    t1.join();
    t2.join();
}
class MyClass
{
public:
    MyClass(int i):_data(i){}
    MyClass() = default;

    friend std::ostream& operator << (std::ostream& os, const MyClass& mc){
        os << mc._data;
        return os;
    }
    int GetData(){return _data;}

private:
    int _data;
};
void testSafeMap(){
    std::set<int> removeSet;
    SafeMap<int, std::shared_ptr<MyClass>> map;

    std::thread t1([&](){
       for(int i=0;i<100;++i){
            map.addOrUpdateFor(i,std::make_shared<MyClass>(i));
       } 
    });
    std::thread t2([&](){
         for(int i=0;i<100;){
            auto value=map.valueFor(i,nullptr);
            if(value){
                map.removeFor(i);
                removeSet.insert(i++);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
       } 
    });
    std::thread t3([&](){
         for(int i=100;i<200;++i){
            map.addOrUpdateFor(i,std::make_shared<MyClass>(i));
       } 
    });
    t1.join();
    t2.join();
    t3.join();

    for(auto& i: removeSet)
         std::cout << "remove data is " << i << std::endl;
    for(auto& i: map.getMap())
        std::cout << "copy data is " << *(i.second) << std::endl;
}

void testSafeList(){
    SafeList<MyClass> list;
    std::set<int> removeSet;
    std::thread t1([&](){
          for (int i = 0; i < 20000; i++)
            {
                MyClass mc(i);
                list.pushFront(mc);
                std::cout << "push front " << i << " success" << std::endl;
            }
    });
    std::thread t2([&](){
        for (int i = 2000; i < 40000; i++)
            {
                MyClass mc(i);
                list.pushBack(mc);
                std::cout << "push back " << i << " success" << std::endl;
            }
    });

    std::thread t3([&](){
         for(int i = 0; i < 40000; )
            {
                bool rmv_res = list.removeFirstIf([&](MyClass& mc)
                    {
                        return mc.GetData() == i;
                    });

                if(!rmv_res)
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    continue;
                }
                i++;
            }
    });

    t1.join();
    t2.join();
    t3.join();
    std::cout << "begin for each print...." << std::endl;
    list.forEach([](MyClass& mc)
        {
            std::cout << "for each print " << mc << std::endl;
        });
    std::cout << "end for each print...." << std::endl;
}
class S: public Singleton<S>
{};
void testSingleton(){
    auto s1=S::GetInstance();
    auto s2=S::GetInstance();
    std::cout << "start...." << s1.get() << "   " << s2.get() << std::endl;
}
void work(asio::io_context& io_context, int id){
     asio::dispatch(io_context, [id]() {
        std::cout << "Worker " << id << " started on thread " << std::this_thread::get_id() << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(2));
        std::cout << "Worker " << id << " finished on thread " << std::this_thread::get_id() << std::endl;
    });
}
void testAsioTP(){
    auto pool=AsioThreadPool::GetInstance();
    for (int i = 0; i < 8; ++i) {
        auto& ioc=pool->getIoContext();
        work(ioc, i);
    }
    std::this_thread::sleep_for(std::chrono::seconds(5));
    work(pool->getIoContext(), 8);
    pool->stop();
}
int main()
{
    testAsioTP();
}