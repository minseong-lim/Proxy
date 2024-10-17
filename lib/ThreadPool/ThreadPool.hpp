#pragma once
#include <iostream>
#include <thread>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>

constexpr auto DEFAULT_THREAD_POOL_SIZE = 20;

class ThreadPool
{
public:
    //////////////////////////////////////////////////////////////    
    ThreadPool(const ThreadPool& other)             = delete;
    ThreadPool(ThreadPool&& other)                  = delete;
    ThreadPool& operator=(const ThreadPool& other)  = delete;
    ThreadPool& operator=(ThreadPool&& other)       = delete;
    //////////////////////////////////////////////////////////////
    
    virtual ~ThreadPool();
    static ThreadPool* getInstance();
    
    void run(int nSize = DEFAULT_THREAD_POOL_SIZE);
    void clean();    

    template<typename F, typename... Args>
    void enqueue(F&& f, Args&&... args)
    {
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_Tasks.emplace([f = std::forward<F>(f), args = std::make_tuple(std::ref(m_bStop), std::forward<Args>(args)...)]
            {
                std::apply(f, args);
            });
        }
        m_Condition.notify_one();
    }
private:
    std::mutex                              m_mutex;
    std::vector<std::thread>                m_Workers;
    std::queue<std::function<void()>>       m_Tasks;
    std::condition_variable                 m_Condition;
    std::atomic<bool>                       m_bStop;

    // For singleton
    static std::once_flag                   m_Flag;
    static std::once_flag                   m_RunFlag;
    static std::once_flag                   m_CleanFlag;
    static std::unique_ptr<ThreadPool>      m_pInstance;

    ThreadPool();
    void stop();
    void wait();
};