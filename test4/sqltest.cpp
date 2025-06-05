#include <mysql.h>
#include <iostream>
#include <vector>
#include <mutex>
#include <queue>
#include <string>
#include <condition_variable>
#include <atomic>
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
        std::cout<<"========================="<<std::endl;
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
        return 0;
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
        while (offset < buffer.size())
        {
            size_t write_num = (buffer.size() - offset) > chunk_size ? chunk_size : (buffer.size() - offset);
            mysql_stmt_send_long_data(stmt, 0, buffer.data() + offset, write_num);
            offset += write_num;
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
        return 0;
    }
    class ConnectionPool
    {

    private:
        std::queue<MYSQL *> connections;
        std::mutex mutex_cp;
        std::condition_variable cv_cp;
        std::atomic<bool> active_flag;
        size_t total_connection;
        size_t available_connection;

    public:
        ConnectionPool() : active_flag(false), total_connection(0), available_connection(0) {}
        ~ConnectionPool()
        {
            shutdown();
        }
        ConnectionPool(const ConnectionPool &) = delete;
        ConnectionPool &operator=(const ConnectionPool &) = delete;

        //
        bool init(const std::string &host, const std::string &user, const std::string &password, const std::string &db_name, unsigned int port, size_t max_conn_num = 3)
        {
            std::lock_guard<std::mutex> lc{mutex_cp};
            if (active_flag.load() == true)
            {
                std::cout << "Already initialized" << std::endl;
                return false;
            }
            std::vector<MYSQL *> created;
            size_t i = 0;
            for (; i < max_conn_num; i++)
            {
                auto conn = mysql_init(nullptr);
                if (conn == nullptr)
                {
                    std::cout << "mysql_init fault." << std::endl;
                    break;
                }
                if (nullptr == mysql_real_connect(conn, host.c_str(), user.c_str(), password.c_str(), db_name.c_str(), port, nullptr, 0))
                {
                    std::cout << "mysql connect error"  << std::endl;
                    mysql_close(conn);
                    break;
                }
                created.push_back(conn);
            }
            if (i < max_conn_num)
            {
                for (auto c : created)
                {
                    mysql_close(c);
                }
                std::cout << "Initialization connection failed, return all temporary connections" << std::endl;
                return false;
            }
            for (auto c : created)
            {
                connections.push(c);
            }
            active_flag.store(true);
            total_connection += max_conn_num;
            available_connection += max_conn_num;
            cv_cp.notify_all();
            return true;
        }
        MYSQL *acquire()
        {
            std::unique_lock<std::mutex> lc{mutex_cp};
            cv_cp.wait(lc, [this]
                       { return !connections.empty() || !active_flag.load(); });

            if (!active_flag.load() || connections.empty())
                return nullptr;

            auto conn = connections.front();
            connections.pop();
            available_connection--;
            return conn;
        }
        void release(MYSQL *conn)
        {
            if (conn == nullptr)
            {
                std::cout << "The released connection is null" << std::endl;
                return;
            }
            std::lock_guard<std::mutex> lc{mutex_cp};
            connections.push(conn);
            available_connection++;
            cv_cp.notify_one();
        }

        // 未考虑全部归还连接的问题
        void shutdown()
        {
            std::lock_guard<std::mutex> lc(mutex_cp);
            active_flag.store(false);

            while (!connections.empty())
            {
                auto conn = connections.front();
                connections.pop();
                mysql_close(conn);
            }
            total_connection = 0;
            available_connection = 0;
            cv_cp.notify_all();
        }
        size_t available()
        {
            std::lock_guard<std::mutex> lc(mutex_cp);
            return available_connection;
        }
    };
    bool sql_test(ConnectionPool &pool)
    {
        MYSQL *sql = pool.acquire();
        if (!sql)
        {
            std::cerr << "Failed to acquire mysql connection from pool." << std::endl;
            return false;
        }

        if (0!=test4::zjq_mysql_insert(sql))
        {
            std::cerr << "Insert failed" << std::endl;
            pool.release(sql);
            return false;
        }
        if (0!=test4::zjq_mysql_selcet(sql))
        {
            std::cerr << "Select  failed" << std::endl;
            pool.release(sql);
            return false;
        }
        if (0!=test4::zjq_mysql_delete(sql))
        {
            std::cerr << "Delete failed" << std::endl;
            pool.release(sql);
            return false;
        }
        if (0!=test4::zjq_mysql_selcet(sql))
        {
            std::cerr << "Select  failed" << std::endl;
            pool.release(sql);
            return false;
        }

        std::string filename1 = "./green.jpg";
        std::string filename2 = "./green_download.jpg";
        std::vector<char> buf_w, buf_r;

        if (0!=test4::read_img(filename1, buf_w))
        {
            std::cerr << "Failed to read image from " << filename1 << std::endl;
            pool.release(sql);
            return false;
        }

        if (0!=test4::mysql_write(sql, buf_w))
        {
            std::cerr << "Failed to write image blob to mysql" << std::endl;
            pool.release(sql);
            return false;
        }

        if (0!=test4::zjq_mysql_selcet(sql))
        {
            std::cerr << "Select after image write failed" << std::endl;
            pool.release(sql);
            return false;
        }

        if (0!=test4::mysql_read(sql, buf_r))
        {
            std::cerr << "Failed to read image blob from mysql" << std::endl;
            pool.release(sql);
            return false;
        }

        if (0!=test4::write_img(filename2, buf_r))
        {
            std::cerr << "Failed to write image to " << filename2 << std::endl;
            pool.release(sql);
            return false;
        }

        pool.release(sql);
        return true;
    }
}

int main()
{
    test4::ConnectionPool pool;
    if (!pool.init(ZJQ_DB_SERVER_HOST, ZJQ_DB_SERVER_USR, ZJQ_DB_SERVER_PWD, ZJQ_DB, ZJQ_DB_SERVER_PORT, 3))
    {
        std::cerr << "Failed to initialize connection pool" << std::endl;
        return -1;
    }

    if (!test4::sql_test(pool))
    {
        std::cerr << "Database example run failed" << std::endl;
        pool.shutdown();
        return -2;
    }

    pool.shutdown();
    return 0;
}