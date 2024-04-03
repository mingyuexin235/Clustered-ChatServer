#include "json.hpp"
using json = nlohmann::json;

#include <iostream>
#include <vector>
#include <map>
#include <string>
using namespace std;

//json序列化示例1
string func1(){
    json js;
    js["Msg_Type"]=2;
    js["From"]="Zhang San";
    js["To"]="Li Si";
    js["Message"]="Hey, BaBy!";

    string sendBuf = js.dump();     //转成字符串通过网络发送
    return sendBuf;
}

//json序列化示例2
string func2(){
    json js;
    //添加数组
    js["id"]={1,2,3,4,5};
    //添加键值对
    js["name"]="Zhang San";
    //添加对象
    js["msg"]={{"Zhang san","Hello World!"},{"Li Si","Hello China!"}};
    return js.dump();
}

//json序列化示例3
string func3(){
    json js;
    //序列化一个vector容器
    vector<int>v;
    v.push_back(1);
    v.push_back(2);
    v.push_back(5);
    js["list"]=v;

    //序列化一个map容器
    map<int,string>m;
    m.insert(make_pair(1,"黄山"));
    m.insert(make_pair(2,"华山"));
    m.insert(make_pair(3,"嵩山"));
    js["path"]=m;

    string SendBuf = js.dump();     //json数据对象->序列化 json字符串
    return SendBuf;  
}


int main(){
    string recBuf = func3();
    json jsbuf=json::parse(recBuf);      //数据的反序列化    json字符串->json对象
    // cout << jsbuf["Msg_Type"] << endl;
    // cout << jsbuf["From"] << endl;
    // cout << jsbuf["To"] << endl;
    // cout << jsbuf["Message"] << endl;   

    // auto arr=jsbuf["id"];
    // cout << arr[2] << endl;

    // auto jsmsg=jsbuf["msg"];
    // cout << jsmsg["Zhang san"] << endl;
    // cout << jsmsg["Li Si"] << endl;

    vector<int> vec = jsbuf ["list"];
    for(int &v:vec){
        cout << v << " ";
    }
    cout << endl;
    map<int,string> mymap=jsbuf["path"];
    for(auto &p:mymap){
        cout << p.first << " " << p.second << endl;
    }
    return 0;
}



