#pragma once
#include "mprpcconfig.h"
#include "mprpcapplication.h"
#include "rpcprovider.h"
#include "mprpccontroller.h"

//mprpc��ܵĳ�ʼ��/������ �����ܵ�һЩ��ʼ������
class MprpcApplication
{
public:
    static void Init(int argc,char **argv); //��������
    static MprpcApplication& GetInstance();
    static MprpcConfig& GetConfig();
    
private:
    static MprpcConfig m_config;
    MprpcApplication(){}
    MprpcApplication(const MprpcApplication&)=delete;  //�������캯�����ȥ��
    MprpcApplication(MprpcApplication&&)=delete;
};