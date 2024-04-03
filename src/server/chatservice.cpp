#include "chatservice.hpp"
#include "public.hpp"
#include <iostream>
#include <muduo/base/Logging.h> //muduo的日志
using namespace std;
using namespace muduo;

// 获取单例对象的接口函数
ChatService *ChatService::instance()
{
    static ChatService service;
    return &service;
}

// 构造方法，注册消息以及对应的Handler回调操作
ChatService::ChatService()
{
    // 用户基本业务管理相关事件处理回调注册
    _msgHandlerMap.insert({LOGIN_MSG, std::bind(&ChatService::login, this, _1, _2, _3)});
    _msgHandlerMap.insert({REG_MSG, std::bind(&ChatService::reg, this, _1, _2, _3)});
    _msgHandlerMap.insert({ONE_CHAT_MSG, std::bind(&ChatService::oneChat, this, _1, _2, _3)});
    _msgHandlerMap.insert({ADD_FRIEND_MSG, std::bind(&ChatService::addFriend, this, _1, _2, _3)});

    // 群组业务管理相关事件处理回调注册
    _msgHandlerMap.insert({CREATE_GROUP_MSG, std::bind(&ChatService::createGroup, this, _1, _2, _3)});
    _msgHandlerMap.insert({ADD_GROUP_MSG, std::bind(&ChatService::addGroup, this, _1, _2, _3)});
    _msgHandlerMap.insert({GROUP_CHAT_MSG, std::bind(&ChatService::groupChat, this, _1, _2, _3)});

    // 注销业务
    _msgHandlerMap.insert({LOGINOUT_MSG, std::bind(&ChatService::Loginout, this, _1, _2, _3)});

    // redis业务
    // 连接redis服务器
    if (_redis.connect())
    {
        // 设置上报消息的回调
        _redis.init_notify_handler(std::bind(&ChatService::handleRedisSubscribeMessage, this, _1, _2));
    }
}

// 获取消息对应的处理器
MsgHandler ChatService::getHandler(int msgid)
{
    // 记录错误日志，msgid没有对应的事件处理回调
    auto it = _msgHandlerMap.find(msgid);
    if (it == _msgHandlerMap.end()) // 找不到
    {
        // 返回一个默认的处理器，空操作，=按值获取
        return [=](const TcpConnectionPtr &conn, json &js, Timestamp)
        {
            LOG_ERROR << "msgid:" << msgid << " can not find handler!"; // muduo日志会自动输出endl
        };
    }
    else // 成功的话
    {
        return _msgHandlerMap[msgid]; // 返回这个处理器
    }
}

// 处理登录业务  {"msgid":1,"id":22,"password":"123456"}
void ChatService::login(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int id = js["id"].get<int>();
    string pwd = js["password"];

    User user = _userModel.query(id);
    if (user.getId() == id && user.getPwd() == pwd)
    {
        if (user.getState() == "online")
        {
            // 该用户已经登录，不允许重复登录
            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 2;
            response["errmsg"] = "this account is using, input another!";
            conn->send(response.dump());
        }
        else
        {
            // 登录成功，记录用户连接信息
            {
                lock_guard<mutex> lock(_connMutex);
                _userConnMap.insert({id, conn});
            }

            // 用户登录成功后，向redis订阅channel id
            _redis.subscribe(id);

            // 登录成功，更新用户状态信息 state offline=>online
            user.setState("online");
            _userModel.updateState(user);

            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 0;
            response["id"] = user.getId();
            response["name"] = user.getName();
            // 查询该用户是否有离线消息
            vector<string> vec = _offlineMsgModel.query(id);
            if (!vec.empty())
            {
                response["offlinemsg"] = vec;
                // 读取该用户的离线消息后，把该用户的所有离线消息删除掉
                _offlineMsgModel.remove(id);
            }

            // 查询该用户的好友信息并返回
            vector<User> userVec = _friendModel.query(id);
            if (!userVec.empty())
            {
                vector<string> vec2;
                for (User &user : userVec)
                {
                    json js;
                    js["id"] = user.getId();
                    js["name"] = user.getName();
                    js["state"] = user.getState();
                    vec2.push_back(js.dump());
                }
                response["friends"] = vec2;
            }

            // 查询用户的群组信息
            vector<Group> groupuserVec = _groupModel.queryGroups(id);
            if (!groupuserVec.empty())
            {
                // group:[{groupid:[xxx, xxx, xxx, xxx]}]
                vector<string> groupV;
                for (Group &group : groupuserVec)
                {
                    json grpjson;
                    grpjson["id"] = group.getId();
                    grpjson["groupname"] = group.getName();
                    grpjson["groupdesc"] = group.getDesc();
                    vector<string> userV;
                    for (GroupUser &user : group.getUsers())
                    {
                        json js;
                        js["id"] = user.getId();
                        js["name"] = user.getName();
                        js["state"] = user.getState();
                        js["role"] = user.getRole();
                        userV.push_back(js.dump());
                    }
                    grpjson["users"] = userV;
                    groupV.push_back(grpjson.dump());
                }

                response["groups"] = groupV;
            }

            conn->send(response.dump());
        }
    }
    else
    {
        // 该用户不存在，用户存在但是密码错误，登录失败
        json response;
        response["msgid"] = LOGIN_MSG_ACK;
        response["errno"] = 1;
        response["errmsg"] = "id or password is invalid!";
        conn->send(response.dump());
    }
}

