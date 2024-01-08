#ifndef SQL_CONN_POOL_H
#define SQL_CONN_POOL_H

#include "../log/log.h"
#include "../utils/semaphore.h"
#include <cassert>
#include <mutex>
#include <mysql/mysql.h>
#include <queue>

class SqlConnPool
{
private:
    int m_max_conn;
    int m_useCount;
    int m_freeCount;
    std::queue<MYSQL *> m_connQue;
    std::mutex m_mutex;
    Semaphore m_sem;
    SqlConnPool();
    ~SqlConnPool();

public:
    static SqlConnPool *Instance();
    void Init(const char *host, int port, const char *user, const char *pwd, const char *dbName, int connSize);
    MYSQL *GetConn();
    /*将不用的连接放回池中*/
    void FreeConn(MYSQL *conn);
    int GetFreeConnCount();
    void ClosePool();
};

/* 资源在对象构造初始化 资源在对象析构时释放*/
class SqlConnRAII
{
public:
    SqlConnRAII(MYSQL **sql, SqlConnPool *connPool) {
        assert(connPool);
        *sql = connPool->GetConn();
        m_sql = *sql;
        m_connPool = connPool;
    }

    ~SqlConnRAII() {
        if (m_sql) {
            m_connPool->FreeConn(m_sql);
        }
    }

private:
    MYSQL *m_sql;
    SqlConnPool *m_connPool;
};

#endif // !SQL_CONN_POOL_H