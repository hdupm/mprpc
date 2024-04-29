#include <iostream>
#include <string>
#include "friend.pb.h"
#include "mprpcapplication.h"
#include "logger.h"


class FriendService:public fixbug::FriendServiceRpc
{
public:
    std::vector<std::string> GetFriendsList(uint32_t userid)
    {
        std::cout<<"do GetFriendsList service!userid:"<<userid<<std::endl;
        std::vector<std::string> vec;
        vec.push_back("gao yang");
        vec.push_back("liu hong");
        vec.push_back("wang shuo");
        return vec;
    }

    //��д���෽��
    void GetFriendsList(::google::protobuf::RpcController* controller,
                       const ::fixbug::GetFriendsListRequest* request,
                       ::fixbug::GetFriendsListResponse* response,
                       ::google::protobuf::Closure* done)
    {
        uint32_t userid=request->userid();

        std::vector<std::string> friendsList=GetFriendsList(userid);
        response->mutable_result()->set_errcode(0);
        response->mutable_result()->set_errmsg("");
        for (std::string &name:friendsList)
        {
            std::string *p=response->add_friends();
            *p=name;
        }

        done->Run();
    }


};

int main(int argc,char **argv)
{
    LOG_INFO("first log message!");
    LOG_ERR("%s:%s:%d",__FILE__,__FUNCTION__,__LINE__);



    //���ÿ�ܵĳ�ʼ������ ������Ҫip�Ͷ˿ں� provider -i config.conf
    MprpcApplication::Init(argc,argv); 

    //provider��һ��rpc���������󡣿����ڿ���Ϸ������
    RpcProvider provider;   
    //�Ѷ��������UserService���󷢲���rpc�ڵ���
    provider.NotifyService(new FriendService());    

    //����һ��rpc���񷢲��ڵ� Run�Ժ󣬽��̽�������״̬���ȴ�Զ�̵�rpc��������
    provider.Run();
    // UserService us;
    // us.Login("xxx","xxx");

    return 0;
}