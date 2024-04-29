#include "zookeeperutil.h"
#include <semaphore.h> //信号量
#include <mprpcapplication.h>
#include <iostream>

// 全局的watcher观察器   zkserver给zkclient的通知 这是单独的线程中运行的
void global_watcher(zhandle_t *zh, int type,
                   int state, const char *path, void *watcherCtx)
{
    if (type == ZOO_SESSION_EVENT)  // 回调的消息类型是和会话相关的消息类型
	{
		if (state == ZOO_CONNECTED_STATE)  // zkclient和zkserver连接成功
		{
			sem_t *sem = (sem_t*)zoo_get_context(zh);//从指定句柄获取信号量
            sem_post(sem);//给资源量+1
		}
	}
}

ZkClient::ZkClient() : m_zhandle(nullptr)
{
}

ZkClient::~ZkClient()
{
    if (m_zhandle != nullptr)
    {
        zookeeper_close(m_zhandle); // 关闭句柄，释放资源  相当于MySQL_Conn
    }
}

// 连接zkserver
void ZkClient::Start()
{
    std::string host = MprpcApplication::GetInstance().GetConfig().Load("zookeeperip");
    std::string port = MprpcApplication::GetInstance().GetConfig().Load("zookeeperport");
    std::string connstr = host + ":" + port;
    
	/*
	zookeeper_mt：多线程版本
	zookeeper的API客户端程序提供了三个线程 是异步连接过程
	API调用线程 当前线程 执行zookeeper_init
	网络I/O线程  pthread_create  poll 创建了一个线程做网络io
	watcher回调线程 pthread_create 接收到响应
	*/
    m_zhandle = zookeeper_init(connstr.c_str(), global_watcher, 30000, nullptr, nullptr, 0);//30000代表会话超时时间30s 仅代表初始化是否成功 并不代表连接
    if (nullptr == m_zhandle) 
    {
        std::cout << "zookeeper_init error!" << std::endl;
        exit(EXIT_FAILURE);
    }

    sem_t sem;
    sem_init(&sem, 0, 0);//初始化为0
    zoo_set_context(m_zhandle, &sem);//给指定句柄设置信号量

    sem_wait(&sem);//阻塞等待信号量 有资源代表连接
    std::cout << "zookeeper_init success!" << std::endl;
}

void ZkClient::Create(const char *path, const char *data, int datalen, int state)//path是每个节点的路径
{
    char path_buffer[128];
    int bufferlen = sizeof(path_buffer);
    int flag;
	// 先判断path表示的znode节点是否存在，如果存在，就不再重复创建了
	flag = zoo_exists(m_zhandle, path, 0, nullptr);
	if (ZNONODE == flag) // 表示path的znode节点不存在
	{
		// 创建指定path的znode节点了
		flag = zoo_create(m_zhandle, path, data, datalen,
			&ZOO_OPEN_ACL_UNSAFE, state, path_buffer, bufferlen);
		if (flag == ZOK)
		{
			std::cout << "znode create success... path:" << path << std::endl;
		}
		else
		{
			std::cout << "flag:" << flag << std::endl;
			std::cout << "znode create error... path:" << path << std::endl;
			exit(EXIT_FAILURE);
		}
	}
}

// 根据指定的path，获取znode节点的值
std::string ZkClient::GetData(const char *path)
{
    char buffer[64];
	int bufferlen = sizeof(buffer);
	int flag = zoo_get(m_zhandle, path, 0, buffer, &bufferlen, nullptr);
	if (flag != ZOK)
	{
		std::cout << "get znode error... path:" << path << std::endl;
		return "";
	}
	else
	{
		return buffer;
	}
}