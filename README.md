## MyWebServer
基于C++11实现的高性能WEB服务器，经过webbenchh压力测试可以实现18000+的QPS
## 功能
* 基于epoll多路复用，C++11多线程实现Reactor高并发web服务器
* 基于std::vector封装的应用层缓冲区(Buffer)，实现缓冲区自增长
* 基于单例模式，日志队列实现的异步日志系统，记录服务器状态
* 基于生产者/消费者实现的线程池，提高服务器性能，减少线程创建和销毁的开销
* 基于RAII(Resource Acquisition Is Initialization)模式实现连接池，确保数据库连接关闭时释放系统资源，并放回连接池中
* 基于正则表达式和有限状态机解析HTTP请求报文
* 基于存储映射 I/O，提高服务器对用于请求文件的访问效率
* 基于集中写将写缓冲和用户请求文件内容一起发送给用户，减少系统调用
* 基于小根堆实现时间堆定时器，定时剔除掉超时的空闲用户，避免他们耗费服务器资源
## 运行环境
* VMware 16.2.2&ProUbuntu 22.04.1 LTS
* 虚拟机内存16G，CPU内核总数16，型号：12th Gen Intel(R) Core(TM) i7-12700K
## 目录结构
```
|———bin             可执行文件
|   └──server
|———code            源代码
|   |——buffer
|   |——http
|   |——log
|   |——pool
|   |——server
|   |——timer
|   |——utils     
|   └──main.cpp
|———test            线程池和日志测试
|   └──test.cpp
|   └──test         可执行文件
|———webbench-1.5    压力测试
|   └──Makefile
|   └──socket.c         
|   └──webbench.c   
|———Makefile        
|———README.MD
```
## 项目运行
1. 在项目根目录下，运行`make`命令编译构建可执行程序
2. 终端运行`./bin/server <port> <threadNum> <connPoolNum>`，参数分别为端口，线程数，连接池数，例如`./bin/server 1316 16 16`
## 压力测试
1. `cd ./webbench-1.5`
2. `make`编译
3. `webbench -c 10000 -t 10 http://ip:port/`，-c用户数，-t测试时间，ip为主机ip地址，port为服务器占用端口号
## 项目解析
以下是我对该项目进行学习过程中，对项目的解析和学习记录[MyWebServer](https://yangkang22.notion.site/C-49f366d0e502480f9538b017ac2a0795)
## 感谢
本项目是基于[@markparticle](https://github.com/markparticle/WebServer)的WebServer项目进行学习，同时参考了[@JehanRio](https://blog.csdn.net/weixin_51322383/article/details/130464403)的博客进行学习
