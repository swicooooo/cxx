#pragma once
#include <boost/asio.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <queue>
#include <mutex>
#include <memory>
#include "const.h"
#include "msg.pb.h"

using boost::asio::ip::tcp;
class CServer;
class LogicSystem;

class CSession: public std::enable_shared_from_this<CSession>
{
public:
	CSession(boost::asio::io_context& io_context, CServer* server);
	~CSession();
	tcp::socket& GetSocket();
	std::string& GetUuid();
	void Start();
	void Send(char* msg,  short max_length, int32_t msgid);
	void Close(){}
	std::shared_ptr<CSession> SharedSelf();
private:
	void HandleRead(const boost::system::error_code& error, size_t  bytes_transferred, std::shared_ptr<CSession> shared_self);
	void HandleWrite(const boost::system::error_code& error, std::shared_ptr<CSession> shared_self);

	tcp::socket _socket;
	std::string _uuid;
	std::vector<char> _data;
	CServer* _server;
	bool _b_close;
	std::mutex _send_lock;
	std::queue<std::shared_ptr<NetMessage>> _send_que;
	bool _b_head_parse;
	// std::queue<shared_ptr<SendNode> > _send_que;
	// std::shared_ptr<RecvNode> _recv_msg_node;
	// std::shared_ptr<MsgNode> _recv_head_node;
};
