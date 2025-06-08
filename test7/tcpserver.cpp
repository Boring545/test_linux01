
#include <string>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <iostream>
#include <sys/socket.h>
#include <string.h>
#include <sys/epoll.h>
#include "../test3/threadpool.h"

static test3::ThreadPool th_pool;

namespace test7
{
#define BUFFER_SIZE 1024
#define EPOLL_SIZE 1024
#define MAX_PORT 100

    void client_routine_1(int clientfd)
    {
        // 一请求一线程,IO阻塞.简单，
        // 但是当连接增多，线程是有限的，每个线程要占用资源，系统资源也是有限的

        while (true)
        {
            char buffer[BUFFER_SIZE] = {0};
            int len = recv(clientfd, buffer, BUFFER_SIZE, 0);
            if (len < 0)
            {
                close(clientfd); // 网络故障
                break;
            }
            else if (len == 0)
            {
                close(clientfd); // 客户端已经断开连接
                break;
            }
            else
            {
                std::cout << "receive " << len << " bytes, { ";
                std::cout.write(buffer, len);
                std::cout << " }" << std::endl;
            }
        }
        return;
    }

    void oneone_server(int listen_sockfd)
    {
        while (true)
        {
            struct sockaddr_in client_addr;
            memset(&client_addr, 0, sizeof(struct sockaddr));

            socklen_t ca_len = sizeof(struct sockaddr_in);

            // 默认是阻塞的
            int clientfd = accept(listen_sockfd, (struct sockaddr *)&client_addr, &ca_len);

            th_pool.submit(client_routine_1, clientfd); // 来一个连接 启一个线程读
        }
    }
    bool is_contain(int fd, const std::vector<int> &listen_fds)
    {
        auto it = std::find(listen_fds.begin(), listen_fds.end(), fd);
        if (it == listen_fds.end())
        {
            return false;
        }
        return true;
    }
    void epoll_server(std::vector<int> &listen_fds)
    {
        int epollfd = epoll_create(1);
        struct epoll_event events[EPOLL_SIZE] = {0};

        // 注册listen进程到epoll
        struct epoll_event ev;
        for (size_t i = 0; i < listen_fds.size(); i++)
        {
            ev.events = EPOLLIN;                                   // 监听sockfd可读
            ev.data.fd = listen_fds[i];                            // 事件触发时，返回这里的数据
            epoll_ctl(epollfd, EPOLL_CTL_ADD, listen_fds[i], &ev); // sockfd让epoll管理
        }

        while (true)
        {
            // 等待事件的发生 包括 listen的fd触发和未来将加入的clientfd的触发,这里等待时间设成5ms无意义，但如果在循环中做一些额外操作，这个就有意义了
            int readys = epoll_wait(epollfd, events, EPOLL_SIZE, 5); // -1 表示阻塞等待，0表示不阻塞,>0 为等待的ms数
            if (readys == 0)
            {
                continue;
            }

            for (int i = 0; i < readys; i++)
            {
                if (true == is_contain(events[i].data.fd, listen_fds))
                {
                    int listen_sockfd = events[i].data.fd;
                    // listen fd触发 ，用于引入新的clientfd
                    struct sockaddr_in client_addr;
                    memset(&client_addr, 0, sizeof(struct sockaddr));

                    socklen_t ca_len = sizeof(struct sockaddr_in);

                    int clientfd = accept(listen_sockfd, (struct sockaddr *)&client_addr, &ca_len);

                    int origin_flags = fcntl(clientfd, F_GETFL, 0);
                    fcntl(clientfd, F_SETFL, origin_flags | O_NONBLOCK); // 设为非阻塞模式

                    struct epoll_event clientev;
                    clientev.events = EPOLLIN | EPOLLET; // 边沿触发。必须一次读完
                    clientev.data.fd = clientfd;

                    epoll_ctl(epollfd, EPOLL_CTL_ADD, clientfd, &clientev); // 新加入的clientfd由epoll管理

                    // 水平触发 有数据即触发 EPOLLIN EPOLLOUT
                    // 边沿触发 数据从无到有才触发 EPOLLIN|EPOLLET EPOLLOUT|EPOLLET高性能，要求一次读完数据
                }
                else
                {
                    // 处理clinet的数据发送
                    int clientfd = events[i].data.fd;

                    char buffer[BUFFER_SIZE] = {0};

                    while (true)
                    {
                        int len = recv(clientfd, buffer, BUFFER_SIZE, 0);
                        if (len < 0)
                        {
                            if (errno == EAGAIN)
                            {
                                // 当前没有数据可以读取
                                break;
                            }
                            else
                            {
                                close(clientfd);                                      // 网络故障
                                epoll_ctl(epollfd, EPOLL_CTL_DEL, clientfd, nullptr); // 从epoll里删除clientfd
                                break;
                            }
                        }
                        else if (len == 0)
                        {
                            close(clientfd);                                      // 客户端已经断开连接
                            epoll_ctl(epollfd, EPOLL_CTL_DEL, clientfd, nullptr); // 从epoll里删除clientfd
                            break;
                        }
                        else
                        {
                            std::cout << "receive " << len << " bytes, clientfd = " << clientfd << " { ";
                            std::cout.write(buffer, len);
                            std::cout << " }" << std::endl;
                        }
                    }
                }
            }
        }
    }
}

