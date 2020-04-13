#include <string.h>
#include <stdio.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>

#define MAX_FILE_NAME 50
#define MAX_DIR_FILES 20

void main(int argc, char* argv[]) {
    // 初始化参数列表
    int R = 0, D = 1, A = 2, I = 3, L = 4;
    char optStr[5];
    optStr[R] = 'r';
    optStr[D] = 'd';
    optStr[A] = 'a';
    optStr[I] = 'i';
    optStr[L] = 'l';
    int opts[5];
    for (int i = 0; i < 5; ++i) opts[i] = 0;

    // 解析参数
    for (int i = 1; i < argc - 1; ++i) {
        for (int j = 0; j < 5; ++j) {
            char* op = argv[i];
            if (op[strlen(op) - 1] == optStr[j]) {
                opts[j] = 1;
            }
        }
    }
    // 解析路径
    char* lastOpt = argv[argc - 1];
    char dirPath[MAX_FILE_NAME];
    // 默认路径为当前工作目录
    if (lastOpt[0] == '-') {
        for (int j = 0; j < 5; ++j) {
            if (lastOpt[strlen(lastOpt) - 1] == optStr[j]) {
                opts[j] = 1;
            }
        }
        getcwd(dirPath, MAX_FILE_NAME);
    } else {
        strcpy(dirPath, lastOpt);
    }

    // 存储文件信息的数组
    int filesNum = 0;
    char fileNames[MAX_DIR_FILES][MAX_FILE_NAME];
    struct stat** files;
    files = (struct stat**) malloc(sizeof(struct stat*) * MAX_DIR_FILES);
    for (int i = 0; i < MAX_DIR_FILES; ++i) {
        *(files + i) = (struct stat*) malloc(sizeof(struct stat));
    }

    // 读取文件信息
    struct stat* entry = (struct stat*) malloc(sizeof(struct stat));
    int ok = lstat(dirPath, entry);
    if (ok == -1) {
        printf("lstat error: %s\n", dirPath);
    } else {
        // 当目标路径是目录时，打开并遍历目录
        if (S_ISDIR(entry->st_mode) == 1) {
            DIR* dir = opendir(dirPath);
            if (dir == NULL) {
                printf("no such dir\n");
            } else {
                struct dirent* curFile = (struct dirent*) malloc(sizeof(struct dirent));
                int file_idx = 0;
                while((curFile = readdir(dir)) != NULL) {
                    // 文件名无法保存在 stat 中，因此用一个数组单独保存
                    strcpy(fileNames[file_idx], curFile->d_name);
                    // 切换工作目录，方便 lstat 使用相对路径
                    chdir(dirPath);
                    ok = lstat(curFile->d_name, entry);
                    if (ok == -1) {
                        printf("lstat error: %s\n", curFile->d_name);
                    } else {
                        // 拷贝文件信息结构体
                        memcpy(files[file_idx], entry, sizeof(struct stat));
                        file_idx++;
                        filesNum++;
                    }
                }
            }
            closedir(dir);
        // 当目标路径是文件时，直接读取并存储
        } else {
            ok = lstat(dirPath, entry);
            if (ok == -1) {
                printf("lstat error: %s\n", dirPath);
            } else {
                memcpy(files[0], entry, sizeof(struct stat));
                strcpy(fileNames[0], dirPath);
                filesNum++;
            }
        }
    }
    

    for (int i = 0; i < filesNum; ++i) {
        printf("%s: %d ", fileNames[i], S_ISDIR(files[i]->st_mode));
    }
}