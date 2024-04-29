#include "test.pb.h"
#include <iostream>
#include <string>
using namespace fixbug;

//������෽�����Ǽ̳ж��� ����message
int main()
{
    // LoginResponse rsp;
    // ResultCode *rc=rsp.mutable_result(); //���������һ���� ��ȡ���Ա������ָ��
    // rc->set_errcode(1);
    // rc->set_errmsg("��½����ʧ����");

    GetFriendListsResponse rsp;
    ResultCode *rc=rsp.mutable_result();//��Ա�����Ƕ���
    rc->set_errcode(0);

    User *user1=rsp.add_friend_list();
    user1->set_name("zhang san");
    user1->set_age(20);
    user1->set_sex(User::MAN);

    std::cout<<rsp.friend_list_size()<<std::endl;//���Ѹ���

    return 0;
}

int main1()
{
    //��װ��login������������
    LoginRequest req;
    req.set_name("zhang san");
    req.set_pwd("123456");

    //�����������л�=�� char*
    std::string send_str;
    if (req.SerializeToString(&send_str))  //���л� ת���ɶ������ַ���send_str
    {
        std::cout<<send_str.c_str()<<std::endl;
    }

    //��send_str�����л�һ��login�������
    LoginRequest reqB;
    if (reqB.ParseFromString(send_str))
    {
        std::cout<<reqB.name()<<std::endl;
        std::cout<<reqB.pwd()<<std::endl;
    }



    return 0;
}