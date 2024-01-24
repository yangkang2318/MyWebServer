#ifndef HEAP_TIMER_H
#define HEAP_TIMER_H

#include <chrono>
#include <cstddef>
#include <functional>
#include <unordered_map>
#include <vector>
typedef std::function<void()> TimeoutCallBack;    //回调函数
typedef std::chrono::high_resolution_clock Clock; //返回的时间点是按秒为单位的
typedef std::chrono::milliseconds MS;             //时间间隔毫秒
typedef Clock::time_point TimeStamp;              //获取时间点

struct TimerNode {
    int id;             //标识定时器
    TimeStamp expires;  //设置过期时间
    TimeoutCallBack cb; //回调函数
    bool operator<(const TimerNode &t) {
        return expires < t.expires;
    }
};

class HeapTimer
{
private:
    std::vector<TimerNode> m_heap;         //小根堆是一个完全二叉树，完全二叉树一般采用顺序存储
    std::unordered_map<int, size_t> m_ref; //定时器id到数组索引的映射
    /*向上调整指定位置的结点*/
    void Siftup(size_t i);
    /*向下调整指定位置的结点**/
    bool Siftdown(size_t index, size_t n);
    void SwapNode(size_t i, size_t j);
    /* 删除指定位置的结点 */
    void Del(size_t i);

public:
    HeapTimer() {
        m_heap.reserve(64);
    }
    ~HeapTimer() {
        Clear();
    }
    /*调整指定id的定时器*/
    void Adjust(int id, int timeout);
    /*添加定时器节点*/
    void Add(int id, int timeout, const TimeoutCallBack &cb);
    void Clear();
    /* 删除指定id结点，并触发回调函数 */
    void DoWork(int id);
    /* 清除超时结点，并触发回调函数*/
    void Tick();
    /*删除根节点*/
    void Pop();
    /*返回下一个即将超时的滴答数，单位为毫秒*/
    int GetNextTick();
};
#endif // !HEAP_TIMER_H
