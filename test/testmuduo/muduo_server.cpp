/*muduo网络库给用户提供了两个主要的类
TcpServer:用于编写服务器程序的类
TcpClient:用于编写客户端程序的类

将epoll和线程池封装起来，好处是能够把网络的I/O代码与业务代码区分开
*/

#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include <iostream>
#include <functional>
#include <string>
using namespace std;
using namespace muduo;
using namespace muduo::net;
using namespace placeholders;


/*基于muduo网络库开发服务器程序
1.组合TcpServer对象
2.创建EventLoop事件循环对象的指针
3.明确TcpServer构造函数需要什么参数，输出ChatServer的构造函数
4.在当前服务器类的构造函数中，注册处理连接和读写事件的回调函数
5.设置合适的服务端线程数量，muduo会自动划分I/O线程和worker线程
*/
class ChatServer
{
public:
    ChatServer(EventLoop* loop,                 //事件循环——Reactor反应堆
            const InetAddress& listenAddr,      //IP和端口
            const string& nameArg)              //服务器的名字
               : _server(loop, listenAddr, nameArg), _loop(loop)
    {
        //给服务器注册用户连接的创建和断开回调
        _server.setConnectionCallback(std::bind(&ChatServer::OnConnection,this,_1));

        //给服务器注册用户读写事件回调
        _server.setMessageCallback(std::bind(&ChatServer::OnMessage,this,_1,_2,_3));

        //设置服务器的线程数量  1 I/O线程，3个worker线程
        _server.setThreadNum(4);
    }
    //开启事件循环
    void start(){
        _server.start();
    }


private:
    //专门处理用户连接创建和断开    epoll
    void OnConnection(const TcpConnectionPtr&conn){
        if(conn->connected()){
            cout << conn->peerAddress().toIpPort() << "->"  << conn->localAddress().toIpPort() 
            <<  " state:online" << endl;
            }
        else{
            cout << conn->peerAddress().toIpPort() << "->"  << conn->localAddress().toIpPort() 
            <<  " state:offline" << endl;
            conn->shutdown();
        }
    }
    //专门处理用户读写事件    
    void OnMessage(const TcpConnectionPtr&conn ,    //连接
                            Buffer*buffer,            //缓冲区
                            Timestamp time)          //接受到数据的时间信息
    {         
        string buf = buffer->retrieveAllAsString();
        cout << "recv data : " << buf << " time:" << time.toString() << endl;
        conn->send(buf);
    }

    TcpServer _server;
    EventLoop *_loop;       //epoll
};


int main(){

    EventLoop loop; //epoll
    InetAddress addr("127.0.0.1",6000);
    ChatServer server(&loop,addr,"ChatServer");
    server.start();         
    loop.loop();        //epoll_wait以阻塞方式等待新用户连接或读写事件等
    return 0;

}