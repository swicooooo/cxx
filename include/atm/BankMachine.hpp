#pragma once

#include "Message.hpp"
#include "Dispatcher.hpp"
#include "WithdrawMsg.hpp"
#include <iostream>

class BankMachine{
public:
    BankMachine():balance_(199){}

    void run(){
        while(true){
            incoming_.wait().handle<verify_pin>([&](const verify_pin& msg){
                if(msg.pin=="0000")
                    msg.atm_queue.send(pin_verified());
                else
                    msg.atm_queue.send(pin_incorrect());

            }).handle<withdraw>([&](const withdraw& msg){
                if(this->balance_>=msg.amount){
                    msg.atm_queue.send(withdraw_ok());
                    this->balance_-=msg.amount;
                }
                else
                    msg.atm_queue.send(withdraw_denied());

            }).handle<get_balance>([&](const get_balance& msg){
                msg.atm_queue.send(::balance(this->balance_));

            }).handle<withdrawal_processed>([&](const withdrawal_processed&){
                std::cout << "withdrawal_processed !!!" << std::endl;

            }).handle<cancel_withdrawal>([&](const cancel_withdrawal& msg){
                this->balance_+=msg.amount;
                std::cout << "cancel withdraw !!!" << std::endl;
            });
        }
    }
    void done(){getSender().send(messaging::CloseQueue());}
    messaging::Sender getSender(){return incoming_;}
private:
    messaging::Receiver incoming_;
    uint balance_;
};