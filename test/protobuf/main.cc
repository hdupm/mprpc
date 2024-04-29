#include "test.pb.h"
#include <iostream>
#include <string>
using namespace fixbug;

//里面许多方法都是继承而来 比如message
int main()
{
    // LoginResponse rsp;
    // ResultCode *rc=rsp.mutable_result(); //本身会生成一个类 获取其成员变量的指针
    // rc->set_errcode(1);
    // rc->set_errmsg("登陆处理失败了");

    GetFriendListsResponse rsp;
    ResultCode *rc=rsp.mutable_result();//成员变量是对象
    rc->set_errcode(0);

    User *user1=rsp.add_friend_list();
    user1->set_name("zhang san");
    user1->set_age(20);
    user1->set_sex(User::MAN);

    std::cout<<rsp.friend_list_size()<<std::endl;//好友个数

    return 0;
}

int main1()
{
    //封装了login请求对象的数据
    LoginRequest req;
    req.set_name("zhang san");
    req.set_pwd("123456");

    //对象数据序列化=》 char*
    std::string send_str;
    if (req.SerializeToString(&send_str))  //序列化 转换成二进制字符串send_str
    {
        std::cout<<send_str.c_str()<<std::endl;
    }

    //从send_str反序列化一个login请求对象
    LoginRequest reqB;
    if (reqB.ParseFromString(send_str))
    {
        std::cout<<reqB.name()<<std::endl;
        std::cout<<reqB.pwd()<<std::endl;
    }



    return 0;
}