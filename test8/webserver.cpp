#include "webserver.h"

namespace test8
{

    test8::connection conns[MAX_CONNS] = {};
    int epfd = epoll_create(1);


    int http_request(connection &conn)
    {

        // printf("request:\n%s", conn.rbuffer);

        //memset(conn.wbuffer, 0, BUFFER_LENGTH);
        conn.wlength = 0;
        conn.wstatus = 0;
        return 0;
    }
    // 给客户端发送响应
    int http_response(connection &conn)
    {
        // 缓存数据
        static std::string cached_file_content;
        static std::string cached_response_header;
        char filename[] = WEBSERVER_ROOTDIR "green.jpg";
        if (cached_file_content.empty())
        {
            FILE *fp = fopen(filename, "rb");

            fseek(fp, 0, SEEK_END);
            size_t len = ftell(fp);
            rewind(fp);

            cached_file_content.resize(len);
            fread(&cached_file_content[0], 1, len, fp);
            fclose(fp);

            char header[BUFFER_LENGTH];
            int hlen = snprintf(header, sizeof(header),
                                "HTTP/1.1 200 OK\r\n"
                                "Content-Type: image/jpg\r\n"
                                "Content-Length: %ld\r\n"
                                "Connection: close\r\n\r\n",
                                len);
            cached_response_header.assign(header, hlen);
        }

        // 数据初始化
        if (conn.wtotallength == 0)
        {
            conn.wtotallength = cached_response_header.size() + cached_file_content.size();
            conn.woffset = 0;
        }

        // 控制状态切换
        if (conn.woffset == cached_response_header.size())
        {
            conn.wstatus = 1;
            // 太长的文件交由filesend处理，判断条件为conn.w_file_fd!=-1
            if (conn.wtotallength > BUFFER_LENGTH * 3)
            {
                int filefd = open(filename, O_RDONLY);
                if (filefd < 0)
                {
                    perror("open");
                    return -1;
                }
                else
                {
                    conn.w_file_fd = filefd;
                    conn.wlength = cached_file_content.size();
                    return 0;
                }
            }
        }

        if (conn.wstatus == 0)
        {
            int num = cached_response_header.size() - conn.woffset;
            memcpy(conn.wbuffer.data(), cached_response_header.data() + conn.woffset, num);

            conn.wlength = num;
        }
        if (conn.wstatus == 1)
        {
            int offset_t = conn.woffset - cached_response_header.size();
            size_t readnum = std::min(cached_file_content.size() - offset_t, BUFFER_LENGTH);

            memcpy(conn.wbuffer.data(), cached_file_content.data() + offset_t, readnum);
            conn.wlength = readnum;
        }
        return 0;
    }
}
