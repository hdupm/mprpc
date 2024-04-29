#pragma once
#include "mprpcconfig.h"
#include "mprpcapplication.h"
#include "rpcprovider.h"
#include "mprpccontroller.h"

//mprpc框架的初始化/基础类 负责框架的一些初始化操作
class MprpcApplication
{
public:
    static void Init(int argc,char **argv); //单例设置
    static MprpcApplication& GetInstance();
    static MprpcConfig& GetConfig();
    
private:
    static MprpcConfig m_config;
    MprpcApplication(){}
    MprpcApplication(const MprpcApplication&)=delete;  //拷贝构造函数相关去掉
    MprpcApplication(MprpcApplication&&)=delete;
};