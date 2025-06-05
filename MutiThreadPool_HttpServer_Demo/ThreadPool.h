#pragma once
#include <iostream>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <memory>

using namespace std;


//�̳߳���
class ThreadPool
{
public:
	ThreadPool() {};
	ThreadPool(size_t numThread) : stop(false)//��ʼ��stop����Ϊfalse
	{
		for (size_t i = 0; i < numThread; i++)
		{
			//�����߳�
			threads.emplace_back([this]
				{
					while (true)
					{
						function<void()> task;
						{
							//����һ��mutex������ֹ�߳�֮�侺������
							unique_lock<mutex> lock(this->queueMutex);
							this->condition.wait(lock, [this] {return this->stop || !this->tasks.empty(); });
							//����̳߳ص���������ֹͣ��������tasks��Ϊ�գ���ôֱ�ӷ���
							if (this->stop && this->tasks.empty())
							{
								return;
							}
							//���̴߳�ͷ�����
							task = move(this->tasks.front());
							//���߳�ֱ�Ӵ�β������
							this->tasks.pop();

						}
						task();
					}
				});
		}
	}
	~ThreadPool() 
	{
		{
			unique_lock<mutex> lock(queueMutex);
			stop = true;//���̳߳ص�״̬����Ϊֹͣ
		}
		condition.notify_all();
		for (thread& thread : threads)
		{
			//�������ȴ������߳��˳�
			thread.join();
		}
	}
	template<typename F>
	void enqueue(F&& f);

private:
	vector<thread> threads;//�����߳�����
	queue<function<void()>> tasks;//��������ָ�����
	mutex queueMutex;//����һ��������
	condition_variable condition;
	bool stop;//�ж��Ƿ�ֹͣ

};

template<typename F>
inline void ThreadPool::enqueue(F&& f)
{
	{
		unique_lock<mutex> lock(queueMutex);
		if (stop)
		{
			//����̳߳ص�״̬��ֹͣ�̻߳�ҫ�������У���ôֱ�ӱ���
			throw runtime_error("enqueue on stopped ThreadPool");

		}
		tasks.emplace(forward<F>(f));

	}
	condition.notify_one();
}
