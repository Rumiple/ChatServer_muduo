#pragma once

#include "user.hpp"


class GroupUser : public User // 群用户，多加了一个role角色信息
{
public:
    void setRole(std::string role) { this->role = role; }
    std::string getRole() { return this->role; }

private:
    std::string role;
};