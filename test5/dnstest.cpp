#include <cstdint>
#include <vector>
#include <random>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string>
#include <stdio.h>
#include <unistd.h>

namespace test5
{

#define DNS_SERVER_PORT 53
#define DNS_SERVER_IP "114.114.114.114"
    uint16_t get_random_id()
    {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_int_distribution<uint16_t> uint16_dist;
        return uint16_dist(gen);
    }

    class dns_header
    {
        // 发送前再转换字节序
    public:
        uint16_t id;
        uint16_t flags;
        uint16_t questions;
        uint16_t answer_rrs;
        uint16_t authority_rrs;
        uint16_t additional_rrs;
        int create(uint16_t _flag, uint16_t _ques_num = 1)
        {
            id = get_random_id();
            flags = _flag;
            questions = _ques_num;
            answer_rrs = 0; //这个部分，还有下面必须初始化
            authority_rrs = 0;
            additional_rrs = 0;

            return 0;
        }
    };

    class dns_queries
    {

    public:
        // 发送前再转换字节序
        std::vector<uint8_t> qname;
        uint16_t qtype;
        uint16_t qclass;
        int encode_hostname(const std::string &hostname, std::vector<uint8_t> &result)
        {
            if (hostname.empty())
            {
                return -2;
            }
            result.clear();
            size_t index = 0;

            while (index < hostname.size())
            {
                auto dot_index = hostname.find_first_of('.', index);
                if (dot_index == std::string::npos)
                {
                    dot_index = hostname.size();
                }
                int domain_len = dot_index - index;
                if (domain_len >= 63 || domain_len < 1)
                {
                    printf("invalid domain name length\n");
                    return -1;
                }
                result.push_back(uint8_t(domain_len));
                for (size_t i = index; i < dot_index; i++)
                {
                    result.push_back(uint8_t(hostname.at(i)));
                }

                index = dot_index + 1;
            }
            result.push_back(0x00);

            return 0;
        }

        enum TYPE
        {
            A = 1,
            NS = 2,
            CNAME = 5,

        };

        int create(const std::string &hostname, TYPE _type, uint16_t _class = 1)
        {
            if (0 != encode_hostname(hostname, this->qname))
            {
                return -1;
            }
            qtype = (uint16_t)_type;
            qclass = _class;
            return 0;
        }
    };
    int dns_build_request(const dns_header &head, const dns_queries &qry, std::string &request)
    {
        request.clear();
        auto append_u16 = [&request](uint16_t v)
        {
            uint16_t v2 = htons(v);
            request.append((const char *)(&v2), sizeof(v2));
        };
        append_u16(head.id);
        append_u16(head.flags);
        append_u16(head.questions);
        append_u16(head.answer_rrs);
        append_u16(head.authority_rrs);
        append_u16(head.additional_rrs);

        request.insert(request.end(), qry.qname.begin(), qry.qname.end());
        append_u16(qry.qtype);
        append_u16(qry.qclass);

        return 0;
    }
    // 解析dns响应，打印所有A记录IPv4地址，gpt生成
    void print_dns_a_records(const uint8_t *response, size_t len)
    {

        // DNS头部固定12字节

        uint16_t question_num = ntohs(*(uint16_t *)(response + 4));   // 5
        uint16_t answer_ancount = ntohs(*(uint16_t *)(response + 6)); // 7

        size_t pos = 12;

        // 跳过问题区
        for (int i = 0; i < question_num; i++)
        {
            // 跳过QNAME（格式是标签长度+标签，直到0x00结束）
            while (pos < len && response[pos] != 0)
            {
                pos += response[pos] + 1;
            }
            pos += 1; // 跳过末尾的0x00
            pos += 4; // QTYPE(2) + QCLASS(2)
        }

        // 解析Answer区
        for (int i = 0; i < answer_ancount; i++)
        {
            if (pos + 12 > len) // NAME(2), TYPE(2), CLASS(2), TTL(4), RDLENGTH(2)
            {
                printf("回答区格式错误\n");
                return;
            }

            // NAME字段通常是指针（2字节）
            // 直接跳过2字节
            pos += 2;

            uint16_t type = ntohs(*(uint16_t *)(response + pos));
            pos += 2;
            uint16_t clas = ntohs(*(uint16_t *)(response + pos));
            pos += 2;
            uint32_t ttl = ntohl(*(uint32_t *)(response + pos));
            pos += 4;
            uint16_t rdlength = ntohs(*(uint16_t *)(response + pos));
            pos += 2;

            if (type == 1 && clas == 1 && rdlength == 4)
            {
                // IPv4地址
                const uint8_t *ip = response + pos;
                printf("IPv4: %u.%u.%u.%u\n", ip[0], ip[1], ip[2], ip[3]);
            }

            pos += rdlength;
        }
    }
    int dns_commit(const std::string &hostname)
    {
        auto sockfd = socket(AF_INET, SOCK_DGRAM, 0); // UDP

        if (sockfd < 0)
        {
            return -1;
        }
        struct sockaddr_in dns_servaddr = {0};
        dns_servaddr.sin_family = AF_INET; // IPV4
        dns_servaddr.sin_port = htons(DNS_SERVER_PORT);
        dns_servaddr.sin_addr.s_addr = inet_addr(DNS_SERVER_IP);

        // 连接
        if (0 != connect(sockfd, (struct sockaddr *)(&dns_servaddr), sizeof(dns_servaddr)))
        {
            printf("connect fault\n");
            return -4;
        }

        dns_header dh;
        dh.create(0x0100, 1);
        dns_queries dq;
        dq.create(hostname, dns_queries::A, 1);

        // 请求
        std::string request;
        dns_build_request(dh, dq, request);

        ssize_t send_num = send(sockfd, request.c_str(), request.size(), 0);
        if (send_num <= 0)
        {
            return -2;
        }
        uint8_t response[2048] = {0};
        socklen_t addr_len = sizeof(struct sockaddr_in);

        // 接收
        struct sockaddr_in recvaddr;
        auto recv_num = recv(sockfd, response, sizeof(response), 0);

        if (recv_num <= 0)
        {
            return -3;
        }
        // printf("receive %ld bytes form dns server :\n", recv_num);
        // for (int i = 0; i < recv_num; i++)
        // {
        //     printf("%02x ", (uint8_t)response[i]);
        //     if ((i + 1) % 4 == 0)
        //         printf("\n");
        // }
        // printf("\n");
        print_dns_a_records(response, recv_num);
        return 0;
    }

}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        return -1;
    }
    for (int i = 1; i < argc; i++)
    {
        test5::dns_commit(argv[i]);
    }

    return 0;
}