// 处理注册业务         {"msgid":3,"name":"Jiao","password":"123456"}
void ChatService::reg(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    string name = js["name"];    // 获取名字
    string pwd = js["password"]; // 获取密码

    User user; // 创建用户对象
    user.setName(name);
    user.setPwd(pwd);
    bool state = _userModel.insert(user); // 新用户的插入
    if (state)                            // 插入成功
    {
        // 注册成功
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 0;
        response["id"] = user.getId();
        conn->send(response.dump()); // 回调 ，返回json字符串
    }
    else // 插入失败
    {
        // 注册失败
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 1;
        conn->send(response.dump()); // 回调 ，返回json字符串
    }
}

// 处理客户端异常退出
void ChatService::clientCloseException(const TcpConnectionPtr &conn)
{
    User user;
    {
        lock_guard<mutex> lock(_connMutex);
        for (auto it = _userConnMap.begin(); it != _userConnMap.end(); it++) // 遍历
        {
            if (it->second == conn)
            {
                user.setId(it->first);
                // 从_userConnMap表中删除用户的连接信息
                _userConnMap.erase(it);
                break;
            }
        }
    }

    // 取消redis订阅
    _redis.unsubscribe(user.getId());

    // 更新用户的状态信息
    if (user.getId() != -1)
    {
        user.setState("offline");
        _userModel.updateState(user);
    }
}

// 处理点对点聊天消息       {"msgid":5,"id":13,"name":"zhang san","to":22,"msg":"Hello,Jiao!"}
void ChatService::oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int to_id = js["toid"].get<int>(); // 获取聊天对象id号
    {
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnMap.find(to_id); // 查看聊天对象是否在线
        if (it != _userConnMap.end())
        {
            // 聊天对象在线，转发消息，服务器主动推动消息给to_id用户
            it->second->send(js.dump());
            return;
        }
    }

    // 查询toid是否在线，如果在线，发布至to_id通道
    User user = _userModel.query(to_id);
    if (user.getState() == "online")
    {
        _redis.publish(to_id, js.dump());
        return;
    }

    // 聊天对象不在线，插入离线消息表中
    _offlineMsgModel.insert(to_id, js.dump());
}

// 服务端异常，业务重置方法
void ChatService::reset()
{

    // 把所有online用户的状态设置为offline
    _userModel.resetState();
}

