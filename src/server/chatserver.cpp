#include "chatserver.hpp"
#include "chatservice.hpp"
#include "json.hpp"

#include <iostream>
#include <functional>
#include <string>


using json = nlohmann::json;

ChatServer::ChatServer(muduo::net::EventLoop *loop, const muduo::net::InetAddress &listenAddr, const std::string &nameArg)
	: _server(loop, listenAddr, nameArg), _loop(loop)
{
	_server.setConnectionCallback(std::bind(&ChatServer::onConnection, this, std::placeholders::_1));
	_server.setMessageCallback(std::bind(&ChatServer::onMessage, this,
										 std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

	_server.setThreadNum(4);
}

void ChatServer::start()
{
	_server.start();
}

void ChatServer::onConnection(const muduo::net::TcpConnectionPtr &conn)
{
	if (!conn->connected())
	{
		ChatService::instance()->clientCloseException(conn);
		conn->shutdown();
	}
}

void ChatServer::onMessage(const muduo::net::TcpConnectionPtr &conn, muduo::net::Buffer *buffer, muduo::Timestamp time)
{
	// 数据的反序列化
	std::string buf = buffer->retrieveAllAsString();
	std::cout << buf << std::endl;

	json js = json::parse(buf);
	auto msgHandler = ChatService::instance()->getHandler(js["msgid"].get<int>()); // 通过js["msgid"]获取业务handler。
	msgHandler(conn, js, time);
}