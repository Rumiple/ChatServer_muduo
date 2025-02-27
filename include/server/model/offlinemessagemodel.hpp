#pragma once

#include <string>
#include <vector>

class OfflineMsgModel // 提供离线消息表的操作接口方法
{
public:
    void insert(int userid, std::string msg); // 存储用户的离线消息
    void remove(int userid); // 删除用户的离线消息
    std::vector<std::string> query(int userid); // 查询用户的离线消息


private:
};