#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <string>
#include <memory.h>
#include <sys/stat.h>
#include <sys/sendfile.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <iostream>
#include <error.h>
#include <unistd.h>
#include <poll.h>
#include <sys/epoll.h>
#include <fcntl.h>

#include <functional>

#define WEBSERVER_ROOTDIR "./"
namespace test8
{
    constexpr size_t BUFFER_LENGTH = 1024;
    constexpr size_t MAX_CONNS = 1048576;

    struct connection
    {
        using RCALLBACK = std::function<int(int)>;
        int fd;
        std::vector<char> rbuffer;
        size_t rlength = 0;

        std::vector<char> wbuffer;
        size_t wlength = 0; //写缓冲区实际装载的数据大小
        int wstatus = 0; // 0初始状态 1发送完头部，需要发送body
        size_t woffset = 0; //已经写的偏移
        size_t wtotallength = 0;    // 需要写的总数据长度

        int w_file_fd = -1; // 大文件的fd
        off_t wfile_offset = 0; //大文件内发送的偏移

        RCALLBACK send_cb; // 可写 epollout
        RCALLBACK recv_cb; // 可读 epollin,也可以对应listenfd监听到连接,事件也是可读 epollin

        connection()
        {
            rbuffer.resize(BUFFER_LENGTH);
            wbuffer.resize(BUFFER_LENGTH);
        }
        void reset_w()
        {
            wstatus = 0;

            woffset = 0;
            wlength = 0;
            wtotallength = 0;

            if (w_file_fd != -1)
            {
                wfile_offset = 0;
                close(w_file_fd);
                w_file_fd = -1;
            }
        }
    };

    extern connection conns[MAX_CONNS];
    extern int epfd;

    // 处理客户端响应
    int http_request(connection &conn);

    // 给客户端发送响应
    int http_response(connection &conn);

}

#endif