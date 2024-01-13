#include "http_conn.h"
#include <arpa/inet.h>
#include <sys/uio.h>

/*静态成员变量必须定义，但可以不用初始化*/
const char *HttpConn::srcDir;
std::atomic<int> HttpConn::userCount;
bool HttpConn::isET;

HttpConn::HttpConn() {
    m_fd = -1;
    m_addr = {0};
    m_isClosed = true;
}

HttpConn::~HttpConn() {
    Close();
};

void HttpConn::Init(int sockFd, const sockaddr_in &addr) {
    assert(sockFd > 0);
    userCount++;
    m_addr = addr;
    m_fd = sockFd;
    m_writeBuff.RetrieveAll();
    m_readBuff.RetrieveAll();
    m_isClosed = false;
    LOG_INFO("client[%d](%s:%d) come in, uesrCount now:%d", m_fd, GetIP(), GetPort(), (int)userCount);
}

void HttpConn::Close() {
    m_response.UnmapFile();
    if (m_isClosed == false) {
        m_isClosed = true;
        userCount--;
        close(m_fd);
        LOG_INFO("Client[%d](%s:%d) quit, UserCount now:%d", m_fd, GetIP(), GetPort(), (int)userCount);
    }
}

int HttpConn::GetFd() const {
    return m_fd;
}

sockaddr_in HttpConn::GetAddr() const {
    return m_addr;
}

const char *HttpConn::GetIP() const {
    return inet_ntoa(m_addr.sin_addr); // inet_ntoa将一个in_addr结构体对象转换成一个用"."表示的IP字符串的形式
}

int HttpConn::GetPort() const {
    return m_addr.sin_port;
}

ssize_t HttpConn::Read(int *saveErrno) {
    ssize_t len = -1;
    do {
        len = m_readBuff.ReadFd(m_fd, saveErrno); //从fd中读取数据
        if (len <= 0) {
            break;
        }
    } while (isET);
    return len;
}

ssize_t HttpConn::Write(int *saveErrno) {
    ssize_t len = -1;
    do {
        len = writev(m_fd, m_iov, m_iovCnt);
        if (len <= 0) {
            *saveErrno = errno;
            break;
        }
        if (m_iov[0].iov_len + m_iov[1].iov_len == 0) {
            break;
        } else if (static_cast<size_t>(len) > m_iov[0].iov_len) {
            m_iov[1].iov_base = (uint8_t *)m_iov[1].iov_base + (len - m_iov[0].iov_len);
            m_iov[1].iov_len -= (len - m_iov[0].iov_len);
            if (m_iov[0].iov_len) {
                m_writeBuff.RetrieveAll();
                m_iov[0].iov_len = 0;
            }
        } else {
            m_iov[0].iov_base = (uint8_t *)m_iov[0].iov_base + len;
            m_iov[0].iov_len -= len;
            m_writeBuff.Retrieve(len);
        }
    } while (isET || ToWriteBytes() > 10240);
    return len;
}

bool HttpConn::Process() {
    m_request.Init();
    if (m_readBuff.ReadableBytes() <= 0) {
        return false;
    } else if (m_request.Parse(m_readBuff)) {
        LOG_DEBUG("%s", m_request.GetPath().c_str());
        m_response.Init(srcDir, m_request.GetPath(), m_request.IsKeepAlive(), 200);
    } else {
        m_response.Init(srcDir, m_request.GetPath(), false, 400);
    }
    m_response.Respond(m_writeBuff);
    m_iov[0].iov_base = const_cast<char *>(m_writeBuff.Peek());
    m_iov[0].iov_len = m_writeBuff.ReadableBytes();
    m_iovCnt = 1;
    if (m_response.FileLen() > 0 && m_response.File()) {
        m_iov[1].iov_base = m_response.File();
        m_iov[1].iov_len = m_response.FileLen();
        m_iovCnt = 2;
    }
    LOG_DEBUG("filesize:%d,%d to %d", m_response.FileLen(), m_iovCnt, ToWriteBytes());
    return true;
}