#include <iostream>
#include <string>
#include "user.pb.h"
#include "mprpcapplication.h"
#include "rpcprovider.h"

using namespace fixbug;

/*
UserService原来是一个本地服务，提供了两个进程内的本地方法，Login和GetFriendLists 可以通过rpc服务变成远程的
要转换成rpc服务
*/
class UserService:public fixbug::UserServiceRpc //使用在rpc服务发布端(rpc服务提供者)
{
public:
    bool Login(std::string name,std::string pwd)
    {
        std::cout<<"doing local service:Login"<<std::endl;
        std::cout<<"name:"<<name<<"pwd"<<pwd<<std::endl;
        return true;
    }

    bool Register(uint32_t id,std::string name,std::string pwd)
    {
        std::cout<<"doing local service:Login"<<std::endl;
        std::cout<<"id"<<id<<"name:"<<name<<"pwd"<<pwd<<std::endl;
        return true;
    }

    /*
    重写基类UserServiceRpc的虚函数 下面这些方法都是框架直接调用的 不属于框架代码，只是在用框架 框架任务:绿色部分=>黄色部分
    1.caller ===> Login(LoginRequest) => muduo => callee
    2.callee ===> Login(LoginRequest) => 交到下面重写的这个Login方法上
    */
    void Login(::google::protobuf::RpcController* controller, //重写方法
                       const ::fixbug::LoginRequest* request,
                       ::fixbug::LoginResponse* response,
                       ::google::protobuf::Closure* done)    
    {
        //框架给业务上报了请求参数 LoginRequest，业务获取相应数据做本地业务，数据已经被框架反序列化好了
        std::string name=request->name();//已经被protobuf字节流反序列好并生成对象 可以直接使用
        std::string pwd=request->pwd();

        //做本地业务
        bool login_result=Login(name,pwd);

        //填写响应消息 包括错误码 错误消息 返回值
        fixbug::ResultCode *code=response->mutable_result();//mutable_result()用于获取一个对象的可变版本
        code->set_errcode(0);
        code->set_errmsg("");
        response->set_success(login_result);

        //执行回调操作 执行响应对象数据的序列化和网络发送(都是由框架来完成的)
        done->Run();
    }

    void Register(google::protobuf::RpcController* controller,
                       const ::fixbug::RegisterRequest* request,
                       ::fixbug::RegisterResponse* response,
                       ::google::protobuf::Closure* done)
    {
        uint32_t id=request->id();
        std::string name=request->name();
        std::string pwd=request->pwd();

        bool ret=Register(id,name,pwd);//做本地业务

        response->mutable_result()->set_errcode(0);
        response->mutable_result()->set_errmsg("");
        response->set_success(ret);

        done->Run();
    }



};

int main(int argc,char **argv)
{
    //调用框架的初始化操作 启动需要ip和端口号 provider -i config.conf
    MprpcApplication::Init(argc,argv); 

    //provider是一个rpc网络服务对象。可以在框架上发表服务
    RpcProvider provider;   
    //把多个服务如UserService对象发布到rpc节点上
    provider.NotifyService(new UserService());    

    //启动一个rpc服务发布节点 Run以后，进程进入阻塞状态，等待远程的rpc调用请求
    provider.Run();
    // UserService us;
    // us.Login("xxx","xxx");

    return 0;
}