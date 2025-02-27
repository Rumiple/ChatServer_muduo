#pragma once

#include "user.hpp"
#include <vector>


class FriendModel // 维护好友信息
{
public:
    void insert(int userid, int friendid); // 添加好友
    std::vector<User> query(int userid); // 返回好友列表以及好友的状态
};