// 1. 客户端发送几千次数据后 connection refused，无法加入新的连接
//  ulimit -n 1048576 临时提高当前会话的fd限制,使得server能持有更多的fd
//  sudo vim /etc/security/limits.conf
//  最后添加如下内容为zjq设置fd默认值和上限
//  zjq              soft    nofile          1048576
//  zjq              hard    nofile          1048576

// 2.客户端 can not assign requested address 客户端无法分配一个本地地址进行连接
// 原因是客户端每次建立连接都要消耗一个port 上限为65536个，即一个16bit数字，连接发送太多导致端口耗尽了
// 方法1：服务器可以监听多个端口,客户端向多个端口发送数据，生成更多的套接字

// 3.客户端 connection time out
//  首先怀疑系统fd上限太少了，cat /proc/sys/fs/file-max  输出9223372036854775807，否决
//  然后怀疑内核允许的最大连接数量不足 cat /proc/sys/net/netfilter/nf_conntrack_max 65536
//  !我的ubuntu里没有nf_conntrack，sudo modprobe nf_conntrack安装 ，sudo rmmod nf_conntrack 卸载
//  vim /etc/sysctl.conf 添加
//  net.netfilter.nf_conntrack_max = 1048576
//  net.ipv4.tcp_mem = 252144 524288 786432   
//  net.ipv4.tcp_rmem = 1024 1024 2048
//  net.ipv4.tcp_wmem = 1024 1024 2097152
//  fs.file-max = 1048576
//  sudo sysctl -p  应用参数

//参数解释：net.ipv4.tcp_mem  页大小4KB 
// 第一个 TCP 内存页使用小于此值时不会触发内存压力控制机制
// 第二个 TCP 内存页超过此值，内核开始尝试回收 TCP 内存
// 第三个 TCP 内存页高于此值，内核会强制回收 TCP 套接字  786432*4KB=3GB

// net.ipv4.tcp_rmem  接受缓冲区控制
// net.ipv4.tcp_wmem  发送缓冲区控制
// 三个参数分别代表缓冲区的 最小值 默认值 最大值  单位BYTE
// 创建一个ipv4连接时，两个缓冲区默认的初始占用是2KB，系统最多允许3GB/2KB=1572864个连接

//  4.服务器 too mant open files in system
// 问题是服务器系统fd上限太少了，设置方法同上fs.file-max = 1048576  sudo sysctl -p

// 5.服务端内存跑满后，已经不接受新的连接了，且内存占用有略微减少
// 因为当内存使用达到极限后，服务器可能关闭或拒绝一些连接，释放对应内存，即内存回收
// 而在终止一个和服务端创建了很多连接的客户端后，服务端CPU占用飙升，因为断开连接需要资源
// 
int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        return -1;
    }
    int port = atoi(argv[1]);
    std::vector<int> listen_fds;

    for (size_t i = 0; i < MAX_PORT; i++)
    {
        if (port + i > 65535)
        {
            break;
        }
        int listen_sockfd = socket(AF_INET, SOCK_STREAM, 0);

        struct sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port + i);
        server_addr.sin_addr.s_addr = INADDR_ANY; // 监听本机上的任意地址 0.0.0.0

        if (0 > bind(listen_sockfd, (const struct sockaddr *)&server_addr, sizeof(server_addr)))
        {
            std::cerr << "bind fault" << std::endl;
            return -2;
        } // 绑定监听fd到对应ip

        if (0 > listen(listen_sockfd, 128))
        {
            // 指定最多有N个连接可以处于等待被处理的状态,控制等待连接的队列长度
            // 关键在于 平均处理能力和瞬时到达速率的关系，如果N太小而瞬时连接增多，则新的连接无法加入
            std::cerr << "listen fault" << std::endl;
            return -3;
        }
        std::cout << "tcpserver is listen on port: " << port + i << std::endl;

        listen_fds.push_back(listen_sockfd);
    }
    th_pool.submit([&listen_fds]
                   { test7::epoll_server(listen_fds); });

    // th_pool.submit(test7::oneone_server, listen_sockfd);

    return 0;
}