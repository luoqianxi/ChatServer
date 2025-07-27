#include "redis.hpp"
#include <iostream>

Redis::Redis() : _publish_context(nullptr), _subscribe_context(nullptr) {}

Redis::~Redis()
{
    if (_publish_context != nullptr)
    {
        redisFree(_publish_context);
    }
    if (_subscribe_context != nullptr)
    {
        redisFree(_subscribe_context);
    }
}

bool Redis::connect()
{
    _publish_context = redisConnect("127.0.0.1", 6379);
    if (_publish_context == nullptr)
    {
        std::cerr << "connect redis failed!" << std::endl;
        return false;
    }

    _subscribe_context = redisConnect("127.0.0.1", 6379);
    if (_subscribe_context == nullptr)
    {
        std::cerr << "connect redis failed!" << std::endl;
        return false;
    }

    std::thread t([&]()
                  { observer_channel_message(); });
    t.detach();

    std::cout << "connect redis-server success!" << std::endl;
    return true;
}

bool Redis::publish(int channel, std::string message)
{
    redisReply *reply = (redisReply *)redisCommand(_publish_context, "PUBLISH %d %s", channel, message.c_str());
    if (reply == nullptr)
    {
        std::cerr << "publish command failed!" << std::endl;
        return false;
    }
    freeReplyObject(reply);
    return true;
}

bool Redis::subscribe(int channel)
{
    if (REDIS_ERR == redisAppendCommand(this->_subscribe_context, "SUBSCRIBE %d", channel))
    {
        std::cerr << "subscribe command failed!" << std::endl;
        return false;
    }

    int done = 0;
    while (!done)
    {
        if (REDIS_ERR == redisBufferWrite(this->_subscribe_context, &done))
        {
            std::cerr << "subscribe command failed!" << std::endl;
            return false;
        }
    }

    return true;
}

bool Redis::unsubscribe(int channel)
{
    if (REDIS_ERR == redisAppendCommand(this->_subscribe_context, "UNSUBSCRIBE %d", channel))
    {
        std::cerr << "unsubscribe command failed!" << std::endl;
        return false;
    }

    int done = 0;
    while (!done)
    {
        if (REDIS_ERR == redisBufferWrite(this->_subscribe_context, &done))
        {
            std::cerr << "unsubscribe command failed!" << std::endl;
            return false;
        }
    }

    return true;
}
// 在独立线程中接收订阅通道中的消息
void Redis::observer_channel_message()
{
    redisReply *reply = nullptr;
    // redisReply *auth_reply = (redisReply *)redisCommand(this->_subscribe_context, "AUTH max123");
    // if (auth_reply->type == REDIS_REPLY_ERROR)
    // {
    //     printf("Auth failed: %s\n", reply->str);
    //     freeReplyObject(auth_reply);
    //     return;
    // }
    // freeReplyObject(auth_reply);
    while (REDIS_OK == redisGetReply(this->_subscribe_context, (void **)&reply))
    {
        // if (reply->type == REDIS_REPLY_ERROR)
        // {
        //     printf("Redis error: %s\n", reply->str);
        //     freeReplyObject(reply);
        //     return;
        // }
        if (reply != nullptr && reply->element[2] != nullptr && reply->element[2]->str != nullptr)
        {
            _notify_message_handler(atoi(reply->element[1]->str), reply->element[2]->str);
        }
        else
        {
            std::cerr << "ERROR:" << reply << " " << reply->element[2]->type << " " << reply->element[2]->str << std::endl;
            return;
        }
        freeReplyObject(reply);
    }
    std::cerr << ">>>>>>>>>>>>>>observer_channel_message_quit<<<<<<<<<<<<<<" << std::endl;
}
// 初始化向业务层上报通道消息的回调对象
void Redis::init_notify_handler(std::function<void(int, std::string)> func)
{
    this->_notify_message_handler = func;
}