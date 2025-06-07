
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
#define EPOLL_SIZE 1024
namespace test7
{
#define BUFFER_SIZE 1024

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
    void epoll_server(int listen_sockfd)
    {
        int epollfd = epoll_create(1);
        struct epoll_event events[EPOLL_SIZE] = {0};

        struct epoll_event ev;
        ev.events = EPOLLIN;        // 监听sockfd可读
        ev.data.fd = listen_sockfd; // 事件触发时，返回这里的数据

        epoll_ctl(epollfd, EPOLL_CTL_ADD, listen_sockfd, &ev); // sockfd让epoll管理

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
                if (events[i].data.fd == listen_sockfd)
                {
                    // listen fd触发 ，用于引入新的clientfd
                    struct sockaddr_in client_addr;
                    memset(&client_addr, 0, sizeof(struct sockaddr));

                    socklen_t ca_len = sizeof(struct sockaddr_in);
                    // 默认是阻塞的
                    int clientfd = accept(listen_sockfd, (struct sockaddr *)&client_addr, &ca_len);

                    int origin_flags = fcntl(clientfd, F_GETFL, 0);
                    fcntl(clientfd, F_SETFL, origin_flags | O_NONBLOCK); // 设为非阻塞模式

                    ev.events = EPOLLIN | EPOLLET; // 边沿触发。必须一次读完
                    ev.data.fd = clientfd;

                    epoll_ctl(epollfd, EPOLL_CTL_ADD, clientfd, &ev); // 新加入的clientfd由epoll管理

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
                            std::cout << "receive " << len << " bytes, { ";
                            std::cout.write(buffer, len);
                            std::cout << " }" << std::endl;
                        }
                    }
                }
            }
        }
    }
}
int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        return -1;
    }
    int port = atoi(argv[1]);

    int listen_sockfd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = INADDR_ANY; // 监听本机上的任意地址 0.0.0.0

    if (0 > bind(listen_sockfd, (const struct sockaddr *)&server_addr, sizeof(server_addr)))
    {
        std::cerr << "bind fault" << std::endl;
        return -2;
    } // 绑定

    if (0 > listen(listen_sockfd, 5))
    {
        // 指定最多有 5 个连接可以处于等待被处理的状态,控制等待连接的队列长度
        std::cerr << "listen fault" << std::endl;
        return -3;
    }

    // th_pool.submit(test7::oneone_server, listen_sockfd);
    th_pool.submit(test7::epoll_server, listen_sockfd);

    return 0;
}