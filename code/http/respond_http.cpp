#include "respond_http.h"

using namespace std;

const unordered_map<string, string> HttpResponse::SUFFIX_TYPE = {
    {".html", "text/html"},          {".xml", "text/xml"},          {".xhtml", "application/xhtml+xml"},
    {".txt", "text/plain"},          {".rtf", "application/rtf"},   {".pdf", "application/pdf"},
    {".word", "application/nsword"}, {".png", "image/png"},         {".gif", "image/gif"},
    {".jpg", "image/jpeg"},          {".jpeg", "image/jpeg"},       {".au", "audio/basic"},
    {".mpeg", "video/mpeg"},         {".mpg", "video/mpeg"},        {".avi", "video/x-msvideo"},
    {".gz", "application/x-gzip"},   {".tar", "application/x-tar"}, {".css", "text/css "},
    {".js", "text/javascript "},
};

const unordered_map<int, string> HttpResponse::CODE_STATUS = {
    {200, "OK"},
    {400, "Bad Request"},
    {403, "Forbidden"},
    {404, "Not Found"},
};

const unordered_map<int, string> HttpResponse::CODE_HTML_PATH = {
    {400, "/400.html"},
    {403, "/403.html"},
    {404, "/404.html"},
};

void HttpResponse::WriteReponseLine(Buffer &buff) {
    string status;
    if (CODE_STATUS.count(m_code) == 1) {
        status = CODE_STATUS.find(m_code)->second;
    } else {
        m_code = 400;
        status = CODE_STATUS.find(m_code)->second;
    }
    buff.Append("HTTP/1.1 " + to_string(m_code) + " " + status + "\r\n");
}

void HttpResponse::WriteResponseHeader(Buffer &buff) {
    buff.Append("Connection: ");
    if (m_isKeepAlive) {
        buff.Append("keep-alive\r\n");
        buff.Append("keep-alive: max=6, timeout=120\r\n");
    } else {
        buff.Append("close\r\n");
    }
    buff.Append("Content-type: " + GetFileType() + "\r\n");
}

void HttpResponse::WriteReponseContent(Buffer &buff) {
    int srcFd = open((m_srcDir + m_path).data(), O_RDONLY);
    if (srcFd < 0) {
        WriteErrorContent(buff, "File NotFound!");
        return;
    }
    LOG_DEBUG("file path %s", (m_srcDir + m_path).data());
    /* 将文件映射到内存提高文件的访问速度
     *MAP_PRIVATE 建立一个写入时拷贝的私有映射
     *仅仅只是对文件副本进行读写*/
    int *mmRet = (int *)mmap(0, m_fileStat.st_size, PROT_READ, MAP_PRIVATE, srcFd, 0);
    if (*mmRet == -1) {
        WriteErrorContent(buff, "File NotFound!");
        return;
    }
    m_file = (char *)mmRet;
    close(srcFd);
    buff.Append("Content-length: " + to_string(m_fileStat.st_size) + "\r\n\r\n");
}

string HttpResponse::GetFileType() {
    string::size_type idx = m_path.find_last_of('.');
    if (idx == string::npos) {
        return "text/plain";
    }
    string suffix = m_path.substr(idx);
    if (SUFFIX_TYPE.count(suffix) == 1) {
        return SUFFIX_TYPE.find(suffix)->second;
    }
    return "text/plain";
}

void HttpResponse::GetErrorHtml() {
    if (CODE_HTML_PATH.count(m_code) == 1) {
        m_path = CODE_HTML_PATH.find(m_code)->second;
        stat((m_srcDir + m_path).data(), &m_fileStat);
    }
}

HttpResponse::HttpResponse() {
    m_code = -1;
    m_path = m_srcDir = "";
    m_isKeepAlive = false;
    m_file = nullptr;
    m_fileStat = {0};
}

HttpResponse::~HttpResponse() {
    UnmapFile();
}

void HttpResponse::Init(const std::string &srcDIR, const std::string &path, bool isKeepAlive, int code) {
    assert(srcDIR != "");
    if (m_file) {
        UnmapFile();
    };
    m_code = code;
    m_isKeepAlive = isKeepAlive;
    m_path = path;
    m_srcDir = srcDIR;
    m_fileStat = {0};
}

void HttpResponse::UnmapFile() {
    if (m_file) {
        munmap(m_file, m_fileStat.st_size);
        m_file = nullptr;
    }
}

void HttpResponse::Respond(Buffer &buff) {
    if (stat((m_srcDir + m_path).data(), &m_fileStat) < 0 || S_ISDIR(m_fileStat.st_mode)) {
        m_code = 404; //没有该资源

    } else if (!(m_fileStat.st_mode & S_IROTH)) {
        m_code = 403; //没有权限
    } else if (m_code == -1) {
        m_code = 200; //请求成功
    }
    GetErrorHtml();
    WriteReponseLine(buff);
    WriteResponseHeader(buff);
    WriteReponseContent(buff);
}

void HttpResponse::WriteErrorContent(Buffer &buff, string message) {
    string body;
    string status;
    body += "<html><title>Error</title>";
    body += "<body bgcolor=\"ffffff\">";
    if (CODE_STATUS.count(m_code) == 1) {
        status = CODE_STATUS.find(m_code)->second;
    } else {
        status = "Bad Request";
    }
    body += to_string(m_code) + " : " + status + "\n";
    body += "<p>" + message + "</p>";
    body += "<hr><em>MyWebServer</em></body></html>";

    buff.Append("Content-length: " + to_string(body.size()) + "\r\n\r\n");
    buff.Append(body);
}

char *HttpResponse::File() {
    return m_file;
}

size_t HttpResponse::FileLen() const {
    return m_fileStat.st_size;
}
