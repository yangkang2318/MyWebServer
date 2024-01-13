#ifndef HTTP_CONN_H
#define HTTP_CONN_H

#include "parse_http.h"
#include "respond_http.h"
#include <bits/types/struct_iovec.h>
#include <netinet/in.h>
class HttpConn
{
private:
    int m_fd;
    struct sockaddr_in m_addr;
    bool m_isClosed;
    int m_iovCnt;
    struct iovec m_iov[2];
    Buffer m_readBuff;
    Buffer m_writeBuff;
    HttpResponse m_response;
    HttpRequest m_request;

public:
    static bool isET;
    static const char *srcDir;
    static std::atomic<int> userCount;

    HttpConn();
    ~HttpConn();
    void Close();
    int GetFd() const;
    sockaddr_in GetAddr() const;
    const char *GetIP() const;
    int GetPort() const;
    void Init(int sockFd, const sockaddr_in &addr);
    bool Process();
    /*从m_fd中接收数据*/
    ssize_t Read(int *saveErrno);
    /*往m_fd中发送数据*/
    ssize_t Write(int *saveErrno);
    int ToWriteBytes() {
        return m_iov[0].iov_len + m_iov[1].iov_len;
    }
};

#endif // !HTTP_CONN_H
