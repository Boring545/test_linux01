#include <cstring>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <filesystem>
namespace fs = std::filesystem;

namespace test2
{
    constexpr int LENGTH_NAME = 16;
    constexpr int LENGTH_PHONE = 16;
#define INFO printf

    // 数据单元，链表存储数据
    template <class T>
    class Node
    {
    public:
        T data;
        Node *prev = nullptr;
        Node *next = nullptr;
        // 插入到链表first最前，item成为链表头
        static int insert(Node *item, Node *&first)
        {
            if (item == nullptr)
                return -1;
            if (first == nullptr)
            {
                item->next = nullptr;
                item->prev = nullptr;
                first = item;
                return 0;
            }

            item->next = first;
            item->prev = nullptr;
            first->prev = item;
            first = item;
            return 0;
        }
        // 如果链表first中有item，则删除其
        static int remove(Node *item, Node *&first)
        {
            if (contains(item, first) == false)
                return -1;

            if (item->prev != nullptr)
            {
                item->prev->next = item->next;
            }
            if (item->next != nullptr)
            {
                item->next->prev = item->prev;
            }
            if (item == first)
            {
                first = item->next;
            }

            item->next = nullptr;
            item->prev = nullptr;
            return 0;
        }
        // first链表中存在数据为target的节点（data一致）
        static bool search(const T &target, const Node *first)
        {
            if (first == nullptr)
                return false;

            auto ptr = first;
            while (ptr != nullptr)
            {
                if (target == ptr->data)
                {
                    return true;
                }
                ptr = ptr->next;
            }

            return false;
        }
        // 是否包含相同地址的节点
        static bool contains(Node *item, Node *first)
        {
            if (first == nullptr || item == nullptr)
                return false;
            auto ptr = first;
            while (ptr != nullptr)
            {
                if (ptr == item)
                    return true;
                ptr = ptr->next;
            }
            return false;
        }
    };
    class Person
    {
    public:
        char name[LENGTH_NAME];
        char phone[LENGTH_PHONE];
        bool operator==(const Person &other) const
        {
            return strcmp(this->name, other.name) == 0 && strcmp(this->phone, other.phone) == 0;
        }
    };

    using PNode = Node<Person>;
    // 通讯录
    class Contacts
    {
    public:
        PNode *data = nullptr;
        size_t count = 0;
        void destroy_contacts()
        {
            while (data != nullptr)
            {
                auto next = data->next;
                delete data;
                data = next;
            }
            data = nullptr;
            count = 0;
        }
        ~Contacts()
        {
            destroy_contacts();
        }
    };

    // 接口：插入person， 返回0成功
    int person_insert(PNode *item, PNode *&first)
    {
        if (item == nullptr)
            return -1;

        return PNode::insert(item, first);
    }

    // 接口：删除person，返回0成功
    int person_delete(PNode *item, PNode *&first)
    {
        if (item == nullptr)
            return -1;
        return PNode::remove(item, first); // 返回0成功
    }
    // 接口：查找并返回name匹配的node，返回nullptr失败,保证只返回在链表中的节点指针
    PNode *person_name_search(const char *name, PNode *first)
    {
        auto ptr = first;
        while (ptr != nullptr)
        {
            if (strcmp(ptr->data.name, name) == 0)
            {
                return ptr;
            }

            ptr = ptr->next;
        }
        return nullptr;
    }
    // 接口：遍历链表，返回0成功
    int person_traversal(PNode *first)
    {
        auto ptr = first;
        while (ptr != nullptr)
        {
            INFO("name: %s, phone: %s\n", ptr->data.name, ptr->data.phone);
            ptr = ptr->next;
        }
        return 0;
    }
    bool mkdir_data_dir()
    {
        return fs::create_directory("./data/");
    }
    int save_file(PNode *first, std::string filename)
    {
        mkdir_data_dir();
        filename = "./data/" + filename;
        FILE *fp = fopen(filename.c_str(), "wb");
        if (fp == nullptr)
        {
            return -1;
        }

        constexpr size_t SIZE_BUFFER_P = 64;
        Person buffer_p[SIZE_BUFFER_P];
        size_t i = 0;

        auto item = first;
        while (item != nullptr)
        {
            buffer_p[i++] = item->data;
            if (i == SIZE_BUFFER_P)
            {
                auto wriitem_num = fwrite(buffer_p, sizeof(Person), SIZE_BUFFER_P, fp);
                if (wriitem_num != SIZE_BUFFER_P)
                {
                    fclose(fp);
                    return -2;
                }
                i = 0;
            }
            item = item->next;
        }
        if (i > 0)
        {
            auto wriitem_num = fwrite(buffer_p, sizeof(Person), i, fp);
            if (wriitem_num != i)
            {
                fclose(fp);
                return -2;
            }
        }
        fclose(fp);
        return 0;
    }
    int load_file(PNode *&first, std::string filename, size_t &count)
    {
        mkdir_data_dir();
        filename = "./data/" + filename;
        FILE *fp = fopen(filename.c_str(), "rb");
        if (fp == nullptr)
        {
            return -1;
        }
        constexpr size_t SIZE_BUFFER_P = 64; // 缓冲区大小
        Person buffer_p[SIZE_BUFFER_P];
        size_t read_num = 0;
        do
        {
            read_num = fread(buffer_p, sizeof(Person), SIZE_BUFFER_P, fp);
            for (int i = 0; i < read_num; i++)
            {
                auto item = new PNode();
                item->data = buffer_p[i];
                if (person_insert(item, first) == 0)
                {
                    count++;
                }
                else
                {
                    delete item;
                }
            }
        } while (read_num != 0);
        fclose(fp);
        return 0;
    }

