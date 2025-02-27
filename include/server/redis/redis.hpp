#pragma once

#include <hiredis/hiredis.h>
#include <thread>
#include <functional>


class Redis
{
public:
    Redis();
    ~Redis();

    bool connect();  // 连接redis服务器
    bool publish(int channel, std::string message); // 发布消息
    bool subscribe(int channel); // 订阅消息
    bool unsubscribe(int channel); // 向redis指定的通道取消订阅消息
    void observer_channel_message(); // 在独立线程中接收订阅通道中的消息
    void init_notify_handler(std::function<void(int, std::string)> fn); // 初始化向业务层上报通道消息的回调对象
    
private:
    // 发布消息和订阅消息需要用两个对象，因为订阅消息后就会被阻塞。
    redisContext* _publish_context; // redisContext相当于一个客户端，负责发布消息
    redisContext* _subcribe_context; // 同步上下文对象，负责订阅消息
    std::function<void(int, std::string)> _notify_message_handler; // 回调操作，收到订阅的信息，上报给service

};
