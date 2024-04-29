#include "mprpcchannel.h"
#include <string>
#include "rpcheader.pb.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include "mprpcapplication.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include "mprpccontroller.h"
#include "zookeeperutil.h"

/*
header_size+service_name method_name args_size+args �����л��ú����һ���ַ����ڷ��͹�ȥ
*/
//����ͨ��stub���������õ�rpc���������ߵ������ˣ�ͬ����rpc�������õ��������л������緢�� ����ִ�е�������������callmethod����
void MprpcChannel::CallMethod(const google::protobuf::MethodDescriptor* method,
                        google::protobuf::RpcController* controller,
                        const google::protobuf::Message* request,
                        google::protobuf::Message* response,
                        google::protobuf::Closure* done)
{    
    const google::protobuf::ServiceDescriptor* sd=method->service();
    std::string service_name=sd->name();//service_name
    std::string method_name=method->name();//method_name

    //��ȡ���������л��ַ������� args_size
    uint32_t args_size=0;
    std::string args_str;
    if (request->SerializeToString(&args_str))//���л�
    {
        args_size=args_str.size();
    }
    else
    {
        controller->SetFailed("serialize request error!");
        return;
    }

    //����rpc������header
    mprpc::RpcHeader rpcHeader;
    rpcHeader.set_service_name(service_name);
    rpcHeader.set_method_name(method_name);
    rpcHeader.set_args_size(args_size);

    uint32_t header_size=0;
    std::string rpc_header_str;
    if (rpcHeader.SerializeToString(&rpc_header_str))//���л�
    {
        header_size=rpc_header_str.size();
    }
    else
    {
        controller->SetFailed("serialize rpc header error!");
        return;
    }

    //��֯�����͵�rpc������ַ��� ��Ҫ�Ƕ����Ƶ� ����������ַ����޷�ȷ��ǰ4���ֽھ���ͷ���ȣ���߻�ɫ���֣�
    std::string send_rpc_str;
    send_rpc_str.insert(0,std::string((char*)&header_size,4));//д���ĸ��ֽ� �����Ƶ�header_size
    send_rpc_str+=rpc_header_str;//rpcheader
    send_rpc_str+=args_str;//args

    //��ӡ������Ϣ
    std::cout<<"========================================================"<<std::endl;
    std::cout<<"header_size "<<header_size<<std::endl;
    std::cout<<"rpc_header_str "<<rpc_header_str<<std::endl;
    std::cout<<"service_name "<<service_name<<std::endl;
    std::cout<<"method_name "<<method_name<<std::endl;
    std::cout<<"args_str "<<args_str<<std::endl;
    std::cout<<"========================================================"<<std::endl;

    //ʹ��tcp���,���rpc������Զ�̵���
    int clientfd=socket(AF_INET,SOCK_STREAM,0);
    if (-1==clientfd)
    {
        char errtxt[512]={0};
        sprintf(errtxt,"create socket error! errno:%d",errno);
        controller->SetFailed(errtxt);
        return;
    }

    //���ٶ�ȡ�����ļ�rpcserver����Ϣ������ֱ�Ӷ�ȡzk��
    // std::string ip=MprpcApplication::GetInstance().GetConfig().Load("rpcserverip");
    // uint16_t port=atoi(MprpcApplication::GetInstance().GetConfig().Load("rpcserverport").c_str());//�ַ���תΪ����
    // rpc���÷������service_name��method_name������Ҫ��ѯzk�ϸ÷������ڵ�host��Ϣ
    ZkClient zkCli;
    zkCli.Start();
    //  /UserServiceRpc/Login
    std::string method_path = "/" + service_name + "/" + method_name;
    // 127.0.0.1:8000
    std::string host_data = zkCli.GetData(method_path.c_str());
    if (host_data == "")
    {
        controller->SetFailed(method_path + " is not exist!");
        return;
    }
    int idx = host_data.find(":");
    if (idx == -1)
    {
        controller->SetFailed(method_path + " address is invalid!");
        return;
    }
    std::string ip = host_data.substr(0, idx);
    uint16_t port = atoi(host_data.substr(idx+1, host_data.size()-idx).c_str()); 

    struct sockaddr_in server_addr;
    server_addr.sin_family=AF_INET;
    server_addr.sin_port=htons(port);
    server_addr.sin_addr.s_addr=inet_addr(ip.c_str());

    //�������� ����rpc����ڵ�
    if (-1==connect(clientfd,(struct sockaddr*)&server_addr,sizeof(server_addr)))
    {
        close(clientfd);
        char errtxt[512]={0};
        sprintf(errtxt,"connect error! errno:%d",errno);
        controller->SetFailed(errtxt);
        return;
    }

    //����rpc����
    if (-1==send(clientfd,send_rpc_str.c_str(),send_rpc_str.size(),0))
    {
        
        close(clientfd);
        char errtxt[512]={0};
        sprintf(errtxt,"send error! errno:%d",errno);
        controller->SetFailed(errtxt);
        return;        
    }

    //����rpc�������Ӧֵ
    char recv_buf[1024]={0};
    int recv_size=0;
    if (-1==(recv_size=recv(clientfd,recv_buf,1024,0)))
    {
        //std::cout<<"recv error! errno:"<<errno<<std::endl;
        close(clientfd);
        char errtxt[512]={0};
        sprintf(errtxt,"recv error! errno:%d",errno);
        controller->SetFailed(errtxt);
        return; 
    }
    
    //�����л�rpc���õ���Ӧ
    //std::string response_str(recv_buf,0,recv_size);//bug��������,recv_buf������\0��������ݾʹ治�����ˣ����·����л�ʧ��
    if (!response->ParseFromArray(recv_buf,recv_size))//ֱ��ָ������
    {
        
        close(clientfd);
        char errtxt[512] = {0};
        sprintf(errtxt, "parse error! response_str:%s", recv_buf);
        controller->SetFailed(errtxt);
        return;
    }

    close(clientfd);






}