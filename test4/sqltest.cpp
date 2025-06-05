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

    return 0;
}