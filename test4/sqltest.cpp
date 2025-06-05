#include <mysql.h>
#include <iostream>
#include <vector>
namespace test4
{
#define ZJQ_DB_SERVER_HOST "192.168.31.210"
#define ZJQ_DB_SERVER_PORT 3306
#define ZJQ_DB_SERVER_USR "zjq"
#define ZJQ_DB_SERVER_PWD "123456"
#define ZJQ_DB "zjq_db"

    int zjq_mysql_selcet(MYSQL *sql)
    {
        const std::string SQL_SELECT_TBL_USR_01 = "select * from tbl_usr;";
        if (0 != mysql_query(sql, SQL_SELECT_TBL_USR_01.c_str()))
        {
            std::cout << mysql_error(sql) << std::endl;
            return -1;
        }

        auto res = mysql_store_result(sql);
        if (res == nullptr)
        {
            std::cout << mysql_error(sql) << std::endl;
            return -2;
        }
        auto rows = mysql_num_rows(res);
        auto cols = mysql_num_fields((res));
        for (int i = 0; i < rows; i++)
        {
            auto row = mysql_fetch_row(res);
            for (int j = 0; j < cols; j++)
            {
                std::cout << (row[j] != nullptr ? row[j] : "NULL") << "\t";
            }
            std::cout << std::endl;
        }

        mysql_free_result(res);
        return 0;
    }

