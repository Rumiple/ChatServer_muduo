#include "json.hpp"
#include <iostream>
#include <thread>
#include <string>
#include <vector>
#include <chrono>
#include <ctime>
#include <unordered_map>
#include <functional>
#include <functional>

using json = nlohmann::json;

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <semaphore.h>
#include <atomic>

#include "group.hpp"
#include "user.hpp"
#include "public.hpp"

User g_currentUser; // 记录当前系统登录的用户信息
std::vector<User> g_currentUserFriendList; // 记录当前登录用户的好友列表信息
std::vector<Group> g_currentUserGroupList; // 记录当前登录用户的群组列表信息

bool isMainMenuRunning = false;

sem_t rwsem; // 用于读写线程之间的通信
std::atomic_bool g_isLoginSuccess{false};

void showCurrentUserData(); // 显示当前登录成功用户的基本信息

void readTaskHandler(int clientfd); // 接收线程

std::string getCurrentTime(); // 获取系统时间

void mainMenu(int); // 主聊天页面程序


int main(int argc, char** argv)
{
    if (argc < 3)
    {
        std::cerr << "command invalid! example: ./ChatClient 127.0.0.1 6000" << std::endl; // std::cerr（console error）是ISO C++标准错误输出流，对应于ISO C标准库的stderr。
    }

    // 获取命令行传入的ip和port
    char* ip = argv[1];
    uint16_t port = atoi(argv[2]);

    // 创建client的socket
    int clientfd = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == clientfd)
    {
        std::cerr << "socket create error" << std::endl;
        exit(-1);
    }

    // 初始化client需要连接的server的ip和port
    sockaddr_in server;
    memset(&server, 0, sizeof(sockaddr_in));
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = inet_addr(ip);

    // client和server进行连接
    if (connect(clientfd, (sockaddr*)&server, sizeof(sockaddr_in)) == -1)
    {
        std::cerr << "connect server error" << std::endl;
        close(clientfd);
        exit(-1);
    }

    // 服务器连接成功后就立马启动接收子线程
    std::thread readTask(readTaskHandler, clientfd);
    readTask.detach();

    for (;;)
    {
        // 首页面菜单，登录、注册、退出
        std::cout << "=============================" << std::endl;
        std::cout << "1.login" << std::endl;
        std::cout << "2.register" << std::endl;
        std::cout << "3.quit" << std::endl;
        std::cout << "=============================" << std::endl;
        std::cout << "choice:";
        int choice = 0;
        std::cin >> choice; // cin在读取完choice后，是不会把用户输入的回车读掉，因此用户最后输入的回车仍留在缓冲区，需要处理
        std::cin.get(); // 读掉缓冲区残留的回车
        std::cout << choice << std::endl;

        switch (choice)
        {
            case 1: // login
            {
                int id = 0;
                char pwd[50] = { 0 };
                std::cout << "userid:";
                std::cin >> id;
                if (std::cin.fail()) {
                    std::cin.clear();
                    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // 清空输入缓冲区
                    std::cerr << "Invalid user ID! Please enter a valid integer." << std::endl;
                    break;
                }
                std::cin.get();
                std::cout << "userpasswrd:";
                std::cin.getline(pwd, 50);
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // 清除残留的回车符

                json js;
                js["msgid"] = LOGIN_MSG;
                js["id"] = id;
                js["password"] = pwd;
                std::string request = js.dump();

                g_isLoginSuccess = false;

                int len = send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0);
                if (len == -1)
                {
                    std::cerr << "send login msg error:" << request << std::endl;
                }

                sem_wait(&rwsem); // 等待信号量，由子线程处理完登录的响应消息后，通知这里进入聊天主菜单。
                if (g_isLoginSuccess)
                {
                    isMainMenuRunning = true;
                    mainMenu(clientfd);
                } 
            }
            break;

            case 2: // register
            {
                char name[50] = { 0 };
                char pwd[50] = { 0 };
                std::cout << "username:";
                std::cin.getline(name, 50);
                std::cout << "userpassword:";
                std::cin.getline(pwd, 50);

                json js;
                js["msgid"] = REG_MSG;
                js["name"] = name;
                js["password"] = pwd;
                std::string request = js.dump();

                int len = send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0); // 将信息发送给服务器
                if (len == -1)
                {
                    std::cerr << "send reg msg error:" << request << std::endl;
                }
                sem_wait(&rwsem);
            } 
            break;
            
            case 3:
            {
                close(clientfd);
                sem_destroy(&rwsem);
                exit(0);
            }
            default:
                std::cerr << "invalid input!" << std::endl;
                break;
                
        }
    }

    return 0;
}


void doRegResponse(json& responsejs) // 处理注册的响应
{
    if (0 != responsejs["errno"].get<int>())
    {
        std::cerr << "name is already exist, register error!" << std::endl;
    } else {
        std::cout << "name register success, userid is " << responsejs["id"] << ", do not forget it!" << std::endl;
    }
}


