#ifndef LOG_H
#define LOG_H
#include "../buffer/buffer.h"
#include "log_queue.h"
#include <cstdarg>
#include <cstdio>
#include <sys/stat.h> //mkdir
#include <sys/time.h>
#include <thread> //va_start
class Log
{
private:
    Log();
    ~Log();
    /*异步写操作，从请求队列中取出日志并写入日志文件*/
    void AsyncWrite();
    /*提示日志级别*/
    void AppendLogLevelTitle(int level);

    /*data*/
    static const int LOG_PATH_LEN = 256; //日志文件最长路径
    static const int LOG_NAME_LEN = 256; //日志文件最长名字
    static const int MAX_LINES = 50000;  //日志文件内最大日志条数

    const char *m_path;   //存放路径
    const char *m_suffix; //文件后缀
    int m_maxLine;
    int m_lineCount; //当前已有行数
    int m_toDay;     //按当天日期区分文件
    bool m_isOpen;
    Buffer m_buff;                                     //读写缓冲区
    int m_level;                                       //日志等级
    bool m_isAsync;                                    //是否异步日志
    FILE *m_fp;                                        //日志文件指针
    std::unique_ptr<LogQueue<std::string>> m_reqQueue; //请求队列
    std::unique_ptr<std::thread> m_writeThread;        //写线程
    std::mutex m_mutex;                                //同步日志所需的互斥量

public:
    /*共有静态方法实例化，单例模式懒汉启动*/
    static Log *Instance();
    /*异步日志写线程工作函数*/
    static void ThreadWriteLog();
    /*刷新缓冲数据到日志，如果是异步写则唤醒消费者*/
    void Flush();
    /*初始化日志实例*/
    void Init(int level = 1, const char *path = "./log", const char *suffix = ".log", int maxRequests = 1024);
    /*将日志放入请求队列(异步)，或者直接写入日志文件(同步)*/
    void Write(int level, const char *format, ...);
    /*返回日志级别*/
    int GetLevel();
    /*设置日志级别*/
    void SetLevel(int level);
    /*返回日志打开状态*/
    inline bool IsOpen() {
        return m_isOpen;
    }
};

#define LOG_BASE(level, format, ...)                     \
    do {                                                 \
        Log *log = Log::Instance();                      \
        if (log->IsOpen() && log->GetLevel() <= level) { \
            log->Write(level, format, ##__VA_ARGS__);    \
            log->Flush();                                \
        }                                                \
    } while (0);
//四组宏定义，用于不同等级日志的输出
//__VA_ARGS__将宏中 …
//的内容原样抄写在右边__VA_ARGS__所在的位置
//如果可变参数被忽略或为空，##
//操作将使预处理器去除掉它前面的那个逗号
#define LOG_DEBUG(format, ...)             \
    do {                                   \
        LOG_BASE(0, format, ##__VA_ARGS__) \
    } while (0);
#define LOG_INFO(format, ...)              \
    do {                                   \
        LOG_BASE(1, format, ##__VA_ARGS__) \
    } while (0);
#define LOG_WARN(format, ...)              \
    do {                                   \
        LOG_BASE(2, format, ##__VA_ARGS__) \
    } while (0);
#define LOG_ERROR(format, ...)             \
    do {                                   \
        LOG_BASE(3, format, ##__VA_ARGS__) \
    } while (0);

#endif // !LOG_H