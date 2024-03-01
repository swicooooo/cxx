#pragma once

#include "Message.hpp"
//per node only process one piece message
namespace messaging
{
class CloseQueue{};

template<typename PreviousDispatcher, typename Msg, typename Func>
class TemplateDispatcher
{
public:
    TemplateDispatcher(TemplateDispatcher&& other):
        queue_(other.queue_),f_(std::move(other.f_)),prev_(other.prev_),chained_(other.chained_)
            {other.chained_=true;}
    explicit TemplateDispatcher(Queue* queue, PreviousDispatcher* prev,Func&& f):
        queue_(queue),f_(std::forward<Func>(f)),prev_(prev),chained_(false){prev->chained_=true;}

    TemplateDispatcher(TemplateDispatcher const&) = delete;
	TemplateDispatcher& operator=(TemplateDispatcher const&) = delete;
    ~TemplateDispatcher() noexcept(false){
        if(!chained_) waitAndDispatcher();
    }

    template<typename otherMsg, typename otherFunc>
    TemplateDispatcher<TemplateDispatcher,otherMsg,otherFunc> handle(otherFunc&& otherf){
        return TemplateDispatcher<TemplateDispatcher,otherMsg,otherFunc>(queue_,this,std::forward<otherFunc>(otherf));
    }

private:
    template<typename Dispatcher, typename otherMsg,typename otherFunc>
    friend class TemplateDispatcher;

    Queue* queue_;
    Func f_;
    PreviousDispatcher* prev_;
    bool chained_;

    void waitAndDispatcher(){
        while(true) {
            auto msg=queue_->waitAndPop();
            if(dispatcher(msg))
                break;
        }
    }

    bool dispatcher(std::shared_ptr<MessageBase> const& msg){
        if(WrappedMessage<Msg>* wrapper=dynamic_cast<WrappedMessage<Msg>*>(msg.get())){
            f_(wrapper->content_);
            return true;
        }else
            return prev_->dispatcher(msg);
    }
    
};

class Dispatcher
{
public:
    Dispatcher(Dispatcher&& other):queue_(other.queue_),chained_(other.chained_){other.chained_=true;}
    explicit Dispatcher(Queue* queue):queue_(queue),chained_(false){}

    Dispatcher(const Dispatcher& other)=delete;
    Dispatcher& operator=(const Dispatcher& other)=delete;
    ~Dispatcher() noexcept(false){
        if(!chained_) waitAndDispatcher();
    }
    
    template<typename Msg, typename Func>
    TemplateDispatcher<Dispatcher,Msg,Func> handle(Func&& f){
        return TemplateDispatcher<Dispatcher,Msg,Func>(queue_,this,std::forward<Func>(f));
    }

private:
    template<typename Dispatcher, typename Msg, typename Func>
    friend class TemplateDispatcher;
    
    Queue* queue_;
    bool chained_;
    bool waitAndDispatcher(){
        while (true){
            auto msg=queue_->waitAndPop();
            dispatcher(msg);
        }   
    }

    bool dispatcher(std::shared_ptr<MessageBase> msg){
        if(dynamic_cast<WrappedMessage<CloseQueue>*>(msg.get())) 
            throw CloseQueue();
        return false;
    }
};

class Sender
{
public:
    Sender():queue(nullptr){}
    explicit Sender(Queue* queue):queue(queue){}

    template<typename Msg>
    void send(Msg const& msg){
        if(queue)
            queue->push(msg);
    }
private:
    Queue* queue;
};

class Receiver
{
public:
    operator Sender(){return Sender(&queue);}
    Dispatcher wait(){return Dispatcher(&queue);}
private:
    Queue queue;
};


}