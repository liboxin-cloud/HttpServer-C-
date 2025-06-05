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


//线程池类
class ThreadPool
{
public:
	ThreadPool() {};
	ThreadPool(size_t numThread) : stop(false)//初始化stop变量为false
	{
		for (size_t i = 0; i < numThread; i++)
		{
			//创建线程
			threads.emplace_back([this]
				{
					while (true)
					{
						function<void()> task;
						{
							//创建一个mutex锁，防止线程之间竞争访问
							unique_lock<mutex> lock(this->queueMutex);
							this->condition.wait(lock, [this] {return this->stop || !this->tasks.empty(); });
							//如果线程池的所有任务都停止，或者是tasks中为空，那么直接返回
							if (this->stop && this->tasks.empty())
							{
								return;
							}
							//新线程从头部入队
							task = move(this->tasks.front());
							//旧线程直接从尾部出队
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
			stop = true;//将线程池的状态设置为停止
		}
		condition.notify_all();
		for (thread& thread : threads)
		{
			//阻塞，等待所有线程退出
			thread.join();
		}
	}
	template<typename F>
	void enqueue(F&& f);

private:
	vector<thread> threads;//创建线程容器
	queue<function<void()>> tasks;//创建函数指针队列
	mutex queueMutex;//创建一个队列锁
	condition_variable condition;
	bool stop;//判断是否停止

};

template<typename F>
inline void ThreadPool::enqueue(F&& f)
{
	{
		unique_lock<mutex> lock(queueMutex);
		if (stop)
		{
			//如果线程池的状态是停止线程还耀继续运行，那么直接报错
			throw runtime_error("enqueue on stopped ThreadPool");

		}
		tasks.emplace(forward<F>(f));

	}
	condition.notify_one();
}
