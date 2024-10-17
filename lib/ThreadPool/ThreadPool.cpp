#include "ThreadPool.hpp"

std::once_flag 				ThreadPool::m_Flag;
std::once_flag 				ThreadPool::m_RunFlag;
std::once_flag 				ThreadPool::m_CleanFlag;
std::unique_ptr<ThreadPool> ThreadPool::m_pInstance = nullptr;

ThreadPool::ThreadPool()
{		
	m_bStop = false;
}

ThreadPool::~ThreadPool()
{
	clean();
}

ThreadPool* ThreadPool::getInstance()
{
	std::call_once(m_Flag, []
	{
		std::unique_ptr<ThreadPool> pThreadPool(new ThreadPool);
		m_pInstance = std::move(pThreadPool);
	});

    return m_pInstance.get();
}

void ThreadPool::run(int nSize)
{
	std::call_once(m_RunFlag, [&]
	{
		std::function<void()> worker = [this]
		{
			while (true)
			{
				std::function<void()> task;

				{
					std::unique_lock<std::mutex> lock(m_mutex);

					m_Condition.wait(lock, [this] { return m_bStop || !m_Tasks.empty(); });
					
					if (m_bStop && m_Tasks.empty())
					{
						break;
					}

					task = move(m_Tasks.front());
					m_Tasks.pop();
				}
				
				task();
			}
		};
	
		int nThreadPoolSize = nSize > 0 ? nSize : DEFAULT_THREAD_POOL_SIZE;

		for (int ni = 0 ; ni < nThreadPoolSize ; ni++)	
		{
			m_Workers.emplace_back(worker);
		}	
	});	
}

void ThreadPool::stop()
{
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		m_bStop = true;
	}
	
	m_Condition.notify_all();
};

void ThreadPool::wait()
{	
	for(size_t ni = 0; ni < m_Workers.size(); ni++)
	{		
		if(m_Workers[ni].joinable())
		{
			m_Workers[ni].join();
		}		
	}	
}

void ThreadPool::clean()
{
	std::call_once(m_CleanFlag, [this]
	{
		stop();
		wait();
	});	
}