    int zjq_mysql_insert(MYSQL *sql)
    {
        const std::string SQL_INSERT_TBL_USR_01 = "insert into tbl_usr (u_name,u_gender) values('zjq','man');";
        const std::string SQL_INSERT_TBL_USR_02 = "insert into tbl_usr (u_name,u_gender) values('alice','woman');";
        if (0 == mysql_query(sql, SQL_INSERT_TBL_USR_01.c_str()))
        {
            std::cout << "Insert 1 success" << std::endl;
        }
        else
        {
            std::cout << "Insert failed: " << mysql_error(sql) << std::endl;
            return -1;
        }
        if (mysql_query(sql, SQL_INSERT_TBL_USR_02.c_str()) == 0)
        {
            std::cout << "Insert 2 success" << std::endl;
        }
        else
        {
            std::cout << "Insert 2 failed: " << mysql_error(sql) << std::endl;
            return -2;
        }

        return 0;
    }
    int zjq_mysql_delete(MYSQL *sql)
    {
        const std::string SQL_DELETE_TBL_USR_01 = "call delete_by_name01('zjq');";
        if (0 == mysql_query(sql, SQL_DELETE_TBL_USR_01.c_str()))
        {
            std::cout << "delete success" << std::endl;
        }
        else
        {
            std::cout << "delete failed: " << mysql_error(sql) << std::endl;
            return -1;
        }

        return 0;
    }
    int write_img(const std::string &filename, std::vector<char> &buffer)
    {

        if (buffer.empty())
        {
            return -3; // 空buffer，不写
        }
        FILE *fp = fopen(filename.c_str(), "wb");
        if (fp == nullptr)
        {
            return -1;
        }
        auto write_size = fwrite(buffer.data(), sizeof(char), buffer.size(), fp);
        if (write_size != buffer.size())
        {
            return -2;
        }

        fclose(fp);
        return buffer.size();
    }
    int read_img(const std::string &filename, std::vector<char> &buffer)
    {
        FILE *fp = fopen(filename.c_str(), "rb");
        if (fp == nullptr)
        {
            return -1;
        }

        fseek(fp, 0, SEEK_END);
        auto length = ftell(fp); // 字节
        fseek(fp, 0, SEEK_SET);

        if (length <= 0)
        {
            fclose(fp);
            return -2;
        }

        buffer.resize(length);
        auto read_size = fread(buffer.data(), sizeof(char), length, fp);
        fclose(fp);
        if (read_size != length)
        {
            return -3;
        }

        return 0;
    }
    int mysql_write(MYSQL *sql, std::vector<char> &buffer)
    {
        const std::string SQL_INSERT_TBL_USR_03 = "insert into tbl_usr (u_name,u_gender,img) values('bob','man',?);";

        auto stmt = mysql_stmt_init(sql);
        if (stmt == nullptr)
        {
            return -1;
        }
        if (mysql_stmt_prepare(stmt, SQL_INSERT_TBL_USR_03.c_str(), SQL_INSERT_TBL_USR_03.size()))
        {
            mysql_stmt_close(stmt);
            return -2;
        }
        MYSQL_BIND bind{0};
        bind.buffer_type = MYSQL_TYPE_LONG_BLOB;
        bind.buffer_length = buffer.size();
        bool isnull = 0;
        bind.is_null = &isnull;
        unsigned long len = buffer.size();
        bind.length = &len;
        bind.buffer = nullptr;
        // bind.buffer = buffer.data(); //对于小数据可以这么写，代替mysql_stmt_send_long_data

        if (mysql_stmt_bind_param(stmt, &bind) != 0)
        {
            return -3;
        }

        size_t offset = 0;
        const size_t chunk_size = 1024;
        while(offset<buffer.size()){
            size_t write_num=(buffer.size()-offset)>chunk_size?chunk_size:(buffer.size()-offset);
            mysql_stmt_send_long_data(stmt,0,buffer.data()+offset,write_num);
            offset+=write_num;
        }


        if (mysql_stmt_execute(stmt) != 0)
        {
            return -4;
        }
        mysql_stmt_close(stmt);
        return 0;
    }
    int mysql_read(MYSQL *sql, std::vector<char> &buffer)
    {
        const std::string SQL_SELECT_TBL_USR_02 = "select img from tbl_usr where u_name='bob';";
        auto stmt = mysql_stmt_init(sql);
        if (stmt == nullptr)
        {
            return -1;
        }
        if (0 != mysql_stmt_prepare(stmt, SQL_SELECT_TBL_USR_02.data(), SQL_SELECT_TBL_USR_02.size()))
        {
            mysql_stmt_close(stmt);
            return -2;
        }

        if (0 != mysql_stmt_execute(stmt))
        {
            mysql_stmt_close(stmt);
            return -3;
        }

        MYSQL_BIND bind{0};
        bind.buffer_type = MYSQL_TYPE_LONG_BLOB;

        unsigned long len = 0;
        bind.length = &len;
        bind.buffer = nullptr;
        bind.buffer_length = 0;

        bool isnull = 0;
        bind.is_null = &isnull;

        if (mysql_stmt_bind_result(stmt, &bind) != 0)
        {
            mysql_stmt_close(stmt);
            return -4;
        }

        buffer.clear();

        int flag = mysql_stmt_fetch(stmt); // fetch这一步才把数据下载
        if (flag != 0 && flag != MYSQL_DATA_TRUNCATED)
        {
            // MYSQL_DATA_TRUNCATED第一次mysql_stmt_fetch的时候
            // 因为缓冲区为空，返回表示因为缓冲区过小，数据被“截断”了
            mysql_stmt_close(stmt);
            return -5; // 读取失败或者无数据
        }
        buffer.resize(len);
        const size_t chunk_size = 1024; // 分块读取时缓冲区大小=1024byte
        size_t offset = 0;

        while (offset < (int)len)
        {
            size_t read_num = (len - offset) > chunk_size ? chunk_size : (len - offset);
            bind.buffer_length = read_num; // 每次读缓冲区大小为1字节的内容
            bind.buffer = buffer.data() + offset;
            mysql_stmt_fetch_column(stmt, &bind, 0, offset);
            offset += read_num;
        }

        mysql_stmt_close(stmt);
        return len;
    }
}

int main()
{
    MYSQL *sql = mysql_init(nullptr);
    if (sql == nullptr)
    {
        std::cout << "mysql init: " << mysql_error(sql) << std::endl;
        return -1;
    }
    if (mysql_real_connect(sql, ZJQ_DB_SERVER_HOST, ZJQ_DB_SERVER_USR, ZJQ_DB_SERVER_PWD, ZJQ_DB, ZJQ_DB_SERVER_PORT, nullptr, 0) == nullptr)
    {
        std::cout << "mysql connect error" << mysql_error(sql) << std::endl;
        mysql_close(sql);
        return -2;
    }
    test4::zjq_mysql_insert(sql);
    test4::zjq_mysql_selcet(sql);
    test4::zjq_mysql_delete(sql);
    test4::zjq_mysql_selcet(sql);

    std::string filename1 = "./green.jpg";
    std::string filename2 = "./green_download.jpg";

    std::vector<char> buf_w;
    std::vector<char> buf_r;

    test4::read_img(filename1, buf_w);
    test4::mysql_write(sql, buf_w);

    test4::zjq_mysql_selcet(sql);

    test4::mysql_read(sql, buf_r);
    test4::write_img(filename2, buf_r);

    return 0;
}