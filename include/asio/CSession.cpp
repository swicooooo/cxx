#include "CSession.h"
#include "CServer.hpp"
#include <iostream>
#include <sstream>
#include <json/json.h>
#include <json/value.h>
#include <json/reader.h>
#include "LogicSystem.h"

void CSession::Start(){
	_data.clear();
	_socket.async_receive(boost::asio::buffer(_data,HEAD_DATA_LEN),
		[self=SharedSelf(),this](const boost::system::error_code& error, size_t  bytes_transferred){
			if(!error){
				if(bytes_transferred<HEAD_DATA_LEN){
					cout << "read head lenth error";
					Close();
					_server->ClearSession(_uuid);
					return;
				}
				short data_len=0;
				int cur_len=0;
				memcpy(&data_len,&_data,HEAD_DATA_LEN);
				if(data_len>MAX_LENGTH){
					std::cout << "invalid data length is " << data_len << endl;
					Close();
					_server->ClearSession(_uuid);
					return;
				}
				_data.clear();
				while (data_len>cur_len){
					_socket.async_receive(boost::asio::buffer(_data,data_len),
						[self=SharedSelf(),&cur_len,&data_len,this](const boost::system::error_code& error, size_t  bytes_transferred){
							if(!error){
								NetMessage message;
								// paser data
								 cout << "receive data is " << _data.data() << endl;
								Send(_data.data(), data_len,message.msg_id());
								//再次接收头部数据
								_data.clear();
								Start();
							}
					});
				}
				

			}else{

			}
	});
	_socket.async_read_some(boost::asio::buffer(_data, MAX_LENGTH), std::bind(&CSession::HandleRead, this, 
		std::placeholders::_1, std::placeholders::_2, SharedSelf()));
}




