#include "parse_http.h"

const std::unordered_set<std::string> HttpRequest::DEFAULT_HTML{
    "/index", "/register", "/login", "/welcome", "/video", "/picture",
};

const std::unordered_map<std::string, int> HttpRequest::DEFAULT_HTML_TAG{
    {"/register.html", 0},
    {"/login.html", 1},
};
void HttpRequest::Init() {
    m_method = m_path = m_version = m_content = "";
    m_state = REQUEST_LINE;
    m_header.clear();
    m_post.clear();
}

bool HttpRequest::Parse(Buffer &buff) {
    const char *CRLF = "\r\n";       //回车换行
    if (buff.ReadableBytes() <= 0) { //没有可读数据
        return false;
    }
    while (buff.ReadableBytes() && m_state != FINISH) {
        /*search用于在一个序列中搜索指定的子序列，并返回第一次出现该子序列的位置*/
        const char *lineEnd = std::search(buff.Peek(), buff.BeginWriteConst(), CRLF, CRLF + 2);
        std::string line(buff.Peek(), lineEnd); //获取一行数据，去除了\r\n
        switch (m_state) {
            case REQUEST_LINE:
                if (!ParseRequestLine(line)) { //解析请求行
                    return false;
                }
                ParsePath(); //解析路径
                break;
            case HEADER:
                ParseHeader(line); //解析请求头
                if (buff.ReadableBytes() <= 2) {
                    m_state = FINISH;
                }
                break;
            case CONTENT:
                ParseContent(line); //解析消息
                break;
            default:
                break;
        }
        /*search函数没有找到在读缓冲区找到\r\n时，回返回写指针位置，此时表示请求不完整或读完了*/
        if (lineEnd == buff.BeginWrite()) {
            break;
        }
        buff.RetrieveUntil(lineEnd + 2);
    }
    LOG_DEBUG("[%s], [%s], [%s]", m_method.c_str(), m_path.c_str(), m_version.c_str());
    return true;
}

std::string HttpRequest::GetPath() const {
    return m_path;
}

std::string &HttpRequest::GetPath() {
    return m_path;
}

std::string HttpRequest::GetMethod() const {
    return m_method;
}

std::string HttpRequest::GetVersion() const {
    return m_version;
}

std::string HttpRequest::GetPost(const std::string &key) const {
    assert(key != "");
    if (m_post.count(key) == 1) {
        return m_post.find(key)->second;
    }
    return "";
}

std::string HttpRequest::GetPost(const char *key) const {
    assert(key != nullptr);
    if (m_post.count(key) == 1) {
        return m_post.find(key)->second;
    }
    return "";
}

bool HttpRequest::ParseRequestLine(const std::string &line) {
    std::regex patten("^([^ ]*) ([^ ]*) HTTP/([^ ]*)$");
    std::smatch subMatch; // std::smatch 类型表示一个正则表达式匹配结果的集合
    if (std::regex_match(line, subMatch, patten)) {
        m_method = subMatch[1];
        m_path = subMatch[2];
        m_version = subMatch[3];
        m_state = HEADER;
        return true;
    }
    LOG_ERROR("RequestLine Error");
    return false;
}

void HttpRequest::ParseHeader(const std::string &line) {
    std::regex patten("^([^:]*): ?(.*)$");
    std::smatch subMatch;
    if (std::regex_match(line, subMatch, patten)) {
        m_header[subMatch[1]] = subMatch[2];
    } else {
        m_state = CONTENT;
    }
}

void HttpRequest::ParseContent(const std::string &line) {
    m_content = line;
    ParsePost();
    m_state = FINISH;
    LOG_DEBUG("Body:%s, len:%d", line.c_str(), line.size());
}

void HttpRequest::ParsePath() {
    if (m_path == "/") {
        m_path = "/index.html";
    } else {
        for (const auto &item : DEFAULT_HTML) {
            if (item == m_path) {
                m_path += ".html";
                break;
            }
        }
    }
}

void HttpRequest::ParsePost() {
    /*application/x-www-form-urlencoded表单数据被编码为key1=value1&key2=value2…形式
     *并且对key和value都进行了URL转码, 空格转换为 “+” 加号，特殊符号转换为 ASCII HEX 值*/
    if (m_method == "POST" && m_header["Content-Type"] == "application/x-www-form-urlencoded") {
        ParseFromUrlencoded();
        if (DEFAULT_HTML_TAG.count(m_path)) {
            int tag = DEFAULT_HTML_TAG.find(m_path)->second;
            LOG_DEBUG("Tag:%d", tag);
            if (tag == 0 || tag == 1) {
                bool isLogin = (tag == 1);
                if (UserVerify(m_post["userName"], m_post["passWord"], isLogin)) {
                    m_path = "/welcome.html";
                } else {
                    m_path = "/error.html";
                }
            }
        }
    }
}

