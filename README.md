# Clustered-ChatServer
本项目为Linux环境下使用Cmake构建的集群聊天服务器，实现新用户注册、用户登录、添加好友、添加群组、好友通信、群组聊天、离线消息等功能。     
- 使用muduo网络库提供高并发网络I0服务
- 使用json序列化和反序列化消息作为私有通信协议
- 配置Nginx基于TCP的负载均衡实现聊天服务器的集群功能
- 使用Redis的发布-订阅模型解决跨服务器通信难题
- 封装MySQL接口存储于磁盘中，

## 文件夹
- /bin：可执行文件ChatClient与ChatServer     
- /build：CMake编译生成的中间文件     
- /include：头文件     
    - public.hpp ：MSGID枚举常量          
    - chatserver.hpp ：网络层头文件     
    - chatservice.hpp ： 业务层头文件     
    - db/db.hpp ： 数据层头文件     
    - model:数据库操作对象     
        - friendmodel.hpp:好友表操作对象     
        - group.hpp      : 群组对象     
        - groupmodel.hpp : 群组操作对象     
        - groupuser.hpp  ：群组用户对象     
        - offlinemessagemodel : 离线消息操作对象    
        - user.hpp       ：用户对象     
        - usermodel.hpp  : 用户操作对象     
    - redis/redis.hpp    : redis发布订阅模式头文件     
- /test:muduo、Boost、Json测试文件     
- /thirdparty:第三方库json.hpp     
- /src:源文件     
    - /client：客户端源文件     
    - /server: 服务端源文件     
- CMakeLists：CMake编译文件     

## 编译与执行
编译项目：./autobuild.sh     
执行：(多个)客户端：./ChatClient 127.0.0.1 8000     
      服务端(1)：./ChatServer 127.0.0.1 6000     
      服务端(2)：./ChatServer 127.0.0.1 6000     



