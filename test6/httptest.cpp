
#include <string>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <iostream>
#include<sys/socket.h>
namespace test6
{
    constexpr size_t BUFFER_SIZE = 4096;
    // 根据域名，获取一个字符串类型的 点分十进制IP地址 比如“192.168.31.210”
    std::string host_to_ip(const std::string &hostname)
    {
        struct addrinfo req{};
        req.ai_family = AF_INET;
        req.ai_socktype = SOCK_DGRAM;

        struct addrinfo *result;
        auto res_num = getaddrinfo(hostname.data(), nullptr, &req, &result);
        if (res_num != 0 || result == nullptr)
        {
            return {};
        }

        // 只取第一个ip
        auto temp_addr = (struct sockaddr_in *)result->ai_addr;

        std::string ip_str = inet_ntoa(temp_addr->sin_addr);

        freeaddrinfo(result);
        return ip_str;
    }
    // 创建 连接到 字符串类型的 点分十进制IP地址 表示的主机 的socket，返回socket
    int http_create_socket(const std::string &ip)
    {
        int sockfd = socket(AF_INET, SOCK_STREAM, 0); // tcp

        struct sockaddr_in servaddr = {0};
        servaddr.sin_family = AF_INET;
        servaddr.sin_port = htons(80);
        servaddr.sin_addr.s_addr = inet_addr(ip.data());

        if (0 != connect(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)))
        {
            close(sockfd);
            return -1;
        }

        auto origin_flags = fcntl(sockfd, F_GETFL);
        fcntl(sockfd, F_SETFL, origin_flags | O_NONBLOCK); // 设为非阻塞模式

        return sockfd;
    }
    // 向hostname 发http请求
    std::string http_send_get_request(const std::string &hostname, const std::string &resourse)
    {
        std::string ip = host_to_ip(hostname);
        if (ip.empty())
        {
            std::cout << "failed to resolve hostname\n";
            return "";
        }
        int sockfd = http_create_socket(ip);

        std::string request;
        request = "GET " + resourse + " HTTP/1.1\r\n";
        request += "Host: " + hostname + "\r\n";
        request += "Connection: close\r\n";
        request += "\r\n";

        auto send_num = send(sockfd, request.data(), request.size(), 0);
        if (send_num != request.size())
        {
            close(sockfd);
            return "";
        }

        char buffer[BUFFER_SIZE];
        std::string response;

        fd_set fdread;
        while (true)
        {
            FD_ZERO(&fdread);
            FD_SET(sockfd, &fdread);
            timeval tv = {5, 0}; // 5s-》最多阻塞5s
            int select_num = select(sockfd + 1, &fdread, nullptr, nullptr, &tv);
            if (select_num <= 0 || !FD_ISSET(sockfd, &fdread))
            {
                break; // 有问题
            }
            else if (select_num > 0)
            {
                int len = recv(sockfd, buffer, BUFFER_SIZE, 0);
                if (len == 0)
                {
                    break; // 服务器关闭连接
                }
                else if (len > 0)
                {
                    response.append(buffer, len); // 可能要读多次
                }
            }
        }
        close(sockfd);

        return response;
    }

}

int main(int argc, char *argv[])
{

    // arg1：hostname arg2: resourse
    if (argc < 3)
    {
        return -1;
    }
    auto response = test6::http_send_get_request(argv[1], argv[2]);
    std::cout << response << std::endl;


    return 0;
}