#include <muduo/base/Logging.h>
#include "db.h"


static std::string server = "127.0.0.1";
static std::string user = "root";
static std::string password = "Ruimple@1234";
static std::string dbname = "chat";

MySQL::MySQL() : _conn(mysql_init(nullptr)) {} // mysql_init用于初始化一个MySQL对象


MySQL::~MySQL() {
    if (_conn != nullptr) {
        mysql_close(_conn); // mysql_close关闭数据库连接以释放找资源
    }
}


bool MySQL::connect() {
    // 使用mysql_real_connect连接mysql，参数依次为MySQL对象、数据库服务器的地址、数据库用户名、数据库密码、要连接的数据库名称。
    MYSQL* p = mysql_real_connect(_conn, server.c_str(), user.c_str(), password.c_str(), dbname.c_str(), 3306, nullptr, 0);
    if (p != nullptr) {
        mysql_query(_conn, "set names gbk");  // C和C++代码默认的编码中字符是ASCII，如果不设置，从mysql上拉下来的中文会显示？。
        LOG_INFO << "connect mysql success!";
    }
    else {
        LOG_INFO << "connect mysql fail!";
    }
    return p;
}


bool MySQL::update(std::string sql) { // 执行sql语句
    if (mysql_query(_conn, sql.c_str())) {
        LOG_INFO << __FILE__ << ":" << __LINE__ << ":" << sql << "更新失败!";
        return false;
    }
    return true;
} 


MYSQL_RES* MySQL::query(std::string sql) {
    if (mysql_query(_conn, sql.c_str())) { // 执行mysql语句
        LOG_INFO << __FILE__ << ":" << __LINE__ << ":" << sql << "查询失败!";
        return nullptr;
    }
    return mysql_use_result(_conn); // mysql_use_result逐行处理结果集。也可以使用mysql_store_result将整个结果集加载到内存中，适用于结果集比较小的情况。
}


MYSQL* MySQL::getConnection() {
    return _conn;
}