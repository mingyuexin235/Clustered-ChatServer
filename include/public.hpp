#ifndef PUBLIC_H
#define PUBLIC_H

/*
server和client的公共文件    MSGID值
*/
enum EnMsgType
{
    LOGIN_MSG = 1, // 1:登录消息
    LOGIN_MSG_ACK, // 2:登录响应消息
    REG_MSG,     // 3:注册消息
    REG_MSG_ACK, // 4:注册响应消息
    ONE_CHAT_MSG,   // 5:聊天消息
    ADD_FRIEND_MSG, // 6:添加好友消息
    ADD_FRIEND_MSG_ACK, // 7:添加好友响应消息
    CREATE_GROUP_MSG, // 8:创建群组
    CREATE_GROUP_MSG_ACK, // 9:创建群组响应消息
    ADD_GROUP_MSG, // 10:加入群组
    ADD_GROUP_MSG_ACK, // 11:加入群组响应消息
    GROUP_CHAT_MSG, // 12:群聊天
    LOGINOUT_MSG, // 13:注销消息
};

#endif