void doLoginResponse(json& responsejs) // 处理登录的响应
{
    if (0 != responsejs["errno"].get<int>()) // 登录失败
    {
        std::cerr << responsejs["errmsg"] << std::endl;
        g_isLoginSuccess = false;
    } else {
        g_currentUser.setId(responsejs["id"].get<int>());
        g_currentUser.setName(responsejs["name"]);

        if (responsejs.contains("friends"))
        {
            g_currentUserFriendList.clear();
            std::vector<std::string> vec = responsejs["friends"];
            for (std::string &str : vec)
            {
                json js = json::parse(str);
                User user;
                user.setId(js["id"].get<int>());
                user.setName(js["name"]);
                user.setState(js["state"]);
                g_currentUserFriendList.push_back(user);
            }
        }

        if (responsejs.contains("groups"))
        {
            g_currentUserGroupList.clear();
            std::vector<std::string> vec1 = responsejs["groups"];
            for (std::string& groupstr : vec1)
            {
                json grpjs = json::parse(groupstr);
                Group group;
                group.setId(grpjs["id"].get<int>());
                group.setName(grpjs["groupname"]);
                group.setDesc(grpjs["groupdesc"]);

                std::vector<std::string> vec2 = grpjs["users"];
                for (std::string &userstr : vec2)
                {
                    GroupUser user;
                    json js = json::parse(userstr);
                    user.setId(js["id"].get<int>());
                    user.setName(js["name"]);
                    user.setState(js["state"]);
                    user.setRole(js["role"]);
                    group.getUsers().push_back(user);
                }
                g_currentUserGroupList.push_back(group);
            }
        }

        showCurrentUserData();

        // 显示当前用户的离线消息
        if (responsejs.contains("offlinemsg"))
        {
            std::vector<std::string> vec = responsejs["offlinemsg"];
            for (std::string &str : vec)
            {
                json js = json::parse(str);
                if (ONE_CHAT_MSG == js["msgid"].get<int>())
                {
                    std::cout << js["time"].get<std::string>() << " [" << js["id"] << "]" << js["name"].get<std::string>()
                            << "said: " << js["msg"].get<std::string>() << std::endl;
                } else {
                    std::cout << "群消息[" << js["groupid"] << "]: " << js["time"].get<int>() << " [" << js["id"] << "]" 
                            << js["name"].get<std::string>() << "said: " << js["msg"].get<std::string>() << std::endl;
                }
            }
        }
        g_isLoginSuccess = true;
    }
}


void readTaskHandler(int clientfd) // 子线程，接收线程
{
    for (;;)
    {
        char buffer[1024] = {0};
        int len = recv(clientfd, buffer, 1024, 0);
        if (-1 == len || 0 == len)
        {
            close(clientfd);
            exit(-1);
        }

        json js = json::parse(buffer);
        int msgtype = js["msgid"].get<int>();
        if (ONE_CHAT_MSG == msgtype)
        {
            std::cout << js["time"].get<std::string>() << "[" << js["id"] << "]" << js["name"].get<std::string>() << " said: "
                << js["msg"].get<std::string>() << std::endl;
            continue;
        }

        if (GROUP_CHAT_MSG == msgtype)
        {
            std::cout << "群消息[" << js["groupid"] << "]:" << js["time"].get<std::string>() << "[" << js["id"] << "]" << js["name"].get<std::string>()
                    << "said: " << js["msg"].get<std::string>() << std::endl;
        }

        if (LOGIN_MSG_ACK == msgtype)
        {
            doLoginResponse(js);
            sem_post(&rwsem); // 通知主线程，登录结束
            continue;
        }

        if (REG_MSG_ACK == msgtype)
        {
            doRegResponse(js);
            sem_post(&rwsem);
            continue;
        }
    }
}


void showCurrentUserData()
{
    std::cout << "=====================login user=====================" << std::endl;
    std::cout << "current login user => id:" << g_currentUser.getId() << " name:" << g_currentUser.getName() << std::endl;
    std::cout << "---------------------friend list---------------------" << std::endl;
    if (!g_currentUserFriendList.empty())
    {
        for (User &user : g_currentUserFriendList)
        {
            std::cout << user.getId() << " " << user.getName() << " " << user.getState() << std::endl;
        }
    }
    std::cout << "---------------------group list---------------------" << std::endl;
    if (!g_currentUserGroupList.empty())
    {
        for (Group &group : g_currentUserGroupList)
        {
            std::cout << group.getId() << " " << group.getName() << " " << group.getDesc() << std::endl;
            for (GroupUser &user : group.getUsers())
            {
                std::cout << user.getId() << " " << user.getName() << " " << user.getState() << " " << user.getRole() << std::endl;
            }
        }
    }
    std::cout << "====================================================" << std::endl;
}



void help(int fd = 0, std::string str = "");
void chat(int, std::string);
void addfriend(int, std::string);
void creategroup(int, std::string);
void addgroup(int, std::string);
void groupchat(int, std::string);
void loginout(int, std::string);

