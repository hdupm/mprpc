#include "rpcprovider.h"
#include <functional>
#include <string>
#include "rpcheader.pb.h"//里面生成了一个类 是定义的数据类型
#include "logger.h"
#include "zookeeperutil.h"

/*
service_name=> service描述
                    =》service* 记录服务对象
                    method_name =》 method方法对象
json键值对 基于文本存储 protobuf基于二进制存储 

*/
//这是框架提供给外部使用的，可以发布rpc方法的函数接口
void RpcProvider::NotifyService(google::protobuf::Service *service) //通过基类指针实现 不关注具体业务细节
{
    ServiceInfo service_info;
    //获取了服务对象的描述信息
    const google::protobuf::ServiceDescriptor *pserviceDesc=service->GetDescriptor();
    //获取服务的名字
    std::string service_name=pserviceDesc->name();
    //获取服务对象service的方法的数量
    int methodCnt=pserviceDesc->method_count(); 

    //std::cout<<"service_name:"<<service_name<<std::endl;
    LOG_INFO("service_name:%s",service_name.c_str());

    for (int i=0;i<methodCnt;++i)
    {
        //获取了服务对象指定下标的服务方法的描述(抽象描述) 调用UserServive服务对象的Login方法
        const google::protobuf::MethodDescriptor* pmethodDesc=pserviceDesc->method(i);
        std::string method_name=pmethodDesc->name();
        service_info.m_methodMap.insert({method_name,pmethodDesc});
        //std::cout<<"method_name:"<<method_name<<std::endl;
        LOG_INFO("method_name:%s",method_name.c_str());

    }
    service_info.m_service=service;
    m_serviceMap.insert({service_name,service_info});

}

//启动rpc服务节点 开始提供rpc远程调用服务 相当于开启epoll
void RpcProvider::Run()
{
    //读取配置文件rpcserver的信息
    std::string ip=MprpcApplication::GetInstance().GetConfig().Load("rpcserverip");
    uint16_t port=atoi(MprpcApplication::GetInstance().GetConfig().Load("rpcserverport").c_str());//字符串转为整数
    muduo::net::InetAddress address(ip,port);

    //创建tcpserver对象
    muduo::net::TcpServer server(&m_eventLoop,address,"RpcProvider");
    //绑定连接回调和消息读写回调方法 分离了网络代码和业务代码
    server.setConnectionCallback(std::bind(&RpcProvider::OnConnection,this, std::placeholders::_1));
    server.setMessageCallback(std::bind(&RpcProvider::OnMessage,this, std::placeholders::_1,
            std::placeholders::_2,std::placeholders::_3));


    //设置muduo库的线程数量 muduo会自动分发工作线程和io线程 如果多线程 自动一个线程负责io连接新用户 另外的是工作线程
    server.setThreadNum(4);

    // 把当前rpc节点上要发布的服务全部注册到zk上面，让rpc client可以从zk上发现服务
    // session timeout   30s     zkclient 网络I/O线程  1/3 * timeout 时间发送ping消息
    ZkClient zkCli;//客户端
    zkCli.Start();//连接zkserver
    // service_name为永久性节点    method_name为临时性节点
    for (auto &sp : m_serviceMap) 
    {
        // /service_name   /UserServiceRpc 要先创建父节点，在创建下面的子节点
        std::string service_path = "/" + sp.first;
        zkCli.Create(service_path.c_str(), nullptr, 0);
        for (auto &mp : sp.second.m_methodMap)
        {
            // /service_name/method_name   /UserServiceRpc/Login 存储当前这个rpc服务节点主机的ip和port
            std::string method_path = service_path + "/" + mp.first;
            char method_path_data[128] = {0};
            sprintf(method_path_data, "%s:%d", ip.c_str(), port);
            // ZOO_EPHEMERAL表示znode是一个临时性节点
            zkCli.Create(method_path.c_str(), method_path_data, strlen(method_path_data), ZOO_EPHEMERAL);
        }
    }

    std::cout<<"RpcProvider start service at ip:"<<ip<<"  port:"<<port<<std::endl;

    //启动网络服务 相当于自身是服务器，也会和zk保持连接
    server.start();
    m_eventLoop.loop();

    
}

//新的socket连接回调 rpc类似短连接 响应后自动关闭
void RpcProvider::OnConnection(const muduo::net::TcpConnectionPtr& conn)
{
    if (!conn->connected())
    {
        //和rpc client的连接断开了
        conn->shutdown();
    }
}

