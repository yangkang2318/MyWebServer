#ifndef BUFFER_H
#define BUFFER_H

#include <atomic>
#include <cstddef>
#include <cstring>
#include <string>
#include <vector>

class Buffer
{
public:
    Buffer(int initBuffSize = 1024);
    ~Buffer() = default;

    size_t WritableBytes() const; // const 表明函数不会修改任何变量
    size_t ReadableBytes() const;
    size_t PrependableBytes() const;

    /*返回指向读缓冲区的指针*/
    const char *Peek() const;
    /*确保有足够的空间写长度为len的数据*/
    void EnsureWriteable(size_t len);
    /*更新m_writeIdx*/
    void HasWritten(size_t len);
    /*更新m_readIdx*/
    void Retrieve(size_t len);
    /*更新m_readIdx到end位置*/
    void RetrieveUntil(const char *end);
    /*重置m_buffer*/
    void RetrieveAll();
    /*将读缓冲区的字符转化为string，并重置m_buffer*/
    std::string RetrieveAllToStr();

    /*返回指向写缓冲区的指针*/
    const char *BeginWriteConst() const; //常量版本
    char *BeginWrite();                  //非常量

    /*往写缓冲区写入从str指针开始，长度为len的数据*/
    void Append(const char *str, size_t len);
    /*往写缓冲区写入string*/
    void Append(const std::string &str);
    /*将任意类型指针指向的数据写入写缓冲区*/
    void Append(const void *data, size_t len);
    /*将另一个buffer的读缓冲区数据写入该buffer的写缓冲区*/
    void Append(const Buffer &buffer);
    /*从fd读数据到写缓冲区中*/
    ssize_t ReadFd(int fd, int *Errno);
    /*将读缓冲区数据写往fd中*/
    ssize_t WriteFd(int fd, int *Errno);

private:
    /*返回指向m_buffer的首地址的char型指针*/
    char *BeginPtr();
    const char *BeginPtr() const;
    /*扩容*/
    void MakeSpace(size_t len);
    std::vector<char> m_buffer;
    std::atomic<std::size_t> m_readIdx;
    std::atomic<std::size_t> m_writeIdx; // atomic原子操作，互斥保护一个变量，并且效率更高
};

#endif // !BUFFER_H
