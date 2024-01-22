#ifndef EPOLLER_H
#define EPOLLER_H

#include <assert.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <vector>
class Epoller
{
private:
    int m_epollfd;
    std::vector<struct epoll_event> m_events;

public:
    explicit Epoller(int maxEvent = 1024);
    ~Epoller();
    bool AddFd(int fd, uint32_t events);
    bool ModFd(int fd, uint32_t events);
    bool DelFd(int fd);
    int Wait(int timeout = -1);
    int GetEventFd(size_t i) const;
    uint32_t GetEvents(size_t i) const;
};

#endif // !EPOLLER_H