    int insert_entry(Contacts *cts)
    {
        if (cts == nullptr)
        {
            return -1;
        }

        PNode *item = new PNode();

        if (item == nullptr)
        {
            return -2;
        }
        std::string name_tmp;
        std::string phone_tmp;

        std::cin.ignore(std::numeric_limits<std::streamsize>::max(),'\n');
        INFO("PLEASE INPUT A NAME: \n");
        std::getline(std::cin, name_tmp);
        strncpy(item->data.name, name_tmp.c_str(), LENGTH_NAME - 1);
        item->data.name[LENGTH_NAME - 1] = '\0';

        INFO("PLEASE INPUT A CORRESPONDING PHONE: \n");
        std::getline(std::cin, phone_tmp);
        strncpy(item->data.phone, phone_tmp.c_str(), LENGTH_PHONE - 1);
        item->data.phone[LENGTH_PHONE - 1] = '\0';

        // 不允许插入同名同电话的
        if (PNode::search(item->data, cts->data))
        {
            INFO("DUPLICATE ENTRY EXISTS\n");
            delete item;
            return -4;
        }

        auto flag = person_insert(item, cts->data);

        if (flag != 0)
        {
            delete item;
            return -3; // 插入失败
        }

        cts->count++;
        INFO("INSERT SUCCESS\n");

        return 0;
    }

    int print_entry(Contacts *cts)
    {
        if (cts == nullptr)
        {
            return -1;
        }
        return person_traversal(cts->data);
    }

    int delete_entry(Contacts *cts)
    {
        if (cts == nullptr)
        {
            return -1;
        }
        char name[LENGTH_NAME];
        INFO("PLEASE INPUT A NAME FOR DELETE: \n");
        std::string name_tmp;
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(),'\n');
        std::getline(std::cin, name_tmp);
        strncpy(name, name_tmp.c_str(), LENGTH_NAME - 1);
        name[LENGTH_NAME - 1] = '\0';

        auto ptr = person_name_search(name, cts->data);
        if (ptr == nullptr)
        {
            INFO("NO MATCH ENTRY TO DELETE\n");
            return -2; // 查找失败
        }

        auto flag = person_delete(ptr, cts->data);
        if (flag != 0)
            return -3; // 删除失败

        INFO("[ NAME: %s, PHONE:%s ] HAS BEEN DELETE\n", ptr->data.name, ptr->data.phone);
        delete ptr;
        cts->count--;
        return 0;
    }
    int search_entry(Contacts *cts)
    {
        if (cts == nullptr)
        {
            return -1;
        }
        char name[LENGTH_NAME];
        INFO("PLEASE INPUT A NAME FOR SEARCH: \n");

        std::string name_tmp;
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(),'\n');
        std::getline(std::cin, name_tmp);
        strncpy(name, name_tmp.c_str(), LENGTH_NAME - 1);
        name[LENGTH_NAME - 1] = '\0';

        auto ptr = person_name_search(name, cts->data);
        if (ptr == nullptr)
        {
            INFO("NO MATCH ENTRY TO SEARCH\n");
            return -2; // 查找失败
        }
        INFO("[ NAME: %s, PHONE:%s ] FOUND\n", ptr->data.name, ptr->data.phone);
        return 0;
    }
    int save_entry(Contacts *cts)
    {
        if (cts == nullptr)
        {
            return -1;
        }
        INFO("PLEASE INPUT FILENAME FOR SAVE ENTRIES: \n");
        std::string filename;
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(),'\n');
        std::getline(std::cin,filename);
        auto flag = save_file(cts->data, filename);
        if (flag != 0)
        {
            return -2;
        }

        INFO("SAVE SUCCESS TO ./data/%s",filename.c_str());
        return 0;
    }
    // 存在一个问题，可能会插入一些同名同电话的重复数据
    int load_entry(Contacts *cts)
    {
        if (cts == nullptr)
        {
            return -1;
        }
        INFO("PLEASE INPUT FILENAME FOR LOAD ENTRIES: \n");
        std::string filename;
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(),'\n');
        std::getline(std::cin,filename);
        
        auto flag = load_file(cts->data, filename.c_str(), cts->count);
        if (flag != 0)
        {
            return -2;
        }

        INFO("LOAD SUCCESS FROM ./data/%s",filename.c_str());
        return 0;
    }

    enum FUNCTION
    {
        OPTION_INSERT = 1, // 添加人员
        OPTION_PRINT = 2,  // 打印人员
        OPTION_DELETE = 3, // 删除人员
        OPTION_SEARCH = 4, // 查找人员
        OPTION_SAVE = 5,   // 保存文件
        OPTION_LOAD = 6,   // 加载文件
        OPTION_EXIT = 7,   // 加载文件
    };
    int menu_function()
    {
        Contacts *cts = new Contacts();
        int select = 0;
        while (true)
        {
            INFO("\n1. Insert\n2. Print\n3. Delete\n4. Search\n5. Save\n6. Load\n7. Exit\n");
            std::cin >> select;
            switch (select)
            {
            case OPTION_INSERT:
                /* code */
                insert_entry(cts);
                break;
            case OPTION_PRINT:
                /* code */
                print_entry(cts);
                break;
            case OPTION_DELETE:
                /* code */
                delete_entry(cts);
                break;
            case OPTION_SEARCH:
                /* code */
                search_entry(cts);
                break;
            case OPTION_SAVE:
                /* code */
                save_entry(cts);
                break;
            case OPTION_LOAD:
                /* code */
                load_entry(cts);
                break;
            case OPTION_EXIT:
                /* code */
                INFO("FUNCTION MENU EXIT\n");

                delete cts;
                return 0;
                break;
            default:
                break;
            }
        }
    }

}

int main()
{
    test2::menu_function();
    return 0;
}
