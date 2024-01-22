#include "server.h"

using namespace std;

Server::Server(int port, int trigMode, int timeoutMS, bool Linger, int sqlPort, const char *sqlUser, const char *sqlPwd,
               const char *dbName, int connPoolNum, int threadNum, bool openLog, int logLevel, int logQueSize)
    : m_port(port), m_openLinger(Linger), m_timeoutMs(timeoutMS), m_isClosed(false), m_timer(new HeapTimer()),
      m_threadPool(new ThreadPool(threadNum)), m_epoller(new Epoller()) {
    /*获取当前工作目录的路径,若传入的 buf 为 NULL，且 size 为 0，则
     *getcwd()内部会按需分配一个缓冲区，并将指向该缓冲区的指针作为函数的返回值
     *调用者使用完之后必须调用 free()来释放这一缓冲区所占内存空间*/
    m_srcDir = getcwd(nullptr, 256);
    assert(m_srcDir);
    strncat(m_srcDir, "/resources/", 16);
    HttpConn::userCount = 0; //初始化静态成员
    HttpConn::srcDir = m_srcDir;
    SqlConnPool::Instance()->Init("localhost", sqlPort, sqlUser, sqlPwd, dbName, connPoolNum); //初始化数据库连接池
    InitEventMode(trigMode);                                                                   //初始化事件
    if (!InitSocket()) {
        m_isClosed = true;
    }
    if (openLog) {
        Log::Instance()->Init(logLevel, "./log", ".log", logQueSize);
        if (m_isClosed) {
            LOG_ERROR("========== Server init error!==========");
        } else {
            LOG_INFO("========== Server init successfully==========");
            LOG_INFO("Port:%d,OpenLinger:%s", m_port, Linger ? "true" : "false");
            LOG_INFO("Listen Mode:%s,OpenConn Mode:%s", (m_listenEvent & EPOLLET ? "ET" : "LT"),
                     (m_connEvent & EPOLLET ? "ER" : "LT"));
            LOG_INFO("LogSys level:%d", logLevel);
            LOG_INFO("srcDir:%s", HttpConn::srcDir);
            LOG_INFO("SqlConnPool num:%d, ThreadPool num:%d", connPoolNum, threadNum);
        }
    }
}

Server::~Server() {
    close(m_listenFd);
    m_isClosed = true;
    free(m_srcDir);
    SqlConnPool::Instance()->ClosePool();
}

void Server::InitEventMode(int trigMode) {
    m_listenEvent = EPOLLRDHUP; // EPOLLRDHUP来判断是否对端已经关闭，这样可以减少一次系统调用
    m_connEvent =
        EPOLLONESHOT |
        EPOLLRDHUP; // EPOLLONESHOT表示一个socket连接在任一时刻都只被一个线程处理，当该线程处理完后，需要通过epoll_ctl重置epolloneshot事件
    switch (trigMode) {
        case 0:
            break;
        case 1:
            m_connEvent |=
                EPOLLET; // ET模式，epoll_wait检测到文件描述符有事件发生，则将其通知给应用程序，应用程序必须立即处理该事件，因为后续的epoll_wait不会再次向此应用程序通知这一事件。
            break;
        case 2:
            m_listenEvent |= EPOLLET;
        case 3:
            m_listenEvent |= EPOLLET;
            m_connEvent |= EPOLLET;
        default:
            m_listenEvent |= EPOLLET;
            m_connEvent |= EPOLLET;
            break;
    }
    HttpConn::isET = (m_connEvent & EPOLLET);
}

bool Server::InitSocket() {
    int ret;
    struct sockaddr_in addr;
    if (m_port > 65535 || m_port < 1024) {
        LOG_ERROR("Port:%d error!", m_port);
        return false;
    }
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(m_port);
    struct linger optLinger = {0}; //设置TCP关闭方式
    if (m_openLinger) {
        /*优雅关闭*/
        optLinger.l_onoff = 1;
        optLinger.l_linger = 1; //内核延迟一段时间
    }
    m_listenFd = socket(PF_INET, SOCK_STREAM, 0);
    if (m_listenFd < 0) {
        LOG_ERROR("Create socket error!");
        return false;
    }
    ret = setsockopt(m_listenFd, SOL_SOCKET, SO_LINGER, &optLinger, sizeof(optLinger));
    if (ret < 0) {
        close(m_listenFd);
        LOG_ERROR("Init linger error!");
        return false;
    }
    /*端口复用允许在一个应用程序可以把 n 个套接字绑在一个端口上而不出错。
     *同时，这 n 个套接字发送信息都正常，没有问题。但是，这些套接字并不
     *是所有都能读取信息，只有最后一个套接字会正常接收数据。*/
    /*端口复用最常用的用途应该是防止服务器重启时之前绑定的端口还未释放
     *或者程序突然退出而系统没有释放端口。这种情况下如果设定了端口复用，
     *则新启动的服务器进程可以直接绑定端口。*/
    int optval = 1;
    ret = setsockopt(m_listenFd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval, sizeof(optval));
    if (ret == -1) {
        LOG_ERROR("Set socket REUSEADDR error!");
        close(m_listenFd);
        return false;
    }
    ret = bind(m_listenFd, (struct sockaddr *)&addr, sizeof(addr));
    if (ret < 0) {
        LOG_ERROR("Bind port:%d error!", m_port);
        close(m_listenFd);
        return false;
    }
    ret = listen(m_listenFd, 6);
    if (ret < 0) {
        LOG_ERROR("Listen port:%d error!", m_port);
        close(m_listenFd);
        return false;
    }
    ret = m_epoller->AddFd(m_listenFd, m_listenEvent | EPOLLIN);
    if (ret == 0) {
        LOG_ERROR("Register event to listenfd error!");
        close(m_listenFd);
        return false;
    }
    SetFdNonBlock(m_listenFd);
    LOG_INFO("Server port:%d", m_port);
    return true;
}

