#include "groupmodel.hpp"
#include "db.h"

bool GroupModel::createGroup(Group& group) 
{
    char sql[1024] = { 0 };
    sprintf(sql, "insert into allgroup(groupname, groupdesc) values('%s', '%s')",
            group.getName().c_str(), group.getDesc().c_str());
    
    MySQL mysql;
    if (mysql.connect())
    {
        if (mysql.update(sql))
        {
            group.setId(mysql_insert_id(mysql.getConnection()));
            return true;
        }
    }
    return false;
}


void GroupModel::addGroup(int userid, int groupid, std::string role)
{
    char sql[1024] = { 0 };
    sprintf(sql, "insert into groupuser values(%d, %d, '%s')",
            groupid, userid, role.c_str());
    
    MySQL mysql;
    if (mysql.connect()) 
    {
        mysql.update(sql);
    }
}


std::vector<Group> GroupModel::queryGroups(int userid) // 返回的是某一个用户的所有的群组的信息
{
    char sql[1024] = { 0 };
    sprintf(sql, "select a.id, a.groupname, a.groupdesc from allgroup a inner join \
            groupuser b on a.id = b.groupid where b.userid = %d", userid);
    
    std::vector<Group> groupVec;
    MySQL mysql;
    if (mysql.connect())
    {
        MYSQL_RES* res = mysql.query(sql);
        if (res != nullptr)  // 查出userid所有的群组信息
        {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr) // 查询出来的是AllGroup表中的群的信息，不包含群组成员的信息，因此之后还要另外补充成员信息进groupVec中
            {
                Group group; 
                group.setId(atoi(row[0]));
                group.setName(row[1]);
                group.setDesc(row[2]);
                groupVec.push_back(group);
            } 
            mysql_free_result(res);
        }
    }
    // 查询群组的用户信息
    for (Group& group : groupVec)
    {
        sprintf(sql, "select a.id, a.name, a.state, b.grouprole from user a \
                inner join groupuser b on b.userid = a.id where b.groupid = %d", group.getId());
        MYSQL_RES* res = mysql.query(sql);
        if (res != nullptr) 
        {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr)
            {
                GroupUser user;
                user.setId(atoi(row[0]));
                user.setName(row[1]);
                user.setState(row[2]);
                user.setRole(row[3]);
                group.getUsers().push_back(user);
            }
            mysql_free_result(res);
        }
    }
    return groupVec;
}   


std::vector<int> GroupModel::queryGroupUsers(int userid, int groupid) // 返回查询的群里的用户id
{
    char sql[1024] = { 0 };
    sprintf(sql, "select userid from groupuser where groupid = %d and userid != %d", groupid, userid);

    std::vector<int> idVec;
    MySQL mysql;
    if (mysql.connect())
    {
        MYSQL_RES* res = mysql.query(sql);
        if (res != nullptr)
        {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr)
            {
                idVec.push_back(atoi(row[0]));
            }
            mysql_free_result(res);
        }
    }
    return idVec;
}