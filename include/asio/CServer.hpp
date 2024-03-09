#pragma once
#include <boost/asio.hpp>
#include "CSession.h"
#include <memory.h>
#include <map>

using boost::asio::ip::tcp;
class CServer
{
public:
	CServer(boost::asio::io_context& io_context, short port):
	_io_context(io_context), _port(port),_acceptor(io_context, tcp::endpoint(tcp::v4(),port)){
		std::cout << "Server start success, listen on port : " << _port << std::endl;
		StartAccept();
	}
	void ClearSession(std::string uuid){_sessions.erase(uuid);}
private:
	void StartAccept(){
		std::shared_ptr<CSession> new_session = make_shared<CSession>(_io_context, this);
		_acceptor.async_accept(new_session->GetSocket(),[&](const boost::system::error_code& error){
			if (!error) {
				new_session->Start();
				_sessions.insert(std::make_pair(new_session->GetUuid(), new_session));
			}
			else 
				std::cout << "session accept failed, error is " << error.what() << std::endl;
			StartAccept();
		});
	}
	boost::asio::io_context &_io_context;
	short _port;
	tcp::acceptor _acceptor;
	std::map<std::string, std::shared_ptr<CSession>> _sessions;
};

