#pragma once

#include <mutex>
#include <memory>

template<typename T>
class SafeList{

struct Node
{
    std::mutex mt_;
    std::shared_ptr<T> data_;
    std::unique_ptr<Node> next_;

    Node():next_(){};
    Node(const T& data):data_(std::make_shared<T>(data)){};
};
public:
    SafeList():head_(new Node()){};
    ~SafeList(){
        removeIf([](Node const& node){return true;});
    }
    SafeList(const SafeList&) = delete;
    SafeList& operator=(const SafeList&) = delete;

    void pushFront(const T& value){
        std::unique_ptr<Node> new_node(new Node(value));
        std::lock_guard<std::mutex> lock(head_->mt_);
        new_node->next_=std::move(head_->next_);
        head_->next_=std::move(new_node);
        // only insert for the first
        if(head_->next_->next_==nullptr){
            std::lock_guard<std::mutex> lock(global_mt_);
            tail_=head_->next_.get();
        }
    }

    void pushBack(const T& value){
        std::unique_ptr<Node> new_node(new Node(value));
        std::unique_lock<std::mutex> lock(tail_->mt_);
        std::unique_lock<std::mutex> global_lock(global_mt_);
        tail_->next_=std::move(new_node);
        tail_=tail_->next_.get();
    }

    template<typename Predicate>
    std::shared_ptr<T> findFirstIf(Predicate p){
        Node* current=head_;
        std::unique_lock<std::mutex> next_lock(head_->mt_);
        while(auto next=current->next_.get()){
            std::unique_lock<std::mutex> lock(next->mt_);
            lock.unlock();
            if(p(*next->data_))
                return next->data_;
            current=next;
            lock=std::move(next_lock);
        }
        return std::shared_ptr<T>();
    }

    template<typename Function>
    void forEach(Function f){
        Node* current=head_;
        std::unique_lock<std::mutex> lock(head_->mt_);
        while(auto next=current->next_.get()){
            std::unique_lock<std::mutex> next_lock(next->mt_);
            lock.unlock();
            f(*next->data_);
            current=next;
            lock=std::move(next_lock);
        }
    }

    template<typename Predicate>
    void removeIf(Predicate p){
        std::unique_lock<std::mutex> lock(head_->mt_);
        Node* current=head_;
        while(auto next=current->next_.get()){
            std::unique_lock<std::mutex> next_lock(next->mt_);
            if(p(*next->data_)){
                current->next_.reset(std::move(next->next_.get()));
                next_lock.unlock();
                // only remove for the last
                if(current->next_==nullptr){
                    std::lock_guard<std::mutex> global_lock(global_mt_);
                    tail_=current;
                }
            }else{
                lock.unlock();
                current=next;
                lock=std::move(next_lock);
            }
        }
    }

    template<typename Predicate>
    bool removeFirstIf(Predicate p){
        std::unique_lock<std::mutex> lock(head_->mt_);
        Node* current=head_;
        while(auto next=current->next_.get()){
            std::unique_lock<std::mutex> next_lock(next->mt_);
            if(p(*next->data_)){
                current->next_.reset(std::move(next->next_.get()));
                // only remove for the last
                if(current->next_==nullptr){
                    std::lock_guard<std::mutex> global_lock(global_mt_);
                    tail_=current;
                }
                next_lock.unlock();
                return true;
            }
            lock.unlock();
            current=next;
            lock=std::move(next_lock);
        }
        return false;
    }

private:
    Node* head_; //Node* head_ is not hold entity nor a pointer
    Node* tail_;
    std::mutex global_mt_;
};