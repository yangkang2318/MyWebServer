#ifndef SEMAPHORE_H
#define SEMAPHORE_H

#include <condition_variable>
#include <mutex>
class Semaphore
{
private:
    int m_count;
    std::mutex m_mutex;
    std::condition_variable m_cond;

public:
    Semaphore(){};
    ~Semaphore(){};

    void InitSem(int count) {
        m_count = count;
    }

    void Acquire() {
        std::unique_lock<std::mutex> locker(m_mutex);
        while (m_count == 0) {
            m_cond.wait(locker);
        }
        m_count--;
    }

    void Release() {
        std::lock_guard<std::mutex> locker(m_mutex);
        m_count++;
        m_cond.notify_one();
    }
};

#endif // !SEMAPHORE_H