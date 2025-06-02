#include <stdio.h>

bool is_letter(const char w)
{
    return (w <= 'z' && w >= 'a') || (w <= 'Z' && w >= 'A');
}

enum class Status_wd {
    INTERVAL = 0,    // 间隔状态
    WORD_EN = 1, // 单词状态
};
int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        printf("需要参数：文件路径(./count_word /home/book.txt)\n");
        return 1;
    }

    constexpr size_t SIZE_BUFFER = 2048; // 缓冲区大小

    const char *file_name = argv[1];
    FILE *fp_cw = fopen(file_name, "rb");
    if (fp_cw == nullptr)
    {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        return 1;
    }

    size_t word_cnt = 0;        // 单词计数
    char buffer_r[SIZE_BUFFER]; // 缓冲区
    size_t read_num = 0;        // 实际读取数

    Status_wd status = Status_wd::INTERVAL; // 当前状态

    do
    {
        read_num = fread(buffer_r, sizeof(char), SIZE_BUFFER, fp_cw);
        for (size_t i = 0; i < read_num; i++)
        {
            if (is_letter(buffer_r[i]))
            {
                // 扫描到字母
                if (status == Status_wd::INTERVAL)
                {
                    // 间隔->字母
                    word_cnt++;
                    status = Status_wd::WORD_EN;
                }
            }
            else
            {
                // 扫描到间隔符
                if (status == Status_wd::WORD_EN)
                {
                    // 字母->间隔
                    status = Status_wd::INTERVAL;
                }
            }
        }
    } while (read_num != 0);
    fclose(fp_cw);

    printf("word_en_num: %zu\n", word_cnt);
    return 0;
}
