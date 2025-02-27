#pragma once


#include <unordered_map>
#include <functional>
#include <mutex>
#include <muduo/net/TcpConnection.h>


#include "usermodel.hpp"
#include "json.hpp"
#include "offlinemessagemodel.hpp"
#include "friendmodel.hpp"
#include "groupmodel.hpp"
#include "redis.hpp"

using json = nlohmann::json;
using MsgHandler = std::function<void(const muduo::net::TcpConnectionPtr& conn, json& js, muduo::Timestamp)>;

class ChatService
{
public:
    static ChatService* instance();  // 获取单例对象的接口函数
    MsgHandler getHandler(int msgid); // 获取消息对应的处理器
    void login(const muduo::net::TcpConnectionPtr& conn, json& js, muduo::Timestamp time); // 登录
    void reg(const muduo::net::TcpConnectionPtr& conn, json& js, muduo::Timestamp time); // 注册
    void oneChat(const muduo::net::TcpConnectionPtr& conn, json& js, muduo::Timestamp time); // 一对一聊天
    void addFriend(const muduo::net::TcpConnectionPtr& conn, json& js, muduo::Timestamp time); // 添加好友
    void createGroup(const muduo::net::TcpConnectionPtr& conn, json& js, muduo::Timestamp time); // 创建群
    void addGroup(const muduo::net::TcpConnectionPtr& conn, json& js, muduo::Timestamp time); // 加入群
    void groupChat(const muduo::net::TcpConnectionPtr& conn, json& js, muduo::Timestamp time); // 群聊天
    void loginout(const muduo::net::TcpConnectionPtr& conn, json& js, muduo::Timestamp time); // 注销
    void clientCloseException(const muduo::net::TcpConnectionPtr &conn); // 处理用户异常退出
    void reset(); // 处理服务器异常退出时的业务重置
    
    void handleRedisSubscribeMessage(int, std::string); // 从redis消息队列中获取订阅的消息

private:
    ChatService();
    std::unordered_map<int, MsgHandler> _msgHandlerMap; // 存储消息id和其对应的业务处理方法

    std::unordered_map<int, muduo::net::TcpConnectionPtr> _userConnMap; // 存储在线用户的通信连接

    std::mutex _connMutex; // 互斥锁保证_userConnMap的线程安全

    UserModel _userModel; // 数据操作类对象
    OfflineMsgModel _offlineMsgModel;
    FriendModel _friendModel;
    GroupModel _groupModel;

    Redis _redis;
};
