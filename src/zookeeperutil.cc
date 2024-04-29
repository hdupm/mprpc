#include "zookeeperutil.h"
#include <semaphore.h> //�ź���
#include <mprpcapplication.h>
#include <iostream>

// ȫ�ֵ�watcher�۲���   zkserver��zkclient��֪ͨ ���ǵ������߳������е�
void global_watcher(zhandle_t *zh, int type,
                   int state, const char *path, void *watcherCtx)
{
    if (type == ZOO_SESSION_EVENT)  // �ص�����Ϣ�����ǺͻỰ��ص���Ϣ����
	{
		if (state == ZOO_CONNECTED_STATE)  // zkclient��zkserver���ӳɹ�
		{
			sem_t *sem = (sem_t*)zoo_get_context(zh);//��ָ�������ȡ�ź���
            sem_post(sem);//����Դ��+1
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
        zookeeper_close(m_zhandle); // �رվ�����ͷ���Դ  �൱��MySQL_Conn
    }
}

// ����zkserver
void ZkClient::Start()
{
    std::string host = MprpcApplication::GetInstance().GetConfig().Load("zookeeperip");
    std::string port = MprpcApplication::GetInstance().GetConfig().Load("zookeeperport");
    std::string connstr = host + ":" + port;
    
	/*
	zookeeper_mt�����̰߳汾
	zookeeper��API�ͻ��˳����ṩ�������߳� ���첽���ӹ���
	API�����߳� ��ǰ�߳� ִ��zookeeper_init
	����I/O�߳�  pthread_create  poll ������һ���߳�������io
	watcher�ص��߳� pthread_create ���յ���Ӧ
	*/
    m_zhandle = zookeeper_init(connstr.c_str(), global_watcher, 30000, nullptr, nullptr, 0);//30000����Ự��ʱʱ��30s �������ʼ���Ƿ�ɹ� ������������
    if (nullptr == m_zhandle) 
    {
        std::cout << "zookeeper_init error!" << std::endl;
        exit(EXIT_FAILURE);
    }

    sem_t sem;
    sem_init(&sem, 0, 0);//��ʼ��Ϊ0
    zoo_set_context(m_zhandle, &sem);//��ָ����������ź���

    sem_wait(&sem);//�����ȴ��ź��� ����Դ��������
    std::cout << "zookeeper_init success!" << std::endl;
}

void ZkClient::Create(const char *path, const char *data, int datalen, int state)//path��ÿ���ڵ��·��
{
    char path_buffer[128];
    int bufferlen = sizeof(path_buffer);
    int flag;
	// ���ж�path��ʾ��znode�ڵ��Ƿ���ڣ�������ڣ��Ͳ����ظ�������
	flag = zoo_exists(m_zhandle, path, 0, nullptr);
	if (ZNONODE == flag) // ��ʾpath��znode�ڵ㲻����
	{
		// ����ָ��path��znode�ڵ���
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

// ����ָ����path����ȡznode�ڵ��ֵ
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