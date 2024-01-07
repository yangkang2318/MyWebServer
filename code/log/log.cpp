#include "log.h"
#include <mutex>

Log::Log() {
    m_lineCount = 0;
    m_isAsync = false;
    m_writeThread = nullptr;
    m_reqQueue = nullptr;
    m_toDay = 0;
    m_fp = nullptr;
}

Log::~Log() {
}

void Log::AsyncWrite() {
    std::string str = "";
    while (m_reqQueue->Pop(str)) {
        std::lock_guard<std::mutex> locker(m_mutex);
        fputs(str.c_str(), m_fp);
    }
}

void Log::AppendLogLevelTitle(int level) {
    switch (level) {
        case 0:
            m_buff.Append("[debug]: ", 9);
            break;
        case 1:
            m_buff.Append("[info] : ", 9);
            break;
        case 2:
            m_buff.Append("[warn] : ", 9);
            break;
        case 3:
            m_buff.Append("[error]: ", 9);
            break;
        default:
            m_buff.Append("[info] : ", 9);
            break;
    }
}

Log *Log::Instance() {
    static Log log;
    return &log;
}

void Log::ThreadWriteLog() {
    Log::Instance()->AsyncWrite();
}

void Log::Flush() {
    if (m_isAsync) {
        m_reqQueue->Flush(); //唤醒请求队列消费者，可以写日志
    }
    fflush(m_fp); //所有缓冲数据刷新到日志文件
}

void Log::Init(int level, const char *path, const char *suffix, int maxRequests) {
    m_isOpen = true;
    m_level = level;
    if (maxRequests > 0) {
        m_isAsync = true;
        if (!m_reqQueue) {
            /*unique_ptr
             * 不能复制构造和赋值操作，只能移动构造和移动赋值操作*/
            std::unique_ptr<LogQueue<std::string>> newLogQueue(new LogQueue<std::string>);
            m_reqQueue = std::move(newLogQueue);
            std::unique_ptr<std::thread> newThread(new std::thread(ThreadWriteLog));
            m_writeThread = std::move(newThread);
        }
    } else {
        m_isAsync = false;
    }
    m_lineCount = 0;
    time_t timer = time(nullptr);             // time_t表示自1970年1月1日00:00到现在所经过的秒数。若
                                              // timer为NULL，则返回日历时间
    struct tm *localTime = localtime(&timer); //将time_t类型的日历时间转换为tm结构的
                                              // Local time（本地时间）
    struct tm sysTime = *localTime;
    m_path = path;
    m_suffix = suffix;
    char fileName[LOG_NAME_LEN] = {0};
    snprintf(fileName, LOG_NAME_LEN - 1, "%s/%04d_%02d_%02d%s", m_path, sysTime.tm_year + 1900, sysTime.tm_mon + 1,
             sysTime.tm_mday, m_suffix);
    m_toDay = sysTime.tm_mday;

    std::lock_guard<std::mutex> locker(m_mutex);
    m_buff.RetrieveAll();
    if (m_fp) {
        Flush();
        fclose(m_fp);
    }
    m_fp = fopen(fileName, "a");
    if (m_fp == nullptr) {
        mkdir(m_path,
              0777); //每个人都能够读取、写入、和执行
        m_fp = fopen(fileName, "a");
    }
    assert(m_fp != nullptr);
}

void Log::Write(int level, const char *format, ...) {
    struct timeval now = {0, 0}; //秒和微妙
    gettimeofday(&now,
                 nullptr); //可以获取从1970年1月1日0时0分0秒开始到现在的时间总数，以秒和微秒的形式返回
    time_t tSec = now.tv_sec;
    struct tm *localTime = localtime(&tSec);
    struct tm sysTime = *localTime;
    va_list vaList; //可变参数
    if (m_toDay != sysTime.tm_mday || (m_lineCount && (m_lineCount % MAX_LINES == 0))) {
        std::unique_lock<std::mutex> locker(m_mutex);
        locker.unlock();
        char newFile[LOG_NAME_LEN];
        char tail[36] = {0};
        snprintf(tail, 36, "%04d_%02d_%02d", sysTime.tm_year + 1900, sysTime.tm_mon + 1, sysTime.tm_mday);
        if (m_toDay != sysTime.tm_mday) { //按时间划分
            snprintf(newFile, LOG_NAME_LEN - 72, "%s/%s%s", m_path, tail, m_suffix);
            m_toDay = sysTime.tm_mday;
            m_lineCount = 0;
        } else { //按日志条数划分
            snprintf(newFile, LOG_NAME_LEN - 72, "%s/%s-%d%s", m_path, tail, (m_lineCount / MAX_LINES), m_suffix);
        }
        locker.lock();
        Flush();
        fclose(m_fp);
        m_fp = fopen(newFile, "a");
        assert(m_fp != nullptr);
    }

    std::unique_lock<std::mutex> locker(m_mutex);
    m_lineCount++;
    int n = snprintf(m_buff.BeginWrite(), 128, "%d-%02d-%02d %02d:%02d:%02d.%06ld ", sysTime.tm_year + 1900,
                     sysTime.tm_mon + 1, sysTime.tm_mday, sysTime.tm_hour, sysTime.tm_min, sysTime.tm_sec, now.tv_usec);
    m_buff.HasWritten(n);
    AppendLogLevelTitle(level);
    va_start(vaList, format);
    int m = vsnprintf(m_buff.BeginWrite(), m_buff.WritableBytes(), format, vaList);
    va_end(vaList);

    m_buff.HasWritten(m);
    m_buff.Append("\n\0", 2);
    if (m_isAsync && m_reqQueue && !m_reqQueue->Full()) {
        m_reqQueue->PushBack(m_buff.RetrieveAllToStr());
    } else {
        fputs(m_buff.Peek(), m_fp);
    }
    m_buff.RetrieveAll();
}

int Log::GetLevel() {
    std::lock_guard<std::mutex> locker(m_mutex);
    return m_level;
}

void Log::SetLevel(int level) {
    std::lock_guard<std::mutex> locker(m_mutex);
    m_level = level;
}