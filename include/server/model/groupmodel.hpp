#pragma once

#include "group.hpp"
#include <string>
#include <vector>

class GroupModel // 群信息的操作接口类，与mysql相关
{
public:
    bool createGroup(Group& group); // 创建群
    void addGroup(int userid, int groupid, std::string role); // 加入群
    std::vector<Group> queryGroups(int userid); // 查询所在群的信息
    std::vector<int> queryGroupUsers(int userid, int groupid); // 根据指定的groupid查询群里用户id列表
};