/*
在框架内部 RpcProvider和RpcConsumer协商好之间通信用的protobuf数据类型 因为rpc方法都发布成了一个类/对象的方法 必须通过服务对象调用
service_name method_name args 框架中定义proto的message类型，进行数据头的序列化和反序列化 
                                header_str包括service_name method_name args_size 记录参数长度防止粘包问题
16UserServiceLoginzhang san123456
header_size(4个字节 即参数前的数据一个整数 服务名和方法名)+header_str(service_name method_name args_size)+args_str 这样分割知道哪些是服务名方法名
std::string insert和copy方法
*/
//已建立连接用户的读写事件回调
void RpcProvider::OnMessage(const muduo::net::TcpConnectionPtr& conn,muduo::net::Buffer* buffer,muduo::Timestamp)
{
    //网络上接受的远程rpc调用请求的字符流   Login args   string是字节序列
    std::string recv_buf=buffer->retrieveAllAsString();

    //从字符流中读取前4个字节的内容 一定得是存放二进制的 如果是字符串就无法控制是否是4个字节
    uint32_t header_size=0;
    recv_buf.copy((char*)&header_size,4,0);//拷贝指定四个字节内容到header_size中

    //根据header_size读取数据头的原始字符流 (service_name method_name args_size) 反序列化数据，得到rpc请求的详细信息
    std::string rpc_header_str=recv_buf.substr(4,header_size);
    mprpc::RpcHeader rpcHeader;//字符流中数据全部放入
    std::string service_name;
    std::string method_name;
    uint32_t args_size;
    if (rpcHeader.ParseFromString(rpc_header_str))
    {
        //数据头反序列化成功
        service_name=rpcHeader.service_name();
        method_name=rpcHeader.method_name();
        args_size=rpcHeader.args_size();
    }
    else
    {
        //数据头反序列化失败
        std::cout<<"rpc_header_str:"<<rpc_header_str<<"parse error!"<<std::endl;
        return;
    }
    //获取rpc方法参数的字符流数据
    std::string args_str=recv_buf.substr(4+header_size,args_size);

    //打印调试信息
    std::cout<<"========================================================"<<std::endl;
    std::cout<<"header_size"<<header_size<<std::endl;
    std::cout<<"rpc_header_str"<<rpc_header_str<<std::endl;
    std::cout<<"service_name"<<service_name<<std::endl;
    std::cout<<"method_name"<<method_name<<std::endl;
    std::cout<<"args_str"<<args_str<<std::endl;
    std::cout<<"========================================================"<<std::endl;

    //获取service对象和method对象
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

    google::protobuf::Service *service=it->second.m_service;//获取service对象 new UserService
    const google::protobuf::MethodDescriptor *method=mit->second;//获取method对象 Login

    //生成rpc方法调用的请求request和响应response参数 这两个都是继承message类，因此使用框架 用抽象的方式表示
    google::protobuf::Message *request=service->GetRequestPrototype(method).New();//获取某个方法的响应的类型 也就是LoginRequest对象    
    if (!request->ParseFromString(args_str))
    {
        std::cout<<"request parse error,content:"<<args_str<<std::endl;
        return;
    }
    google::protobuf::Message *response=service->GetResponsePrototype(method).New();

    //给下面的method方法的调用，绑定一个Closure的回调函数    
    google::protobuf::Closure *done=google::protobuf::NewCallback<RpcProvider,const muduo::net::TcpConnectionPtr&,google::protobuf::Message*>(this,&RpcProvider::SendRpcResponse,conn,response);

    //在框架上根据远端rpc请求，调用当前rpc节点上发布的方法  (由框架来调用) 必须用基类指针 抽象方法来调用
    //实际上是new UserService().Login(controller,request,response,done)
    service->CallMethod(method,nullptr,request,response,done);//框架拿到填写好的response进行响应 CallMethod将这些参数都传入login中 封装底层

}

//Closure的回调操作，用于序列化rpc的响应和网络发送 response是业务层填写好的
void RpcProvider::SendRpcResponse(const muduo::net::TcpConnectionPtr& conn,google::protobuf::Message* response)
{    
    std::string response_str;
    if (response->SerializeToString(&response_str))//黄色response序列化部分
    {
        //序列化成功后，通过网络把rpc方法执行的结果发动回rpc的调用方
        conn->send(response_str);
        
    }
    else
    {
        std::cout<<"serialize response_str error!"<<std::endl;
    }
    conn->shutdown();//模拟http的短连接服务，由rpcprovider主动断开连接

}