// 系统提供给用户看的可用客户端命令列表
std::unordered_map<std::string, std::string> commandMap = {
    {"help", "显示所有支持的命令,格式help"},
    {"chat", "一对一聊天,格式chat::friendid::message"},
    {"addfriend", "添加好友,格式addfriend:friendid"},
    {"creategroup","创建群组,格式creategroup::groupname::groupdesc"},
    {"addgroup", "加入群组,格式addgroup:groupid"},
    {"groupchat", "群聊,格式groupchat::groupid::message"},
    {"loginout", "注销,格式loginout"}
};

// 用于系统处理客户端的命令
std::unordered_map<std::string, std::function<void(int, std::string)>> commandHandlerMap = {
    {"help", help},
    {"chat", chat},
    {"addfriend", addfriend},
    {"creategroup", creategroup},
    {"addgroup", addgroup},
    {"groupchat", groupchat},
    {"loginout", loginout}
};

void mainMenu(int clientfd) // 主聊天页面程序
{
    help();
    char buffer[1024] = { 0 };
    while (isMainMenuRunning)
    {
        std::cin.getline(buffer, 1024);
        std::string commandbuf(buffer);
        std::string command;
        int idx = commandbuf.find(":");
        if (-1 == idx)
        {
            command = commandbuf;
        } else {
            command = commandbuf.substr(0, idx);
        }
        auto it = commandHandlerMap.find(command);
        if (it == commandHandlerMap.end())
        {
            std::cerr << "invalid input command!" << std::endl;
            continue;
        }
        it->second(clientfd, commandbuf.substr(idx + 1, commandbuf.size() - idx));
    }
}

void help(int, std::string)
{
    std::cout << "show command list >>> " << std::endl;
    for (auto& p : commandMap)
    {
        std::cout << p.first << " : " << p.second << std::endl;
    }
    std::cout << std::endl;
}

void addfriend(int clientfd, std::string str)
{
    int friendid = atoi(str.c_str());
    json js;
    js["msgid"] = ADD_FRIEND_MSG;
    js["id"] = g_currentUser.getId();
    js["friendid"] = friendid;
    std::string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        std::cerr << "send addfriend msg error -> " << buffer << std::endl;
    }
}

void chat(int clientfd, std::string str)
{
    int idx = str.find(":");
    if (-1 == idx)
    {
        std::cerr << "chate command invalid!" << std::endl;
        return;
    }
    int friendid = atoi(str.substr(0, idx).c_str());
    std::string message = str.substr(idx + 1, str.size() - idx);
    json js;
    js["msgid"] = ONE_CHAT_MSG;
    js["id"] = g_currentUser.getId();
    js["name"] = g_currentUser.getName();
    js["toid"] = friendid;
    js["msg"] = message;
    js["time"] = getCurrentTime();
    std::string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        std::cerr << "send chat msg error -> " << buffer << std::endl;
    }
}

void creategroup(int clientfd, std::string str)
{
    int idx = str.find(":");
    if (-1 == idx)
    {
        std::cerr << "creategroup command invalid!" << std::endl;
        return;
    }
    std::string groupname = str.substr(0, idx);
    std::string groupdesc = str.substr(idx + 1, str.size() - idx);
    json js;
    js["msgid"] = CREATE_GROUP_MSG;
    js["id"] = g_currentUser.getId();
    js["groupname"] = groupname;
    js["groupdesc"] = groupdesc;
    std::string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        std::cerr << "send creategroup msg error -> " << buffer << std::endl;
    }
}

void addgroup(int clientfd, std::string str)
{
    int groupid = atoi(str.c_str());
    json js;
    js["msgid"] = ADD_GROUP_MSG;
    js["id"] = g_currentUser.getId();
    js["groupid"] = groupid;
    std::string buffer = js.dump();
    
    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        std::cerr << "send addgroup msg error -> " << buffer << std::endl;
    }
}

void groupchat(int clientfd, std::string str)
{
    int idx = str.find(":");
    if (-1 == idx)
    {
        std::cerr << "groupchat command invalid!" << std::endl;
        return;
    }
    int groupid = atoi(str.substr(0, idx).c_str());
    std::string message = str.substr(idx + 1, str.size() - idx);
    json js;
    js["msgid"] = GROUP_CHAT_MSG;
    js["id"] = g_currentUser.getId();
    js["name"] = g_currentUser.getName();
    js["groupid"] = groupid;
    js["msg"] = message;
    js["time"] = getCurrentTime();
    std::string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        std::cerr << "send groupchat msg error -> " << buffer << std::endl;
    }
}

void loginout(int clientfd, std::string str)
{
    json js;
    js["msgid"] = LOGINOUT_MSG;
    js["id"] = g_currentUser.getId();
    std::string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        std::cerr << "send loginout msg error -> " << buffer << std::endl;
    } else {
        isMainMenuRunning = false;
    }
}


std::string getCurrentTime()
{
    auto tt = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    struct tm* ptm = localtime(&tt);
    char date[60] = {0};
    sprintf(date, "%d-%02d-%02d %02d:%02d:%02d", (int)ptm->tm_year + 1900, (int)ptm->tm_mon + 1, (int)ptm->tm_mday,
            (int)ptm->tm_hour, (int)ptm->tm_min, (int)ptm->tm_sec);
    return std::string(date);
}

