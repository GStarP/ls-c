#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>

#define MAX_FILE_NAME 50

struct INFO {
    int lineNum;   // 行数
    int wordNum;   // 单词数
    long byteNum;  // 字节数
    char name[MAX_FILE_NAME];  // 文件名
};

int main(int argc, char* argv[]) {
    // 根据要求，只需要完成对文件的 wc
    if (argc != 2) {
        printf("only support one arg as file path\n");
    }
    char path[MAX_FILE_NAME];
    strcpy(path, argv[1]);

    struct stat file = {};
    int ok = lstat(path, &file);
    if (ok == -1) {
        printf("stat error: %s\n", path);
    } else {
        // 给定的路径不能是目录
        if (S_ISDIR(file.st_mode)) {
            printf("arg must be normal file\n");
        } else {
            // 初始化
            struct INFO info = {};
            info.lineNum = 0;
            info.wordNum = 0;
            // 复制文件名
            strcpy(info.name, path);
            // 取字节数
            info.byteNum = file.st_size;
            FILE* fp = fopen(path, "r");
            if (fp == NULL) {
                printf("open file error\n");
            } else {
                char c;
                int inWord = 0;
                while((c = fgetc(fp)) != EOF) {
                    // 每次遇到换行认为行数增加
                    if (c == '\n') {
                        info.lineNum++;
                    }
                    // 每次碰到分割符，若之前处在非分隔符中，则认为一个单词结束
                    if ( c != '\t' && c != ' ' && c != '\n') {
                        inWord = 1;
                    }
                    if ( c == '\t' || c == ' ' || c == '\n') {
                        if (inWord == 1) {
                            info.wordNum++;
                            inWord = 0;
                        }
                    }
                }
                // 结束的 EOF 也需要判断之前是否处于非分隔符中
                if (inWord == 1) {
                    info.wordNum++;
                }
            }
            // 格式与系统的 wc 略有不同，但数字完全一致
            printf("%d %d %ld %s\n", info.lineNum, info.wordNum, info.byteNum, info.name);
        }
    }
}
