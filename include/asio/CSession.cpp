#include "CSession.h"
#include "CServer.h"
#include <iostream>
#include <sstream>
#include <json/json.h>
#include <json/value.h>
#include <json/reader.h>
#include "LogicSystem.h"

CSession::CSession(boost::asio::io_context& io_context, CServer* server):
	_socket(io_context), _server(server), _b_close(false),_b_head_parse(false){
	_uuid = boost::uuids::to_string(boost::uuids::random_generator()());
	// _recv_head_node = make_shared<MsgNode>(HEAD_TOTAL_LEN);
}
CSession::~CSession() {
	std::cout << "~CSession destruct" << endl;
}

tcp::socket& CSession::GetSocket() {
	return _socket;
}

std::string& CSession::GetUuid() {
	return _uuid;
}

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

void CSession::Send(char* msg, short max_length, int32_t msgid) {
	std::lock_guard<std::mutex> lock(_send_lock);
	int send_que_size = _send_que.size();
	if (send_que_size > MAX_SENDQUE) {
		std::cout << "session: " << _uuid << " send que fulled, size is " << MAX_SENDQUE << endl;
		return;
	}

	_send_que.push(make_shared<NetMessage>(msg, max_length, msgid));
	if (send_que_size>0) {
		return;
	}
	auto& msgnode = _send_que.front();
	boost::asio::async_write(_socket, boost::asio::buffer(msgnode->_data, msgnode->_total_len), 
		std::bind(&CSession::HandleWrite, this, std::placeholders::_1, SharedSelf()));
}

void CSession::Close() {
	_socket.close();
	_b_close = true;
}

std::shared_ptr<CSession>CSession::SharedSelf() {
	return shared_from_this();
}

void CSession::HandleWrite(const boost::system::error_code& error, std::shared_ptr<CSession> shared_self) {
	//�����쳣����
	try {
		if (!error) {
			std::lock_guard<std::mutex> lock(_send_lock);
			//cout << "send data " << _send_que.front()->_data+HEAD_LENGTH << endl;
			_send_que.pop();
			if (!_send_que.empty()) {
				auto& msgnode = _send_que.front();
				boost::asio::async_write(_socket, boost::asio::buffer(msgnode->_data, msgnode->_total_len),
					std::bind(&CSession::HandleWrite, this, std::placeholders::_1, shared_self));
			}
		}
		else {
			std::cout << "handle write failed, error is " << error.what() << endl;
			Close();
			_server->ClearSession(_uuid);
		}
	}
	catch (std::exception& e) {
		std::cerr << "Exception code : " << e.what() << endl;
	}
	
}

void CSession::HandleRead(const boost::system::error_code& error, size_t  bytes_transferred, std::shared_ptr<CSession> shared_self){
	
}
