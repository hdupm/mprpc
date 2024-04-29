#include <iostream>
#include "mprpcapplication.h"
#include "friend.pb.h"
#include "mprpcchannel.h"

//rpc���÷�
int main(int argc,char **argv)
{
    //�������������Ժ���ʹ��mprpc���������rpc������ã�һ����Ҫ�ȵ��ÿ�ܵĳ�ʼ������(ֻ��ʼ��һ��)
    MprpcApplication::Init(argc,argv);

    //��ʾ����Զ�̷�����rpc����
    fixbug::FriendServiceRpc_Stub stub(new MprpcChannel());//channel�൱���н飬���ն��ǵ���rpcchannel��callmethod����
    //rpc�������������
    fixbug::GetFriendsListRequest request;
    request.set_userid(1000);
    //rpc��������Ӧ
    fixbug::GetFriendsListResponse response;//�Է�ִ���귵����Ӧ

    //����rpc�����ĵ��ã���ͬ����rpc���ù��� ��Ӧͼ��߻ƺ���ɫ���� �ײ���MprpcChannel::callMethod
    MprpcController controller;
    stub.GetFriendsList(&controller,&request,&response,nullptr);//RpcChannel->RpcChannel::callMethod ������������rpc�������õĲ������л������緢��

    //һ��rpc������ɣ������õĽ��
    if (controller.Failed())
    {
        std::cout<<controller.ErrorText()<<std::endl;
    }
    else
    {
        if (0==response.result().errcode())
        {
            std::cout<<"rpc GetFriendsList response success!"<<std::endl;
            int size=response.friends_size();
            for (int i=0;i<size;i++)
            {
                std::cout<<"index:"<<(i+1)<<" name:"<<response.friends(i)<<std::endl;
            }
        }
        else
        {
            std::cout<<"rpc GetFriendsList response error:"<<response.result().errmsg()<<std::endl;
        }

    }
    
    return 0;
}