void HttpRequest::ParseFromUrlencoded() {
    /*消息体为key1=value1&key2=value2…，同时进行了URL转码，
     *空格转换为 “+” 加号，特殊符号转换为 ASCII HEX 值*/
    if (m_content.size() == 0) {
        return;
    }
    std::string key, value;
    int num = 0;
    int n = m_content.size();
    int i = 0, j = 0;
    for (; i < n; i++) {
        char ch = m_content[i];
        switch (ch) {
            /*key*/
            case '=':
                key = m_content.substr(j, i - j);
                j = i + 1;
                break;
            /*+代表空格*/
            case '+':
                m_content[i] = ' ';
                break;
            /*16进制 ASCII*/
            case '%':
                /*16进制转10进制*/
                num = ConvertHex(m_content[i + 1]) * 16 + ConvertHex(m_content[i + 2]);
                m_content[i + 2] = num % 10 + '0';
                m_content[i + 1] = num / 10 + '0';
                break;
            /*value*/
            case '&':
                value = m_content.substr(j, i - j);
                j = i + 1;
                m_post[key] = value;
                LOG_DEBUG("%s=%s", key.c_str(), value.c_str());
            default:
                break;
        }
    }
    assert(j <= i);
    /*最后一个的key=value*/
    if (m_post.count(key) == 0 && j < i) {
        value = m_content.substr(j, i - j);
        m_post[key] = value;
    }
}

int HttpRequest::ConvertHex(const char &ch) {
    if (ch >= 'A' && ch <= 'F') {
        return ch - 'A' + 10;
    }
    if (ch >= 'a' && ch <= 'f') {
        return ch - 'a' + 10;
    }
    return ch;
}

bool HttpRequest::UserVerify(const std::string &name, const std::string &pwd, bool isLogin) {
    if (name == "" || pwd == "")
        return false;
    LOG_INFO("Verify name:%s pwd:%s", name.c_str(), pwd.c_str());
    MYSQL *sql;
    SqlConnRAII(&sql, SqlConnPool::Instance());
    assert(sql);

    bool flag = false;
    unsigned int j = 0;
    char order[256] = {0};
    MYSQL_FIELD *fields = nullptr;
    MYSQL_RES *res = nullptr; //查询结果
    if (!isLogin) {
        flag = true;
    }
    /* 查询用户及密码 */
    /*用户账号是唯一标识的,如果明知道查询结果只有⼀个,SQL语句中使⽤LIMIT 1会提⾼查询效率*/
    snprintf(order, 256, "SELECT username, password FROM user WHERE username='%s' LIMIT 1", name.c_str());
    LOG_DEBUG("%s", order);
    /*返回0表示查询成功，否则失败*/
    if (mysql_query(sql, order)) {
        /*查询失败*/
        mysql_free_result(res); //一旦完成了对结果集的操作，必须调用mysql_free_result()释放结果集，以防内存泄露
        return false;
    }
    /*对于成功检索了数据的每个查询，必须调用mysql_store_result()或mysql_use_result()*/
    res = mysql_store_result(sql);
    /*获取查询结果集的列数目(即有多少列)*/
    j = mysql_num_fields(res);
    /*返回结果集中代表字段（列）的对象的数组*/
    fields = mysql_fetch_fields(res);
    /*处理执行结果一般放在while循环中，遍历每一行*/
    while (MYSQL_ROW row = mysql_fetch_row(res)) { //返回执行结果的当前行的数值数组，执行这个函数后，结果指向下一行
        LOG_DEBUG("MYSQL ROW: %s %s", row[0], row[1]);
        std::string passWord(row[1]);
        /*登录行为*/
        if (isLogin) {
            if (pwd == passWord) {
                flag = true;
                LOG_DEBUG("pwd correct!");
            } else {
                flag = false;
                LOG_DEBUG("pwd error!");
            }
        } else { /*注册行为*/
            flag = false;
            LOG_DEBUG("user used!");
        }
    }
    mysql_free_result(res);
    /* 注册行为且用户名未被使用*/
    if (!isLogin && flag == true) {
        LOG_DEBUG("regirster!");
        bzero(order, 256);
        snprintf(order, 256, "INSERT INTO user(username, password) VALUES('%s','%s')", name.c_str(), pwd.c_str());
        LOG_DEBUG("%s", order);
        if (mysql_query(sql, order)) {
            LOG_DEBUG("regirster error!");
            flag = false;
        } else {
            LOG_DEBUG("regirster success!");
        }
    }
    SqlConnPool::Instance()->FreeConn(sql); //将sql连接放回连接池
    return flag;
}
