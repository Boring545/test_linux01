#ifndef CONNECTION_POOL
#define CONNECTION_POOL

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
    // 只能连接一个【用户、数据库】
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
                    std::cout << "mysql connect error" << std::endl;
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
    

}

#endif