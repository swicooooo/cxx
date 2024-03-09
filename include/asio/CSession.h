#pragma once

#include <boost/asio.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <memory>
#include "const.h"
#include "msg.pb.h"

using boost::asio::ip::tcp;
class CServer;
class CSession: public std::enable_shared_from_this<CSession>
{
public:
	CSession(boost::asio::io_context& io_context, CServer* server):_socket(io_context), _server(server), _b_close(false){
		_uuid = boost::uuids::to_string(boost::uuids::random_generator()());
	}
	~CSession(){
		std::cout << "~CSession destruct" << std::endl;
	}

	tcp::socket& GetSocket(){return _socket;}
	std::string& GetUuid(){return _uuid;}

	void Start(){
		_data.clear();
		_socket.async_receive(boost::asio::buffer(_data,HEAD_DATA_LEN),
			std::bind(&CSession::HandleRead, this, std::placeholders::_1, std::placeholders::_2, shared_from_this()));
	}
	void Send(char* msg,  short max_length, int32_t msgid){

	}
	void Close(){
		_b_close=true;
		_socket.close();

	}
private:
	void HandleRead(const boost::system::error_code& error, size_t  bytes_transferred, std::shared_ptr<CSession>){
		std::cout << "HandleRead: " << _data.data() << std::endl;
		if(!error){
			if(bytes_transferred<HEAD_DATA_LEN){
				std::cout << "read head lenth error" << std::endl;
				Close();
				_server->ClearSession(_uuid);
				return;
			}
			// need to convert byte order
			short data_len=0;
			memcpy(&data_len,&_data,HEAD_DATA_LEN);
			short len=ntohs(data_len);
			_data.clear();
			_socket.async_receive(boost::asio::buffer(_data,len),
				std::bind(&CSession::HandleReadMsg, this, std::placeholders::_1, std::placeholders::_2, shared_from_this()));
		}

	}

	void HandleReadMsg(const boost::system::error_code& error, size_t  bytes_transferred, std::shared_ptr<CSession>){
		if(!error){
			std::cout << "HandleRead: " << _data.data() << std::endl;

			// recover data and recall HandleRead
			
		}
	}

	void HandleWrite(const boost::system::error_code& error, std::shared_ptr<CSession>){

	}

	tcp::socket _socket;
	std::string _uuid;
	std::vector<char> _data;
	std::unique_ptr<CServer> _server;
	bool _b_close;
};