int Server::SetFdNonBlock(int fd) {
    assert(fd > 0);
    int oldOption = fcntl(fd, F_GETFL);
    int newOption = oldOption | O_NONBLOCK;
    return fcntl(fd, F_SETFL, newOption);
}

void Server::ProcessListen() {
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    do {
        int connfd = accept(m_listenFd, (struct sockaddr *)&addr, &len);
        if (connfd <= 0)
            return;
        else if (HttpConn::userCount >= MAX_FD) { //用户超出系统限制
            SendError(connfd, "Server busy!");
            LOG_WARN("Clients is full!");
            return;
        } else {
            AddClient(connfd, addr);
        }
    } while (m_listenEvent & EPOLLET);
}

void Server::ProcessWrite(HttpConn *client) {
    assert(client);
    ResetTime(client);
    m_threadPool->AddTask(std::bind(&Server::Write, this, client));
}

void Server::ProcessRead(HttpConn *client) {
    assert(client);
    ResetTime(client);
    m_threadPool->AddTask(std::bind(&Server::Read, this, client));
}

void Server::SendError(int fd, const char *info) {
    assert(fd > 0);
    int ret = send(fd, info, strlen(info), 0);
    if (ret < 0) {
        LOG_WARN("Send error to client[%d] error", fd);
    }
    close(fd);
}

void Server::ResetTime(HttpConn *client) {
    assert(client);
    if (m_timeoutMs > 0) {
        m_timer->Adjust(client->GetFd(), m_timeoutMs);
    }
}

void Server::AddClient(int fd, sockaddr_in addr) {
    assert(fd > 0);
    m_users[fd].Init(fd, addr); //用户初始化
    if (m_timeoutMs > 0) {
        m_timer->Add(fd, m_timeoutMs, std::bind(&Server::CloseConn, this, &m_users[fd])); //注册定时器
    }
    m_epoller->AddFd(fd, EPOLLIN | m_connEvent); //内核事件表注册用户事件
    SetFdNonBlock(fd);                           //设置非阻塞模式
    LOG_INFO("Client[%d] in!", m_users[fd].GetFd());
}

void Server::CloseConn(HttpConn *client) {
    assert(client);
    LOG_INFO("Client[%d] quit!", client->GetFd());
    m_epoller->DelFd(client->GetFd());
    client->Close();
}

void Server::Write(HttpConn *client) {
    assert(client);
    int ret = -1;
    int writeErrno = 0;
    ret = client->Write(&writeErrno);
    if (client->ToWriteBytes() == 0) {
        if (client->IsKeepAlive()) {
            KeepProcess(client);
            return;
        }
    } else if (ret < 0) {
        if (writeErrno == EAGAIN) {
            m_epoller->ModFd(client->GetFd(), m_connEvent | EPOLLOUT);
            return;
        }
    }
    CloseConn(client);
}

void Server::Read(HttpConn *client) {
    assert(client);
    int ret = -1;
    int readErrno = 0;
    ret = client->Read(&readErrno);
    if (ret <= 0 && readErrno != EAGAIN) {
        CloseConn(client);
        return;
    }
    KeepProcess(client);
}

void Server::KeepProcess(HttpConn *client) {
    if (client->Process()) {
        m_epoller->ModFd(client->GetFd(), m_connEvent | EPOLLOUT); //监听输出
    } else {
        m_epoller->ModFd(client->GetFd(), m_connEvent | EPOLLIN); //监听接收
    }
}

void Server::Start() {
    int timeMS = -1; //超时值为-1会导致epoll_wait（）无限期阻塞
    if (!m_isClosed) {
        LOG_INFO("========== Server start ==========");
    }
    while (!m_isClosed) {
        if (m_timeoutMs > 0) {
            timeMS = m_timer->GetNextTick();
        }
        int eventCnt = m_epoller->Wait(timeMS);
        for (int i = 0; i < eventCnt; i++) {
            int fd = m_epoller->GetEventFd(i);         //获取事件发生的文件描述符
            uint32_t events = m_epoller->GetEvents(i); //获取发生的事件
            if (fd == m_listenFd) {
                /*新的连接请求到来*/
                ProcessListen();
            } else if (events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
                /*有异常事件发生*/
                assert(m_users.count(fd) > 0);
                CloseConn(&m_users[fd]);
            } else if (events & EPOLLIN) {
                /*有新的接收数据事件发生*/
                assert(m_users.count(fd) > 0);
                ProcessRead(&m_users[fd]);
            } else if (events & EPOLLOUT) {
                /*有新的发送数据事件发生*/
                assert(m_users.count(fd) > 0);
                ProcessWrite(&m_users[fd]);
            } else {
                LOG_ERROR("Unexpected event");
            }
        }
    }
}
