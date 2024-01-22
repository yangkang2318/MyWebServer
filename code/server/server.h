#ifndef SERVER_H
#define SERVER_H
#include <unistd.h>

#include "../http/http_conn.h"
#include "../log/log.h"
#include "../pool/sql_conn_pool.h"
#include "../pool/thread_pool.h"
#include "../timer/heap_timer.h"
#include "epoller.h"
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <unordered_map>
class Server
{
private:
    int m_port;
    bool m_openLinger;
    int m_timeoutMs; //毫秒MS,定时器的默认过期时间
    bool m_isClosed;
    int m_listenFd;
    char *m_srcDir;

    uint32_t m_listenEvent;
    uint32_t m_connEvent;
    std::unique_ptr<HeapTimer> m_timer;
    std::unique_ptr<ThreadPool> m_threadPool;
    std::unique_ptr<Epoller> m_epoller;
    std::unordered_map<int, HttpConn> m_users; //用户fd到HttpConn实例的映射
    static const int MAX_FD = 65536;

    /*初始化事件*/
    void InitEventMode(int trigMode);
    /*初始化监听socket*/
    bool InitSocket();
    /*设置为非阻塞IO */
    static int SetFdNonBlock(int fd);
    /*处理新的用户请求*/
    void ProcessListen();
    /*将用户的写任务放入线程池的工作队列*/
    void ProcessWrite(HttpConn *client);
    /*将用户的读任务放入线程池的工作队列*/
    void ProcessRead(HttpConn *client);
    /*发送错误提示*/
    void SendError(int fd, const char *info);
    /*重置某个用户的超时时间*/
    void ResetTime(HttpConn *client);
    /*添加新用户*/
    void AddClient(int fd, sockaddr_in addr);
    /*关闭用户连接*/
    void CloseConn(HttpConn *client);
    /*用户写操作*/
    void Write(HttpConn *client);
    /*用户读操作*/
    void Read(HttpConn *client);
    /**/
    void KeepProcess(HttpConn *client);

public:
    Server(int port, int trigMode, int timeoutMS, bool Linger, int sqlPort, const char *sqlUser, const char *sqlPwd,
           const char *dbName, int connPoolNum, int threadNum, bool openLog, int logLevel, int logQueSize);
    ~Server();
    void Start();
};

#endif // !SERVER_H