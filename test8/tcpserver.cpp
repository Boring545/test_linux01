#include <sys/socket.h>
#include <netinet/in.h>
#include <iostream>
#include <error.h>
#include <unistd.h>
#include <poll.h>
#include <sys/epoll.h>
#include "../test3/threadpool.h"

static test3::ThreadPool th_pool;
namespace test8
{
    int client_task(int clientfd)
    {
        while (true)
        {
            char buffer[1024] = {0};
            int count = recv(clientfd, buffer, 1024, 0);
            if (count == 0)
            {
                printf("client disconnect\n");
                close(clientfd);
                break;
            }
            else if (count < 0)
            {
                perror("client_task recv error");
                close(clientfd);
                break;
            }
            printf("recv: %s\n", buffer);
        }
        return 0;
    }
    void oneoneio(int sockfd)
    {
        while (true)
        {
            sockaddr_in clientaddr;
            socklen_t len = sizeof(clientaddr);

            int clientfd = accept(sockfd, (sockaddr *)&clientaddr, &len);

            th_pool.submit([clientfd]
                           { test8::client_task(clientfd); });
        }
    }
    void selectio(int sockfd)
    {
        fd_set rfds, rfds_t;
        FD_ZERO(&rfds);
        FD_SET(sockfd, &rfds);
        int maxfd = sockfd;

        while (true)
        {
            rfds_t = rfds;
            int readynum = select(maxfd + 1, &rfds_t, nullptr, nullptr, nullptr);
            if (readynum == -1)
            {
                perror("select error");
            }
            else
            {
                if (FD_ISSET(sockfd, &rfds_t))
                {
                    int clientfd = accept(sockfd, nullptr, nullptr);
                    FD_SET(clientfd, &rfds);
                    if (maxfd < clientfd)
                    {
                        maxfd = clientfd;
                    }
                }

                // recv
                char buffer[1024] = {0};
                for (int i = sockfd + 1; i <= maxfd; i++)
                {
                    if (FD_ISSET(i, &rfds_t))
                    {
                        int count = recv(i, buffer, 1024, 0);
                        if (count == 0)
                        {
                            printf("client disconnect\n");
                            FD_CLR(i, &rfds);
                            close(i);
                            continue;
                        }
                        else if (count < 0)
                        {
                            perror("client_task recv error");
                            FD_CLR(i, &rfds);
                            close(i);
                            continue;
                        }
                        printf("recv: %s\n", buffer);
                    }
                }
            }
        }
    }

    void pollio(int sockfd)
    {
        pollfd pfds[1024] = {0};
        pfds[0].fd = sockfd;
        pfds[0].events = POLLIN;
        int nfds = 1;

        while (true)
        {
            int readynum = poll(pfds, nfds, -1);
            if (readynum == -1)
            {
                perror("poll error");
                continue;
            }

            if (pfds[0].revents & POLLIN)
            {
                sockaddr_in clientaddr;
                socklen_t len = sizeof(clientaddr);

                int clientfd = accept(sockfd, (sockaddr *)&clientaddr, &len);
                if (clientfd < 0)
                {
                    perror("accept error");
                    continue;
                }

                pfds[nfds].fd = clientfd;
                pfds[nfds].events = POLLIN;
                nfds++;
            }
            char buffer[1024] = {0};
            for (int i = 1; i < nfds; i++)
            {
                if (pfds[i].revents & POLLIN)
                {
                    int count = recv(pfds[i].fd, buffer, 1024, 0);
                    if (count == 0)
                    {
                        printf("client disconnect\n");
                        // pfds[i].fd = -1;
                        // pfds[i].events = 0;
                        pfds[i] = pfds[nfds - 1]; // 用末尾项覆盖当前项
                        nfds--;
                        close(pfds[i].fd);
                        continue;
                    }
                    else if (count < 0)
                    {
                        perror("client_task recv error");
                        // pfds[i].fd = -1;
                        // pfds[i].events = 0;
                        pfds[i] = pfds[nfds - 1]; // 用末尾项覆盖当前项
                        nfds--;
                        close(pfds[i].fd);
                        continue;
                    }
                    printf("recv: %s\n", buffer);
                }
            }
        }
    }

    void epollio(int sockfd)
    {
        int epfd = epoll_create(1); // 参数无意义，不是0即可
        epoll_event ev;
        ev.events = EPOLLIN;
        ev.data.fd = sockfd;
        epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &ev);

        while (true)
        {
            epoll_event events[1024] = {0};
            int readynum = epoll_wait(epfd, events, 1024, -1);
            
            for (int i = 0; i < readynum; i++)
            {
                int connfd = events[i].data.fd;
                if (connfd == sockfd)
                {
                    sockaddr_in clientaddr;
                    socklen_t len = sizeof(clientaddr);

                    int clientfd = accept(sockfd, (sockaddr *)&clientaddr, &len);
                    if (clientfd < 0)
                    {
                        perror("accept error");
                        continue;
                    }
                    ev.events = EPOLLIN;
                    ev.data.fd = clientfd;
                    epoll_ctl(epfd, EPOLL_CTL_ADD, clientfd, &ev);
                }
                else if (events[i].events & EPOLLIN)
                {
                    char buffer[1024] = {0};
                    int count = recv(connfd, buffer, 1024, 0);
                    if (count == 0)
                    {
                        printf("client disconnect\n");
                        epoll_ctl(epfd, EPOLL_CTL_DEL, connfd, nullptr);//删除时ev填null即可
                        close(connfd);
                        continue;
                    }
                    else if (count < 0)
                    {
                        perror("client_task recv error");
                        epoll_ctl(epfd, EPOLL_CTL_DEL, connfd, nullptr);//删除时ev填null即可
                        close(connfd);
                        continue;
                    }
                    printf("recv: %s\n", buffer);
                }
            }
        }
    }

}
int main(int argc, char *argv[])
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in servaddr;
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(2003);

    if (0 != bind(sockfd, (sockaddr *)&servaddr, sizeof(servaddr)))
    {
        perror("bind error");
        return -1;
    }

    if (0 != listen(sockfd, 10))
    {
        perror("listen error");
        return -2;
    }
    std::cout << "listen finished\n";

    // test8::oneoneio(sockfd);
    // test8::selectio(sockfd);
    //test8::pollio(sockfd);
    test8::epollio(sockfd);
    return 0;
}
