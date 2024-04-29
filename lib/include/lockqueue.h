#pragma once
#include <queue>
#include <thread>
#include <mutex>//pthread_mutex_t������
#include <condition_variable>//pthread_condition_t��������

//�첽д��־����־����
template<typename T>
class LockQueue
{
public:
    //���������Ҫʵ���̰߳�ȫ ��ӳ������õ�ͬһ����
    //���worker�̶߳���д��־queue
    void Push(const T &data)
    {
        std::lock_guard<std::mutex> lock(m_mutex);//������ �������Զ������ͷ�
        m_queue.push(data);
        m_condvariable.notify_one();//��������notify one����Ϊֻ��һ���߳�������ȡ����д��־ ֻ��֪ͨһ��
    }

    //һ���̶߳���־queue��д��־�ļ�
    T Pop()
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        while (m_queue.empty())
        {
            //��־����Ϊ�գ��߳̽���wait״̬ �������е����ͷ� �߳����������� m_condvariable �ϵȴ���ֱ������ m_queue ����Ϊ��
            m_condvariable.wait(lock);
        }
        T data=m_queue.front();
        m_queue.pop();
        return data;
    }
private:
    std::queue<T> m_queue;
    std::mutex m_mutex;
    std::condition_variable m_condvariable;



};