#ifndef RESPOND_HTTP_H
#define RESPOND_HTTP_H
#include "../buffer/buffer.h"
#include "../log/log.h"
#include <fcntl.h>    // open
#include <sys/mman.h> // mmap, munmap
#include <sys/stat.h> // stat
#include <unistd.h>   // close
#include <unordered_map>
class HttpResponse
{
private:
    int m_code;
    bool m_isKeepAlive;
    std::string m_path;
    std::string m_srcDir;
    char *m_file;           //目标文件映射到内存的地址
    struct stat m_fileStat; //目标文件状态                                             // 文件属性
    static const std::unordered_map<std::string, std::string> SUFFIX_TYPE; //请求文件后缀路径映射
    static const std::unordered_map<int, std::string> CODE_STATUS;         //响应状态码
    static const std::unordered_map<int, std::string> CODE_HTML_PATH;      //响应状态码对应的HTML页面路径

    void WriteReponseLine(Buffer &buff);    //写响应行
    void WriteResponseHeader(Buffer &buff); //写响应头
    void WriteReponseContent(Buffer &buff); //写响应消息
    std::string GetFileType();              //判断请求的文件类型
    void GetErrorHtml();                    //请求失败，返回给客户的HTML文件

public:
    HttpResponse();
    ~HttpResponse();
    void Init(const std::string &srcDIR, const std::string &path, bool isKeepAlive = false, int code = -1);
    void UnmapFile(); //关闭目标文件映射到内存，释放内存
    void Respond(Buffer &buff);
    void WriteErrorContent(Buffer &buff, std::string message); //写错误HTML返回给客户端
    char *File();
    size_t FileLen() const;
    int Code() const {
        return m_code;
    }
};

#endif // !RESPOND_HTTP_H
