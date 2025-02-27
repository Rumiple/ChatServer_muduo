#include "redis.hpp"
#include <iostream>


Redis::Redis()
    : _publish_context(nullptr), _subcribe_context(nullptr)
{
}


Redis::~Redis()
{
    if (_publish_context != nullptr)
    {
        redisFree(_publish_context);
    }
    if (_subcribe_context != nullptr)
    {
        redisFree(_subcribe_context);
    }
}


bool Redis::connect() // 连接redis服务器
{
    _publish_context = redisConnect("127.0.0.1", 6379); // 设置发布消息的上下文连接
    if (nullptr == _publish_context)
    {
        std::cerr << "connect redis failed!" << std::endl;
        return false;
    }

    _subcribe_context = redisConnect("127.0.0.1", 6379); // 设置订阅消息的上下文连接
    if (nullptr == _subcribe_context)
    {
        std::cerr << "connect redis failed" << std::endl;
        return false;
    }

    std::thread t([&]() {
        observer_channel_message();
    });
    t.detach();

    std::cout << "connect redis-server success!" << std::endl;
    return true;

}


bool Redis::publish(int channel, std::string message)
{
    redisReply* reply = (redisReply*)redisCommand(_publish_context, "PUBLISH %d %s", channel, message.c_str());
    if (nullptr == reply)
    {
        std::cerr << "publish command failed!" << std::endl;
        return false;
    }
    freeReplyObject(reply);
    return true;
}


bool Redis::subscribe(int channel)
{
    // 这里没使用redisCommand的原因是：redisCommand相当于是redisAppendCommand+redisBufferRead+redisGetReply，多了最后一个阻塞等待回应。
    // 但我们这里只负责发送订阅的命令，不阻塞接收redis_server响应消息，否则回合notifyMsg线程抢占资源。
    if (REDIS_ERR == redisAppendCommand(this->_subcribe_context, "SUBSCRIBE %d", channel))
    {
        std::cerr << "subscribe command failed!" << std::endl;
        return false;
    }
    int done;
    while (!done)
    {
        if (REDIS_ERR == redisBufferWrite(this->_subcribe_context, &done))
        {
            std::cerr << "subscribe command failed!" << std::endl;
            return false;
        }
    }
    return true;
}


bool Redis::unsubscribe(int channel)
{
    if (REDIS_ERR == redisAppendCommand(this->_subcribe_context, "UNSUBCRIBE %d", channel))
    {
        std::cerr << "unsubscribe command failed!" << std::endl;
        return false;
    }
    int done = 0;
    while (!done)
    {
        if (REDIS_ERR == redisBufferWrite(this->_subcribe_context, &done))
        {
            std::cerr << "unsuscribe command failed!" << std::endl;
            return false;
        }
    }
    return true;
}


void Redis::observer_channel_message()
{
    redisReply* reply = nullptr;
    while (REDIS_OK == redisGetReply(this->_subcribe_context, (void**)&reply))
    {
        // 订阅接收到的消息是一个3个元素的数组。element[1]是通道号，element[2]是消息。
        if (reply != nullptr && reply->element[2] != nullptr && reply->element[2]->str != nullptr)
        {
            _notify_message_handler(atoi(reply->element[1]->str), reply->element[2]->str);
        }
        freeReplyObject(reply);
    }
    std::cerr << ">>>>>>>>>> observer_channel_message quit <<<<<<<<<<" << std::endl;
}


void Redis::init_notify_handler(std::function<void(int, std::string)> fn)
{
    this->_notify_message_handler = fn;
}