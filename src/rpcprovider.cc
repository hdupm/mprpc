#include "rpcprovider.h"
#include <functional>
#include <string>
#include "rpcheader.pb.h"//����������һ���� �Ƕ������������
#include "logger.h"
#include "zookeeperutil.h"

/*
service_name=> service����
                    =��service* ��¼�������
                    method_name =�� method��������
json��ֵ�� �����ı��洢 protobuf���ڶ����ƴ洢 

*/
//���ǿ���ṩ���ⲿʹ�õģ����Է���rpc�����ĺ����ӿ�
void RpcProvider::NotifyService(google::protobuf::Service *service) //ͨ������ָ��ʵ�� ����ע����ҵ��ϸ��
{
    ServiceInfo service_info;
    //��ȡ�˷�������������Ϣ
    const google::protobuf::ServiceDescriptor *pserviceDesc=service->GetDescriptor();
    //��ȡ���������
    std::string service_name=pserviceDesc->name();
    //��ȡ�������service�ķ���������
    int methodCnt=pserviceDesc->method_count(); 

    //std::cout<<"service_name:"<<service_name<<std::endl;
    LOG_INFO("service_name:%s",service_name.c_str());

    for (int i=0;i<methodCnt;++i)
    {
        //��ȡ�˷������ָ���±�ķ��񷽷�������(��������) ����UserServive��������Login����
        const google::protobuf::MethodDescriptor* pmethodDesc=pserviceDesc->method(i);
        std::string method_name=pmethodDesc->name();
        service_info.m_methodMap.insert({method_name,pmethodDesc});
        //std::cout<<"method_name:"<<method_name<<std::endl;
        LOG_INFO("method_name:%s",method_name.c_str());

    }
    service_info.m_service=service;
    m_serviceMap.insert({service_name,service_info});

}

//����rpc����ڵ� ��ʼ�ṩrpcԶ�̵��÷��� �൱�ڿ���epoll
void RpcProvider::Run()
{
    //��ȡ�����ļ�rpcserver����Ϣ
    std::string ip=MprpcApplication::GetInstance().GetConfig().Load("rpcserverip");
    uint16_t port=atoi(MprpcApplication::GetInstance().GetConfig().Load("rpcserverport").c_str());//�ַ���תΪ����
    muduo::net::InetAddress address(ip,port);

    //����tcpserver����
    muduo::net::TcpServer server(&m_eventLoop,address,"RpcProvider");
    //�����ӻص�����Ϣ��д�ص����� ��������������ҵ�����
    server.setConnectionCallback(std::bind(&RpcProvider::OnConnection,this, std::placeholders::_1));
    server.setMessageCallback(std::bind(&RpcProvider::OnMessage,this, std::placeholders::_1,
            std::placeholders::_2,std::placeholders::_3));


    //����muduo����߳����� muduo���Զ��ַ������̺߳�io�߳� ������߳� �Զ�һ���̸߳���io�������û� ������ǹ����߳�
    server.setThreadNum(4);

    // �ѵ�ǰrpc�ڵ���Ҫ�����ķ���ȫ��ע�ᵽzk���棬��rpc client���Դ�zk�Ϸ��ַ���
    // session timeout   30s     zkclient ����I/O�߳�  1/3 * timeout ʱ�䷢��ping��Ϣ
    ZkClient zkCli;//�ͻ���
    zkCli.Start();//����zkserver
    // service_nameΪ�����Խڵ�    method_nameΪ��ʱ�Խڵ�
    for (auto &sp : m_serviceMap) 
    {
        // /service_name   /UserServiceRpc Ҫ�ȴ������ڵ㣬�ڴ���������ӽڵ�
        std::string service_path = "/" + sp.first;
        zkCli.Create(service_path.c_str(), nullptr, 0);
        for (auto &mp : sp.second.m_methodMap)
        {
            // /service_name/method_name   /UserServiceRpc/Login �洢��ǰ���rpc����ڵ�������ip��port
            std::string method_path = service_path + "/" + mp.first;
            char method_path_data[128] = {0};
            sprintf(method_path_data, "%s:%d", ip.c_str(), port);
            // ZOO_EPHEMERAL��ʾznode��һ����ʱ�Խڵ�
            zkCli.Create(method_path.c_str(), method_path_data, strlen(method_path_data), ZOO_EPHEMERAL);
        }
    }

    std::cout<<"RpcProvider start service at ip:"<<ip<<"  port:"<<port<<std::endl;

    //����������� �൱�������Ƿ�������Ҳ���zk��������
    server.start();
    m_eventLoop.loop();

    
}

//�µ�socket���ӻص� rpc���ƶ����� ��Ӧ���Զ��ر�
void RpcProvider::OnConnection(const muduo::net::TcpConnectionPtr& conn)
{
    if (!conn->connected())
    {
        //��rpc client�����ӶϿ���
        conn->shutdown();
    }
}

