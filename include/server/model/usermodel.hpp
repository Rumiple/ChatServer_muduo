#pragma once

#include "user.hpp"


class UserModel // User表的数据操作类
{
public:
    bool insert(User& user); // 增加User表
    User query(int id); // 根据用户id查询用户信息
    bool updateState(User user); // 更新用户的状态信息
    void resetState(); // 重置用户的状态信息


};

