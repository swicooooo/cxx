#pragma once

#include "BankMachine.hpp"
#include "InterfaceMachine.hpp"
#include <functional>
#include <iostream>

class ATM {
public:
    ATM(messaging::Sender bank, messaging::Sender interface):bank_(bank), interface_(interface) {};
    ATM(const ATM&) = delete;
    ATM& operator=(const ATM&) = delete;

    void run(){
        state_=[this](){waitForCard();};
        try{
            while (true) 
                state_();
        }catch (const messaging::CloseQueue &){
            std::cout << "system ending..." << std::endl;
        }
    }
    void done(){getSender().send(messaging::CloseQueue());}
    messaging::Sender getSender(){return incoming_;}

private:
    messaging::Receiver incoming_;
    messaging::Sender bank_;
    messaging::Sender interface_;
    uint withdrawal_amount_;
    std::string account_;
    std::string pin_;
    std::function<void()> state_;

    void waitForCard(){
        interface_.send(display_enter_card());
        incoming_.wait().handle<card_inserted>([&](const card_inserted& msg){
            account_= msg.account;
            pin_="";
            interface_.send(display_enter_pin());
            state_=[this](){gettingPin();};
        });
    }
    void gettingPin(){
        incoming_.wait().handle<digit_pressed>([&](const digit_pressed& msg){
            pin_+=msg.digit;
            if(pin_.length()==4){
                bank_.send(verify_pin(account_,pin_,getSender()));
                state_=[&](){verifyingPin();};
            }
        
        }).handle<clear_last_pressed>([&](const clear_last_pressed& msg){
            if(!pin_.empty())
                pin_.pop_back();

        }).handle<cancel_pressed>([&](const cancel_pressed& msg){
            state_=[&](){doneProcessing();};
        });
    }
    void verifyingPin(){
        incoming_.wait().handle<pin_verified>([&](const pin_verified& msg){
            interface_.send(display_pin_correct_message());
            state_=[&](){waitForAction();};

        }).handle<pin_incorrect>([&](const pin_incorrect& msg){
            interface_.send(display_pin_incorrect_message());
            pin_.clear();
            state_=[&](){gettingPin();};

        }).handle<cancel_pressed>([&](const cancel_pressed& msg){
            state_=[&](){doneProcessing();};
        });
    }
    void waitForAction(){
        interface_.send(display_withdrawal_options());
        incoming_.wait().handle<withdraw_pressed>([&](const withdraw_pressed& msg){
            withdrawal_amount_=msg.amount;
            bank_.send(withdraw(account_,msg.amount,getSender()));
            state_=[&](){processingWithdrawal();};

        }).handle<balance_pressed>([&](const balance_pressed& msg){
            bank_.send(get_balance(account_,getSender()));
            state_=[&](){processBalance();};

        }).handle<cancel_pressed>([&](const cancel_pressed& msg){
            state_=[&](){doneProcessing();};
        });
    }
    void processingWithdrawal(){
        incoming_.wait().handle<withdraw_ok>([&](const withdraw_ok& msg){
           interface_.send(issue_money(withdrawal_amount_));
           state_=[&](){doneProcessing();};

        }).handle<withdraw_denied>([&](const withdraw_denied& msg){
           interface_.send(display_insufficient_funds());
           state_=[&](){doneProcessing();};

        }).handle<cancel_pressed>([&](const cancel_pressed& msg){
            bank_.send(cancel_withdrawal(account_,withdrawal_amount_));
            interface_.send(display_withdrawal_cancelled());
            state_=[&](){doneProcessing();};
        });
    }
    void processBalance(){
        incoming_.wait().handle<balance>([&](const balance& msg){
            interface_.send(display_balance(msg.amount));
            state_=[&](){waitForAction();};

        }).handle<cancel_pressed>([&](const cancel_pressed& msg){
            state_=[&](){doneProcessing();};
        });
    }
    void doneProcessing(){
       interface_.send(eject_card());
       state_=[&](){waitForCard();};
    }
};