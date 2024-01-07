#ifndef THREAD_POOL_H
#define THREAD_POOL_H
#include <cassert>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>

class ThreadPool
{
private:
    struct Pool {
        std::mutex m_mutex;
        std::condition_variable m_cond;
        bool m_isClosed;
        std::queue<std::function<void()>> m_tasks;
    };
    std::shared_ptr<Pool> m_pool;

public:
    // explict关键字用于修饰单参数构造函数，防止编译器在某些情况下自动执行隐式类型转换，以提高代码的明确性和安全性
    explicit ThreadPool(size_t threadCount = 8) : m_pool(std::make_shared<Pool>()) {
        assert(threadCount > 0);
        for (size_t i = 0; i < threadCount; i++) {
            std::thread([pool = m_pool] {
                /*工作线程运行的函数，它不断从任务队列中取出任务并执行*/
                std::unique_lock<std::mutex> locker(pool->m_mutex);
                while (true) {
                    if (!pool->m_tasks.empty()) {
                        // std::move将资源的所有权从一个对象转移到另一个对象，而不需要进行深拷贝操作
                        auto task = std::move(pool->m_tasks.front());
                        pool->m_tasks.pop();
                        locker.unlock(); // 因为已经把任务取出来了，所以可以提前解锁了
                        task();
                        locker.lock(); // 马上又要取任务了，上锁
                    } else if (pool->m_isClosed) {
                        break;
                    } else {
                        pool->m_cond.wait(locker); //任务空的情况下，阻塞等待添加任务
                    }
                }
                /*detach()的作用是将子线程和主线程的关联分离，
                 *也就是说detach()后子线程在后台独立继续运行，
                 *主线程无法再取得子线程的控制权，即使主线程结束，
                 *子线程未执行也不会结束。当主线程结束时，由运行时
                 *库负责清理与子线程相关的资源*/
            }).detach();
        }
    }
    ~ThreadPool() {
        if (static_cast<bool>(m_pool)) {
            {
                std::lock_guard<std::mutex> locker(m_pool->m_mutex);
                m_pool->m_isClosed = true;
            }
            m_pool->m_cond.notify_all();
        }
    }

    template <typename T>
    void AddTask(T &&task) {
        {
            std::unique_lock<std::mutex> locker(m_pool->m_mutex);
            m_pool->m_tasks.emplace(std::forward<T>(task));
        }
        m_pool->m_cond.notify_one();
    }
};

#endif // !THREAD_POOL_H