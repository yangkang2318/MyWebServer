#include "heap_timer.h"
#include <cassert>

void HeapTimer::Siftup(size_t i) {
    assert(i >= 0 && i < m_heap.size());
    size_t j = (i - 1) / 2; //父节点
    while (j >= 0) {
        if (m_heap[j] < m_heap[i]) {
            SwapNode(i, j);
        }
        i = j;
        j = (i - 1) / 2;
    }
}

bool HeapTimer::Siftdown(size_t index, size_t n) {
    assert(index >= 0 && index < m_heap.size());
    assert(n >= 0 && n <= m_heap.size());
    size_t i = index;
    size_t j = index * 2 + 1; //左孩子
    while (j < n) {
        if (j + 1 < n && m_heap[j + 1] < m_heap[j])
            j++;
        if (m_heap[i] < m_heap[j])
            break; //以该节点为根的堆仍是小根堆，不用调整
        SwapNode(i, j);
        i = j;         //更新父节点
        j = i * 2 + 1; // 更新左孩子
    }
    return i > index; //返回true表示向下调整成功
}

void HeapTimer::SwapNode(size_t i, size_t j) {
    assert(i >= 0 && i < m_heap.size());
    assert(j >= 0 && j < m_heap.size());
    std::swap(m_heap[i], m_heap[j]);
    m_ref[m_heap[i].id] = j;
    m_ref[m_heap[j].id] = i;
}

void HeapTimer::Del(size_t i) {
    assert(!m_heap.empty() && i >= 0 && i < m_heap.size());
    size_t n = m_heap.size() - 1;
    assert(i <= n);
    //将要删除的结点换到队尾，然后调整堆
    if (i < n) {
        SwapNode(i, n);
        if (!Siftdown(i, n)) {
            Siftup(i);
        }
    }
    //删除队尾元素
    m_ref.erase(m_heap.back().id);
    m_heap.pop_back();
}

void HeapTimer::Adjust(int id, int timeout) {
    assert(!m_heap.empty() && m_ref.count(id) > 0);
    m_heap[m_ref[id]].expires = Clock::now() + MS(timeout);
    Siftdown(m_ref[id], m_heap.size());
}

void HeapTimer::Add(int id, int timeout, const TimeoutCallBack &cb) {
    assert(id >= 0);
    size_t i;
    if (m_ref.count(id) == 0) {
        /* 新节点：堆尾插入，从下到上调整堆 */
        i = m_heap.size();
        m_ref[id] = i;
        m_heap.push_back({id, Clock::now() + MS(timeout), cb});
        Siftup(i);
    } else {
        /* 已有节点,重新设置时间和回调函数，调整堆 */
        i = m_ref[id];
        m_heap[i].expires = Clock::now() + MS(timeout);
        m_heap[i].cb = cb;
        if (!Siftdown(i, m_heap.size())) {
            /*没有向下调整成功，表示以该节点为根的小根堆仍是一个小根堆
             *但有可能该节点的值小于整个堆的根节点，因此需要向上调整*/
            Siftup(i);
        }
    }
}

void HeapTimer::Clear() {
    m_ref.clear();
    m_heap.clear();
}

void HeapTimer::DoWork(int id) {
    if (m_heap.empty() || m_ref.count(id) == 0) {
        return;
    }
    size_t i = m_ref[id];
    TimerNode node = m_heap[i];
    node.cb();
    Del(i);
}

void HeapTimer::Tick() {
    if (m_heap.empty()) {
        return;
    }
    while (!m_heap.empty()) {
        TimerNode node = m_heap.front();
        /*表示定时器到现在过了多少毫秒
         *>0，表示还未超时
         *<=0，表示超时*/
        if (std::chrono::duration_cast<MS>(node.expires - Clock::now()).count() > 0) {
            break;
        }
        node.cb();
        Pop();
    }
}

void HeapTimer::Pop() {
    assert(!m_heap.empty());
    Del(0);
}

int HeapTimer::GetNextTick() {
    Tick();
    size_t res = -1;
    if (!m_heap.empty()) {
        /*表示定时器到现在过了多少毫秒
         *>0，表示还未超时
         *<=0，表示超时*/
        res = std::chrono::duration_cast<MS>(m_heap.front().expires - Clock::now()).count();
        if (res < 0) {
            res = 0;
        }
    }
    return res;
}