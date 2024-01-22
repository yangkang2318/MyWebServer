#include "epoller.h"

Epoller::Epoller(int maxEvent) : m_epollfd(epoll_create(512)), m_events(maxEvent) {
    assert(m_epollfd >= 0 && m_events.size() > 0);
}

Epoller::~Epoller() {
    close(m_epollfd);
}

bool Epoller::AddFd(int fd, uint32_t events) {
    if (fd < 0)
        return false;
    epoll_event event;
    event.data.fd = fd;
    event.events = events;
    return epoll_ctl(m_epollfd, EPOLL_CTL_ADD, fd, &event) == 0;
}

bool Epoller::ModFd(int fd, uint32_t events) {
    if (fd < 0)
        return false;
    epoll_event event;
    event.data.fd = fd;
    event.events = events;
    return epoll_ctl(m_epollfd, EPOLL_CTL_MOD, fd, &event) == 0;
}

bool Epoller::DelFd(int fd) {
    if (fd < 0)
        return false;
    epoll_event event = {0};
    return epoll_ctl(m_epollfd, EPOLL_CTL_DEL, fd, &event) == 0;
}

int Epoller::Wait(int timeout) {
    return epoll_wait(m_epollfd, &m_events[0], static_cast<int>(m_events.size()), timeout);
}

int Epoller::GetEventFd(size_t i) const {
    assert(i < m_events.size() && i >= 0);
    return m_events[i].data.fd;
}

uint32_t Epoller::GetEvents(size_t i) const {
    assert(i < m_events.size() && i >= 0);
    return m_events[i].events;
}