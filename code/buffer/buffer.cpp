#include "buffer.h"
#include <bits/types/struct_iovec.h>
#include <cassert>
#include <sys/uio.h>
#include <unistd.h>

Buffer::Buffer(int initBuffSize) : m_buffer(initBuffSize), m_readIdx(0), m_writeIdx(0) {
}

size_t Buffer::ReadableBytes() const {
    return m_writeIdx - m_readIdx;
}

size_t Buffer::WritableBytes() const {
    return m_buffer.size() - m_writeIdx;
}

size_t Buffer::PrependableBytes() const {
    return m_readIdx;
}

const char *Buffer::Peek() const {
    return BeginPtr() + m_readIdx;
}

void Buffer::EnsureWriteable(size_t len) {
    if (WritableBytes() < len) {
        MakeSpace(len);
    }
    assert(WritableBytes() >= len);
}

void Buffer::HasWritten(size_t len) {
    m_writeIdx += len;
}

void Buffer::Retrieve(size_t len) {
    assert(len <= ReadableBytes());
    m_readIdx += len;
}

void Buffer::RetrieveUntil(const char *end) {
    assert(Peek() <= end);
    Retrieve(end - Peek());
}

void Buffer::RetrieveAll() {
    /*bzero函数用于将一段内存区域清零，即将这段内存区域中的所有字节都设置为0*/
    bzero(&m_buffer[0], m_buffer.size());
    m_readIdx = 0;
    m_writeIdx = 0;
}

std::string Buffer::RetrieveAllToStr() {
    std::string str(Peek(), ReadableBytes());
    RetrieveAll();
    return str;
}

const char *Buffer::BeginWriteConst() const {
    return BeginPtr() + m_writeIdx;
}

char *Buffer::BeginWrite() {
    return BeginPtr() + m_writeIdx;
}

void Buffer::Append(const char *str, size_t len) {
    assert(str);
    EnsureWriteable(len);
    /*std::copy 复制[start,end)范围的数据到另一个指针开始的范围
    copy只负责复制数据，不申请空间，因此要保证有足够的空间进行复制*/
    std::copy(str, str + len, BeginWrite());
    HasWritten(len);
}

void Buffer::Append(const std::string &str) {
    /*string.data()生成一个const char*指针，指向一个临时数组*/
    Append(str.data(), str.length());
}

void Buffer::Append(const void *data, size_t len) {
    assert(data);
    /*static_cast 强制转化
    将任意类型的指针指向的数据转化为char*指针数组*/
    Append(static_cast<const char *>(data), len);
}

void Buffer::Append(const Buffer &buffer) {
    Append(buffer.Peek(), buffer.ReadableBytes());
}

ssize_t Buffer::ReadFd(int fd, int *Errno) {
    char buff[65535];
    /*iovec结构体用于描述一个数据缓冲区。它通常与readv和writev
    系统调用一起使用，用于在一次系统调用中读取或写入多个缓冲区*/
    struct iovec iov[2];
    const size_t writableBytes = WritableBytes();
    /*分散读*/
    iov[0].iov_base = BeginPtr() + m_writeIdx;
    iov[0].iov_len = writableBytes;
    iov[1].iov_base = buff;
    iov[1].iov_len = sizeof(buff);
    /*fd是文件描述符，iov是iovec结构体数组，iovcnt是数组元素个数*/
    const ssize_t len = readv(fd, iov, 2);
    if (len < 0) {
        *Errno = errno;
    } else if (static_cast<size_t>(len) <= writableBytes) {
        m_writeIdx += len;
    } else {
        m_writeIdx = m_buffer.size();
        Append(buff, len - writableBytes);
    }
    return len;
}

ssize_t Buffer::WriteFd(int fd, int *Errno) {
    size_t readSize = ReadableBytes();
    ssize_t len = write(fd, Peek(), readSize);
    if (len < 0) {
        *Errno = errno;
        return len;
    }
    m_readIdx += len;
    return len;
}

char *Buffer::BeginPtr() {
    return &*m_buffer.begin();
}

const char *Buffer::BeginPtr() const {
    return &*m_buffer.begin();
}

void Buffer::MakeSpace(size_t len) {
    if (WritableBytes() + PrependableBytes() < len) {
        /*如果prepend区和writable区的总空间也不足够，则需要为缓冲区
    resize空间，以实现自动增长。Buffer的空间增长之后，不会再回缩*/
        m_buffer.resize(m_writeIdx + len - 1);
    } else {
        /*如果prepend区和writable区的总空间足够写入数据，
        则可先将readable区的数据腾挪到前面，以腾出空间*/
        size_t readSize = ReadableBytes();
        std::copy(BeginPtr() + m_readIdx, BeginPtr() + m_writeIdx, BeginPtr());
        m_readIdx = 0;
        m_writeIdx = m_readIdx + readSize;
        assert(readSize == ReadableBytes());
    }
}
