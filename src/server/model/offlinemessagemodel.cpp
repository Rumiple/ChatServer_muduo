#include "offlinemessagemodel.hpp"
#include "db.h"




void OfflineMsgModel::insert(int userid, std::string msg) // 存储用户的离线消息
{
    char sql[1024] = { 0 };
    sprintf(sql, "insert into offlinemessage values(%d, '%s')", userid, msg.c_str());
    MySQL mysql;
    if (mysql.connect())
    {
        mysql.update(sql);
    }
}

void OfflineMsgModel::remove(int userid) // 删除用户的离线消息
{
    char sql[1024] = { 0 };
    sprintf(sql, "delete from offlinemessage where userid=%d", userid);
    MySQL mysql;
    if (mysql.connect())
    {
        mysql.update(sql);
    }
}

std::vector<std::string> OfflineMsgModel::query(int userid) // 查询用户的离线消息
{
    char sql[1024] = { 0 };
    sprintf(sql, "select message from offlinemessage where useid = %d", userid);
    std::vector<std::string> vec;
    MySQL mysql;
    if (mysql.connect()) {
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr) {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr) {
                vec.push_back(row[0]);
            }
            mysql_free_result(res);
        }
    }
    return vec;
}





