#include <sys/socket.h>
#include <arpa/inet.h>
#include <string>
#include <error.h>
#include <unistd.h>
#include <fcntl.h>
class udp_sock
{
private:
    sockaddr_in addr_;
    int family_ = AF_INET; // AF_INET或AF_INET6
    int fd_;
    std::string ip_;
    uint16_t port_;

public:
    udp_sock(int family, const std::string &ip, uint16_t port) : ip_(ip), port_(port), family_(family)
    {
        addr_.sin_family = family;
        inet_aton(ip.c_str(), &addr_.sin_addr);
        addr_.sin_port = htons(port);
        fd_ = socket(family_, SOCK_DGRAM, 0);

        if (fd_ == -1)
        {
            perror("create fd error");
        }
    }
    udp_sock(int family):family_(family)
    {
        addr_.sin_family = family;
        fd_ = socket(family_, SOCK_DGRAM, 0);

        if (fd_ == -1)
        {
            perror("create fd error");
        }
    }
    std::string getaddrtostring()
    {
        std::string res;
        res.append(inet_ntoa(addr_.sin_addr)).append(" : ").append(std::to_string(port_));
        return res;
    }
    ~udp_sock()
    {
        close(fd_);
        printf("close fd %d\n", fd_);
    }

    bool setunblock()
    {
        int flag = fcntl(fd_, F_GETFL);
        return 0 == fcntl(fd_, F_SETFL, flag | O_NONBLOCK);
    }
    bool bind_local(const sockaddr *addr,size_t addrlen){
        int ret = bind(fd_,addr,addrlen);
        if(ret == -1){
            perror("bind error ");
            return false;
        }

        return true;
    }


    int recvudp(void * buf,size_t recvnum,sockaddr *srcaddr,socklen_t addrlen)
    {
        return recvfrom(fd_,buf,recvnum,0,srcaddr,nullptr);
    }
    int sendudp(void * buf,size_t sendnum,sockaddr *dstaddr,socklen_t addrlen){
        return sendto(fd_,buf,sendnum,0,dstaddr,addrlen);
    }
};