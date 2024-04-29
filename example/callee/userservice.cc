#include <iostream>
#include <string>
#include "user.pb.h"
#include "mprpcapplication.h"
#include "rpcprovider.h"

using namespace fixbug;

/*
UserServiceԭ����һ�����ط����ṩ�����������ڵı��ط�����Login��GetFriendLists ����ͨ��rpc������Զ�̵�
Ҫת����rpc����
*/
class UserService:public fixbug::UserServiceRpc //ʹ����rpc���񷢲���(rpc�����ṩ��)
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
    ��д����UserServiceRpc���麯�� ������Щ�������ǿ��ֱ�ӵ��õ� �����ڿ�ܴ��룬ֻ�����ÿ�� �������:��ɫ����=>��ɫ����
    1.caller ===> Login(LoginRequest) => muduo => callee
    2.callee ===> Login(LoginRequest) => ����������д�����Login������
    */
    void Login(::google::protobuf::RpcController* controller, //��д����
                       const ::fixbug::LoginRequest* request,
                       ::fixbug::LoginResponse* response,
                       ::google::protobuf::Closure* done)    
    {
        //��ܸ�ҵ���ϱ���������� LoginRequest��ҵ���ȡ��Ӧ����������ҵ�������Ѿ�����ܷ����л�����
        std::string name=request->name();//�Ѿ���protobuf�ֽ��������кò����ɶ��� ����ֱ��ʹ��
        std::string pwd=request->pwd();

        //������ҵ��
        bool login_result=Login(name,pwd);

        //��д��Ӧ��Ϣ ���������� ������Ϣ ����ֵ
        fixbug::ResultCode *code=response->mutable_result();//mutable_result()���ڻ�ȡһ������Ŀɱ�汾
        code->set_errcode(0);
        code->set_errmsg("");
        response->set_success(login_result);

        //ִ�лص����� ִ����Ӧ�������ݵ����л������緢��(�����ɿ������ɵ�)
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

        bool ret=Register(id,name,pwd);//������ҵ��

        response->mutable_result()->set_errcode(0);
        response->mutable_result()->set_errmsg("");
        response->set_success(ret);

        done->Run();
    }



};

int main(int argc,char **argv)
{
    //���ÿ�ܵĳ�ʼ������ ������Ҫip�Ͷ˿ں� provider -i config.conf
    MprpcApplication::Init(argc,argv); 

    //provider��һ��rpc���������󡣿����ڿ���Ϸ������
    RpcProvider provider;   
    //�Ѷ��������UserService���󷢲���rpc�ڵ���
    provider.NotifyService(new UserService());    

    //����һ��rpc���񷢲��ڵ� Run�Ժ󣬽��̽�������״̬���ȴ�Զ�̵�rpc��������
    provider.Run();
    // UserService us;
    // us.Login("xxx","xxx");

    return 0;
}