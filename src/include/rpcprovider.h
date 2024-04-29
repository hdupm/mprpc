#pragma once 
#include "google/protobuf/service.h"
#include "mprpcapplication.h"
#include <google/protobuf/descriptor.h>
//#include <memory>
#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/InetAddress.h>
#include <muduo/net/TcpConnection.h>
#include <unordered_map>


//����ṩ��ר�ŷ��񷢲�rpc��������������
class RpcProvider
{
public:
    //�����ǿ���ṩ���ⲿʹ�õ� ���Է���rpc�����ĺ����ӿ�
    void NotifyService(google::protobuf::Service *service);

    //����rpc����ڵ� ��ʼ�ṩrpcԶ�̵��÷���
    void Run();

private:
    // //�����TcpServer
    // std::unique_ptr<muduo::net::TcpServer> m_tcpserverPtr;
    //���EventLoop
    muduo::net::EventLoop m_eventLoop;

    //service����������Ϣ
    struct ServiceInfo
    {
        google::protobuf::Service *m_service;//����������
        std::unordered_map<std::string,const google::protobuf::MethodDescriptor*> m_methodMap;//������񷽷�

    };
    //�洢ע��ɹ��ķ���������ֺ�����񷽷���������Ϣ
    std::unordered_map<std::string,ServiceInfo> m_serviceMap;

    //�µ�socket���ӻص�
    void OnConnection(const muduo::net::TcpConnectionPtr&);
    //�ѽ��������û��Ķ�д�¼��ص�
    void OnMessage(const muduo::net::TcpConnectionPtr&,muduo::net::Buffer*,muduo::Timestamp);
    //Closure�Ļص��������������л�rpc����Ӧ�����緢��
    void SendRpcResponse(const muduo::net::TcpConnectionPtr&,google::protobuf::Message*);



};