// 添加好友业务             {"msgid":6,"id":22,"name":"Jiao","friendid":13}
void ChatService::addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();         // 获取用户id
    int friendid = js["friendid"].get<int>(); // 获取好友id

    // 存储消息
    bool state = _friendModel.insert(userid, friendid);
    if (state)
    {
        // 添加好友成功！
        json response;
        response["msgid"] = ADD_FRIEND_MSG_ACK;
        response["error"] = 0;
        response["errmsg"] = "Add friends successfully!";
        response["id"] = userid;
        response["friendid"] = friendid;
        conn->send(response.dump());
    }
    else
    {
        // 添加好友失败
        json response;
        response["msgid"] = ADD_FRIEND_MSG_ACK;
        response["error"] = 1;
        response["errmsg"] = "Add friend failed!";
        conn->send(response.dump());
    }
}

// 创建群组业务             {"msgid":8,"id":22,"groupname":"Happy Group","groupdesc":"Just for fun!"}
void ChatService::createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>(); // 创建群组的用户id
    string name = js["groupname"];    // 群组名称
    string desc = js["groupdesc"];    // 群组描述

    // 存储新创建的群组信息
    Group group(-1, name, desc);
    bool state = _groupModel.createGroup(group);
    if (state)
    {
        // 群组创建成功
        json response;
        response["msgid"] = CREATE_GROUP_MSG_ACK;
        response["error"] = 0;
        response["errmsg"] = "Group creation succeeded!";
        response["groupid"] = group.getId();
        response["groupname"] = group.getName();
        response["groupdesc"] = group.getDesc();

        // 存储群组创建人信息
        bool pstate = _groupModel.addGroup(userid, group.getId(), "creator");
        if (pstate)
        {
            response["creategroup_userid"] = userid;
        }
        conn->send(response.dump());
    }
    else
    {
        // 群组创建失败
        json response;
        response["msgid"] = CREATE_GROUP_MSG_ACK;
        response["error"] = 1;
        response["errmsg"] = "Group creation failure!";
        conn->send(response.dump());
    }
}
// 加入群组业务     {"msgid":10,"id":15,"groupid":2}
void ChatService::addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();       // 想加入群组的用户id
    int groupid = js["groupid"].get<int>(); // 群组id号
    bool pstate = _groupModel.addGroup(userid, groupid, "normal");
    if (pstate)
    {
        json response;
        response["msgid"] = ADD_GROUP_MSG_ACK;
        response["error"] = 0;
        response["errmsg"] = "Successfully join a group!";
        response["userid"] = userid;
        response["groupid"] = groupid;
        conn->send(response.dump());
    }
    else
    {
        json response;
        response["msgid"] = ADD_GROUP_MSG_ACK;
        response["error"] = 1;
        response["errmsg"] = "Failed to join a group!";
        conn->send(response.dump());
    }
}
// 群聊业务         {"msgid":12,"id":13,"groupid":2,"msg":"Oh.The weather is good today."}
void ChatService::groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    vector<int> useridVec = _groupModel.queryGroupUsers(userid, groupid); // 查询这个用户所在群组的其他用户id

    lock_guard<mutex> lock(_connMutex); // 不允许其他人在map里面增删改查
    for (int id : useridVec)
    {
        auto it = _userConnMap.find(id);
        if (it != _userConnMap.end()) // 证明在线，直接转发
        {
            // 转发群消息
            it->second->send(js.dump());
        }
        else
        {
            // 查询群成员是否在线，如果在线，发布至id通道
            User user = _userModel.query(id);
            if (user.getState() == "online")
            {
                _redis.publish(id, js.dump());
            }
            else
            {
                // 存储离线群消息
                _offlineMsgModel.insert(id, js.dump());
            }
        }
    }
}

// 处理注销业务
void ChatService::Loginout(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    {
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnMap.find(userid);
        if (it != _userConnMap.end())
        {
            _userConnMap.erase(it);
        }
    }

    // 更新用户状态信息
    User user(userid, "", "", "offline");
    _userModel.updateState(user);
}

// 从redis消息队列获取订阅消息
void ChatService::handleRedisSubscribeMessage(int userid, string msg)
{
    lock_guard<mutex>locak(_connMutex);
    auto it = _userConnMap.find(userid);
    if(it!=_userConnMap.end()){
        it->second->send(msg);
        return ;
    }
    //插入离线消息表中
    _offlineMsgModel.insert(userid,msg);
}
