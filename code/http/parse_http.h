#ifndef PARSE_HTTP_H
#define PARSE_HTTP_H
#include "../buffer/buffer.h"
#include "../log/log.h"
#include "../pool/sql_conn_pool.h"
#include <mysql/mysql.h>
#include <regex>
#include <string>
#include <unordered_map>
#include <unordered_set>

class HttpRequest
{
public:
    enum PARSE_STATE { REQUEST_LINE = 0, HEADER, CONTENT, FINISH };
    HttpRequest() {
        Init();
    };
    ~HttpRequest() = default;
    /*初始化*/
    void Init();
    bool Parse(Buffer &buff);
    std::string GetPath() const;
    std::string &GetPath();
    std::string GetMethod() const;
    std::string GetVersion() const;
    std::string GetPost(const std::string &key) const;
    std::string GetPost(const char *key) const;
    bool IsKeepAlive() const;
    /*
    todo
    void HttpConn::ParseFormData() {}
    void HttpConn::ParseJson() {}
    */

private:
    PARSE_STATE m_state;
    std::string m_method;
    std::string m_path;
    std::string m_version;
    std::string m_content;
    std::unordered_map<std::string, std::string> m_header; //存放请求头部
    std::unordered_map<std::string, std::string> m_post;   //存放POST请求消息键值对
    static const std::unordered_set<std::string> DEFAULT_HTML; //必须在类外定义，因为容器是模板而不是字面值常量类型
    static const std::unordered_map<std::string, int> DEFAULT_HTML_TAG;

    bool ParseRequestLine(const std::string &line);
    void ParseHeader(const std::string &line);
    void ParseContent(const std::string &line);
    void ParsePath();
    /*处理POST请求*/
    void ParsePost();
    /*解析POST请求中的消息体*/
    void ParseFromUrlencoded();
    /*将16进制转10进制*/
    static int ConvertHex(const char &ch);
    /*用户注册或登陆查询*/
    static bool UserVerify(const std::string &name, const std::string &pwd, bool isLogin);
};

#endif // !PARSE_HTTP_H