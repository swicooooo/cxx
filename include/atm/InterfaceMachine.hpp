#pragma once

#include "Message.hpp"
#include "Dispatcher.hpp"
#include "WithdrawMsg.hpp"
#include <iostream>

class InterfaceMachine {
public:
    void run() {
        while (true){
            incoming.wait().handle<issue_money>([&](const issue_money& msg){
                std::lock_guard<std::mutex> lk(mt_);
                std::cout << "Issuing "<< msg.amount << std::endl;

            }).handle<display_insufficient_funds>([&](const display_insufficient_funds&){
                std::lock_guard<std::mutex> lk(mt_);
                std::cout << "Insufficient funds" << std::endl;

            }).handle<display_enter_card>([&](const display_enter_card&){
                std::lock_guard<std::mutex> lk(mt_);
                std::cout<< "Please enter your card (I)"<< std::endl;

            }).handle<display_enter_pin>([&](const display_enter_pin&){
                std::lock_guard<std::mutex> lk(mt_);
                std::cout<< "Please enter your PIN (0-9)"<< std::endl;

            }).handle<display_balance>([&](const display_balance& msg){
                std::lock_guard<std::mutex> lk(mt_);
                std::cout<< "The balance of your account is " << msg.amount << std::endl;

            }).handle<display_withdrawal_options>([&](const display_withdrawal_options&){
                std::lock_guard<std::mutex> lk(mt_);
                std::cout << "Withdraw 50? (w)" << std::endl;
                std::cout << "Display Balance? (b)"<< std::endl;
                std::cout << "Cancel? (c)" << std::endl;

            }).handle<display_withdrawal_cancelled>([&](const display_withdrawal_cancelled&){
                std::lock_guard<std::mutex> lk(mt_);
                std::cout<< "Withdrawal cancelled"<< std::endl;

            }).handle<display_pin_incorrect_message>([&](const display_pin_incorrect_message&){
                std::lock_guard<std::mutex> lk(mt_);
                std::cout<< "PIN incorrect"<< std::endl;
                
            }).handle<display_pin_correct_message>([&](const display_pin_correct_message&){
                std::lock_guard<std::mutex> lk(mt_);
                std::cout<< "PIN verified"<< std::endl;
                
            }).handle<eject_card>([&](const eject_card&){
                std::lock_guard<std::mutex> lk(mt_);
                std::cout << "Ejecting card" << std::endl;
            });
        } 
    }
    void done() {getSender().send(messaging::CloseQueue());}
    messaging::Sender getSender() {return incoming;}
private:
    std::mutex mt_;
    messaging::Receiver incoming;
};