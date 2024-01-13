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
    /*原子对象的主要特征是，从不同的线程访问这个包含的值不会导致数据竞争
     *（即，这样做是明确定义的行为，访问正确排序）*/
    static std::atomic<int> userCount;

    HttpConn();
    ~HttpConn();
    void Close();
    int GetFd() const;
    sockaddr_in GetAddr() const;
    const char *GetIP() const;
    int GetPort() const;
    /*初始化*/
    void Init(int sockFd, const sockaddr_in &addr);
    /*解析接收的请求报文，并准备响应报文*/
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
