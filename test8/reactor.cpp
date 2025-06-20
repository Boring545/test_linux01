#include "webserver.h"

namespace test8
{
    constexpr int LISTEN_EV = EPOLLIN;
    constexpr int RECV_EV = EPOLLIN;
    constexpr int SEND_EV = EPOLLOUT;

    int init_server(uint16_t port)
    {
        int sockfd = socket(AF_INET, SOCK_STREAM, 0);

        sockaddr_in servaddr;
        servaddr.sin_family = AF_INET;
        servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
        servaddr.sin_port = htons(port);

        if (0 != bind(sockfd, (sockaddr *)&servaddr, sizeof(servaddr)))
        {
            perror("bind error");
            return -1;
        }

        if (0 != listen(sockfd, 100))
        {
            perror("listen error");
            return -2;
        }
        std::cout << "listen finished\n";
        return sockfd;
    }
    int set_event(int fd, int op, int flag = 1)
    {
        epoll_event ev;
        ev.data.fd = fd;
        if (flag == 1)
        {
            ev.events = op;
            int ret = epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);
            return ret;
        }
        else if (flag == 2)
        {
            ev.events = op;
            int ret = epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev);
            return ret;
        }
        else
        {
            // 无
            return -2;
        }
    }
    int read_callback(int fd)
    {

        int count = recv(fd, conns[fd].rbuffer.data(), BUFFER_LENGTH, 0);

        if (count == 0)
        {
            printf("client disconnect  %d\n", fd);
            epoll_ctl(epfd, EPOLL_CTL_DEL, fd, nullptr); // 删除时ev填null即可
            close(fd);
            return -1;
        }
        else if (count < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                // 如果当前没有数据可读（或无法写入）
                return 0;
            }
            else
            {
                perror("client_task recv error");
                epoll_ctl(epfd, EPOLL_CTL_DEL, fd, nullptr); // 删除时ev填null即可
                close(fd);
                return -2;
            }
        }
        conns[fd].rbuffer[count] = '\0';
        conns[fd].rlength = count;
        // printf("recv from fd %d: %s\n", fd, conns[fd].rbuffer);
#if 0
        // 回写数据
        conns[fd].wlength = snprintf(conns[fd].wbuffer, BUFFER_LENGTH, "GET %d bytes\n", count);
        set_event(fd, EPOLLOUT, 2);
#else
        http_request(conns[fd]);
        set_event(fd, EPOLLOUT, 2);
#endif
        return 0;
    }
    int write_callback(int fd)
    {
        http_response(conns[fd]);
        if (conns[fd].w_file_fd == -1)
        {
            int sended = send(fd, conns[fd].wbuffer.data(), conns[fd].wlength, 0);
            if (sended == 0)
            {
                // 对端关闭了连接
                printf("client disconnect  %d\n", fd);
                epoll_ctl(epfd, EPOLL_CTL_DEL, fd, nullptr);
                close(fd);
                return -1;
            }
            else if (sended < 0)
            {
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                {
                    // EAGAIN 和 EWOULDBLOCK 是同一个错误的两种别名，表示资源暂时不可用；
                    return 0;
                }
                else
                {
                    // 发生了错误，打印错误信息
                    perror("send error");
                    epoll_ctl(epfd, EPOLL_CTL_DEL, fd, nullptr);
                    close(fd);
                    return -1;
                }
            }
            // printf("sent to fd %d: [%d]%s\n", fd, count,conns[fd].wbuffer);
            conns[fd].woffset += sended;

            // printf(" %ld  \n",conns[fd].wremain);
        }
        else
        {

            size_t remaining = conns[fd].wtotallength - conns[fd].woffset;

            ssize_t sent = sendfile(conns[fd].fd, conns[fd].w_file_fd, &conns[fd].wfile_offset, remaining);
            if (sent <= 0)
            {
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                {
                    return 0;
                }
                else
                {
                    // 发生了错误，打印错误信息
                    perror("send error");
                    epoll_ctl(epfd, EPOLL_CTL_DEL, fd, nullptr);
                    close(fd);
                    return -1;
                }
            }

            conns[fd].woffset += sent;
        }

        // 事件切换控制
        if (conns[fd].wstatus == 0)
        {
            // set_event(fd, EPOLLOUT, 2);
        }
        else if (conns[fd].wstatus == 1)
        {
            // set_event(fd, EPOLLOUT, 2);
            if (conns[fd].woffset == conns[fd].wtotallength)
            {
                conns[fd].wstatus = 2;
            }
        }

        // 复位
        if (conns[fd].wstatus == 2)
        {
            conns[fd].reset_w();
            set_event(fd, RECV_EV, 2);
        }
        return 0;
    }
    int register_clinetfd(int fd, int event)
    {
        conns[fd].fd = fd;
        conns[fd].recv_cb = read_callback;
        conns[fd].send_cb = write_callback;

        set_event(fd, event);
        return 0;
    }
    // 注册新链接，监听fd用
    int listen_callback(int fd)
    {
        sockaddr_in clientaddr;
        socklen_t len = sizeof(clientaddr);

        int clientfd = accept(fd, (sockaddr *)&clientaddr, &len);

        if (clientfd < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                return 0; // 没有更多连接了
            }

            else
            {
                perror("accept error");
                return -1;
            }
        }
        else
        {
            printf("accept fd %d\n", clientfd);

            int flag = fcntl(clientfd, F_GETFL, 0);
            fcntl(clientfd, F_SETFL, flag | O_NONBLOCK); // 设clientfd为非阻塞

            register_clinetfd(clientfd, RECV_EV);

            return 0;
        }
    }
    int register_listenfd(int fd, int event = LISTEN_EV)
    {
        conns[fd].fd = fd;
        conns[fd].recv_cb = listen_callback;
        conns[fd].send_cb = nullptr;

        int flag = fcntl(fd, F_GETFL, 0);
        fcntl(fd, F_SETFL, flag | O_NONBLOCK);

        set_event(fd, event);
        return 0;
    }
}

int main()
{
    uint16_t port = 2006;
    int sockfd = test8::init_server(port); // 初始化一个监听fd
    test8::register_listenfd(sockfd);

    while (true)
    {
        epoll_event events[1024] = {0};
        int readynum = epoll_wait(test8::epfd, events, 1024, -1);
        for (int i = 0; i < readynum; i++)
        {
            int curfd = events[i].data.fd;
            if (events[i].events & EPOLLIN)
            {
                test8::conns[curfd].recv_cb(curfd);
            }
            if (events[i].events & EPOLLOUT)
            {
                test8::conns[curfd].send_cb(curfd);
            }
        }
    }

    return 0;
}