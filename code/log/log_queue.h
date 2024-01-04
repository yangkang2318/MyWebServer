#ifndef LOG_QUEUE_H
#define LOG_QUEUE_H
#include <cassert>
#include <condition_variable>
#include <list>
#include <mutex>
template <typename T>
class LogQueue
{
public:
    explicit LogQueue(size_t MaxRequests = 1000); //声明为explicit的构造函数不能在隐式转换中使用
    ~LogQueue();

    /*清空队列*/
    void Clear();
    /*队列是否空*/
    bool Empty();
    /*队列是否已满*/
    bool Full();
    /*关闭队列*/
    void Close();
    /*请求队列任务数量*/
    size_t Size();
    /*请求队列最大任务数量*/
    size_t Capacity();
    /*获取队头任务*/
    T Front();
    /*获取队尾任务*/
    T Back();
    /*从队尾添加一个任务*/
    void PushBack(const T &task);
    /*从队头添加一个任务*/
    void PushFront(const T &task);
    /*从队头取出一个任务*/
    bool Pop(T &task);
    bool Pop(T &task, int timeout);
    /*唤醒消费者*/
    void Flush();

private:
    std::list<T> m_workqueue; //底层数据结构双向链表
    size_t m_max_requests;    //队列最大任务数
    std::mutex m_mutex;       //互斥量
    bool isClosed;
    std::condition_variable m_condConsumer; //条件变量，消费者
    std::condition_variable m_condProducer; //生产者
};

template <typename T>
LogQueue<T>::LogQueue(size_t MaxRequests) : m_max_requests(MaxRequests) {
    assert(MaxRequests > 0);
    isClosed = false;
}

template <class T>
LogQueue<T>::~LogQueue() {
    Close();
};

template <typename T>
void LogQueue<T>::Clear() {
    std::lock_guard<std::mutex> locker(m_mutex);
    m_workqueue.clear();
}

template <typename T>
bool LogQueue<T>::Empty() {
    std::lock_guard<std::mutex> locker(m_mutex);
    return m_workqueue.empty();
}

template <typename T>
bool LogQueue<T>::Full() {
    std::lock_guard<std::mutex> locker(m_mutex);
    return m_workqueue.size() >= m_max_requests;
}

template <typename T>
void LogQueue<T>::Close() {
    Clear();
    isClosed = true;
    m_condConsumer.notify_all(); //通知所有等待的线程
    m_condProducer.notify_all();
}

template <class T>
size_t LogQueue<T>::Size() {
    std::lock_guard<std::mutex> locker(m_mutex);
    return m_workqueue.size();
}

template <class T>
size_t LogQueue<T>::Capacity() {
    std::lock_guard<std::mutex> locker(m_mutex);
    return m_max_requests;
}

template <class T>
T LogQueue<T>::Front() {
    std::lock_guard<std::mutex> locker(m_mutex);
    return m_workqueue.front();
}

template <class T>
T LogQueue<T>::Back() {
    std::lock_guard<std::mutex> locker(m_mutex);
    return m_workqueue.back();
}

template <class T>
void LogQueue<T>::PushBack(const T &task) {
    /*条件变量需要搭配unique_lock*/
    std::unique_lock<std::mutex> locker(m_mutex);
    while (m_workqueue.size() >= m_max_requests) { //队列已满需要等待
        m_condProducer.wait(locker);               //阻塞当前线程，直到条件变量被唤醒
    }
    m_workqueue.push_back(task);
    m_condConsumer.notify_one(); //通知一个等待的线程
}

template <class T>
void LogQueue<T>::PushFront(const T &task) {
    /*条件变量需要搭配unique_lock*/
    std::unique_lock<std::mutex> locker(m_mutex);
    while (m_workqueue.size() >= m_max_requests) { //队列已满需要等待
        m_condProducer.wait(locker);
    }
    m_workqueue.push_front(task);
    m_condConsumer.notify_one();
}

template <class T>
bool LogQueue<T>::Pop(T &task) {
    /*条件变量需要搭配unique_lock*/
    std::unique_lock<std::mutex> locker(m_mutex);
    while (m_workqueue.empty()) { //队列已空需要等待
        m_condConsumer.wait(locker);
        if (isClosed) {
            return false;
        }
    }
    task = m_workqueue.front();
    m_workqueue.pop_front();
    m_condProducer.notify_one();
    return true;
}

template <class T>
bool LogQueue<T>::Pop(T &task, int timeout) {
    /*条件变量需要搭配unique_lock*/
    std::unique_lock<std::mutex> locker(m_mutex);
    while (m_workqueue.empty()) {
        if (m_condConsumer.wait_for(locker, std::chrono::seconds(timeout)) == std::cv_status::timeout) {
            return false;
        }
        if (isClosed) {
            return false;
        }
    }
    task = m_workqueue.front();
    m_workqueue.pop_front();
    m_condProducer.notify_one();
    return true;
}

template <class T>
void LogQueue<T>::Flush() {
    m_condConsumer.notify_one();
};

#endif // !LOG_QUEUE_H
