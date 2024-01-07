#include "sql_conn_pool.h"

SqlConnPool::SqlConnPool() {
    m_useCount = 0;
    m_freeCount = 0;
}

SqlConnPool::~SqlConnPool() {
    ClosePool();
}

SqlConnPool *SqlConnPool::Instance() {
    static SqlConnPool connPool;
    return &connPool;
}

void SqlConnPool::Init(const char *host, int port, const char *user, const char *pwd, const char *dbName,
                       int connSize = 10) {
    assert(connSize > 0);
    for (int i = 0; i < connSize; i++) {
        MYSQL *conn = nullptr;
        conn = mysql_init(conn);
        if (!conn) {
            LOG_ERROR("MySql init error!");
            assert(conn);
        }
        conn = mysql_real_connect(conn, host, user, pwd, dbName, port, nullptr, 0);
        if (!conn) {
            LOG_ERROR("MySql connnect error!");
        }
        m_connQue.push(conn);
    }
    m_max_conn = connSize;
    m_sem.InitSem(m_max_conn);
}

MYSQL *SqlConnPool::GetConn() {
    MYSQL *conn = nullptr;
    if (m_connQue.empty()) {
        LOG_ERROR("SqlConnPool busy!");
        return nullptr;
    }
    m_sem.Acquire();
    {
        std::lock_guard<std::mutex> locker(m_mutex);
        conn = m_connQue.front();
        m_connQue.pop();
    }
    return conn;
}

void SqlConnPool::FreeConn(MYSQL *conn) {
    assert(conn);
    std::lock_guard<std::mutex> locker(m_mutex);
    m_connQue.push(conn);
    m_sem.Release();
}

int SqlConnPool::GetFreeConnCount() {
    std::lock_guard<std::mutex> locker(m_mutex);
    return m_connQue.size();
}

void SqlConnPool::ClosePool() {
    std::lock_guard<std::mutex> locker(m_mutex);
    while (!m_connQue.empty()) {
        auto item = m_connQue.front();
        m_connQue.pop();
        mysql_close(item);
    }
    mysql_library_end(); //来终止使用MySQL库
}