/*
�ڿ���ڲ� RpcProvider��RpcConsumerЭ�̺�֮��ͨ���õ�protobuf�������� ��Ϊrpc��������������һ����/����ķ��� ����ͨ������������
service_name method_name args ����ж���proto��message���ͣ���������ͷ�����л��ͷ����л� 
                                header_str����service_name method_name args_size ��¼�������ȷ�ֹճ������
16UserServiceLoginzhang san123456
header_size(4���ֽ� ������ǰ������һ������ �������ͷ�����)+header_str(service_name method_name args_size)+args_str �����ָ�֪����Щ�Ƿ�����������
std::string insert��copy����
*/
//�ѽ��������û��Ķ�д�¼��ص�
void RpcProvider::OnMessage(const muduo::net::TcpConnectionPtr& conn,muduo::net::Buffer* buffer,muduo::Timestamp)
{
    //�����Ͻ��ܵ�Զ��rpc����������ַ���   Login args   string���ֽ�����
    std::string recv_buf=buffer->retrieveAllAsString();

    //���ַ����ж�ȡǰ4���ֽڵ����� һ�����Ǵ�Ŷ����Ƶ� ������ַ������޷������Ƿ���4���ֽ�
    uint32_t header_size=0;
    recv_buf.copy((char*)&header_size,4,0);//����ָ���ĸ��ֽ����ݵ�header_size��

    //����header_size��ȡ����ͷ��ԭʼ�ַ��� (service_name method_name args_size) �����л����ݣ��õ�rpc�������ϸ��Ϣ
    std::string rpc_header_str=recv_buf.substr(4,header_size);
    mprpc::RpcHeader rpcHeader;//�ַ���������ȫ������
    std::string service_name;
    std::string method_name;
    uint32_t args_size;
    if (rpcHeader.ParseFromString(rpc_header_str))
    {
        //����ͷ�����л��ɹ�
        service_name=rpcHeader.service_name();
        method_name=rpcHeader.method_name();
        args_size=rpcHeader.args_size();
    }
    else
    {
        //����ͷ�����л�ʧ��
        std::cout<<"rpc_header_str:"<<rpc_header_str<<"parse error!"<<std::endl;
        return;
    }
    //��ȡrpc�����������ַ�������
    std::string args_str=recv_buf.substr(4+header_size,args_size);

    //��ӡ������Ϣ
    std::cout<<"========================================================"<<std::endl;
    std::cout<<"header_size"<<header_size<<std::endl;
    std::cout<<"rpc_header_str"<<rpc_header_str<<std::endl;
    std::cout<<"service_name"<<service_name<<std::endl;
    std::cout<<"method_name"<<method_name<<std::endl;
    std::cout<<"args_str"<<args_str<<std::endl;
    std::cout<<"========================================================"<<std::endl;

    //��ȡservice�����method����
    auto it=m_serviceMap.find(service_name);
    if (it==m_serviceMap.end())
    {
        std::cout<<service_name<<" is not exist! "<<std::endl;
        return;
    }    

    auto mit=it->second.m_methodMap.find(method_name);
    if (mit==it->second.m_methodMap.end())
    {
        std::cout<<service_name<<":"<<method_name<<" is not exist! "<<std::endl;
        return;
    }

    google::protobuf::Service *service=it->second.m_service;//��ȡservice���� new UserService
    const google::protobuf::MethodDescriptor *method=mit->second;//��ȡmethod���� Login

    //����rpc�������õ�����request����Ӧresponse���� ���������Ǽ̳�message�࣬���ʹ�ÿ�� �ó���ķ�ʽ��ʾ
    google::protobuf::Message *request=service->GetRequestPrototype(method).New();//��ȡĳ����������Ӧ������ Ҳ����LoginRequest����    
    if (!request->ParseFromString(args_str))
    {
        std::cout<<"request parse error,content:"<<args_str<<std::endl;
        return;
    }
    google::protobuf::Message *response=service->GetResponsePrototype(method).New();

    //�������method�����ĵ��ã���һ��Closure�Ļص�����    
    google::protobuf::Closure *done=google::protobuf::NewCallback<RpcProvider,const muduo::net::TcpConnectionPtr&,google::protobuf::Message*>(this,&RpcProvider::SendRpcResponse,conn,response);

    //�ڿ���ϸ���Զ��rpc���󣬵��õ�ǰrpc�ڵ��Ϸ����ķ���  (�ɿ��������) �����û���ָ�� ���󷽷�������
    //ʵ������new UserService().Login(controller,request,response,done)
    service->CallMethod(method,nullptr,request,response,done);//����õ���д�õ�response������Ӧ CallMethod����Щ����������login�� ��װ�ײ�

}

//Closure�Ļص��������������л�rpc����Ӧ�����緢�� response��ҵ�����д�õ�
void RpcProvider::SendRpcResponse(const muduo::net::TcpConnectionPtr& conn,google::protobuf::Message* response)
{    
    std::string response_str;
    if (response->SerializeToString(&response_str))//��ɫresponse���л�����
    {
        //���л��ɹ���ͨ�������rpc����ִ�еĽ��������rpc�ĵ��÷�
        conn->send(response_str);
        
    }
    else
    {
        std::cout<<"serialize response_str error!"<<std::endl;
    }
    conn->shutdown();//ģ��http�Ķ����ӷ�����rpcprovider�����Ͽ�����

}