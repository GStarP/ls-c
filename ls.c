#include <string.h>
#include <stdio.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <fcntl.h>

#define MAX_FILE_NAME 50
#define MAX_DIR_FILES 20
#define MAX_ATTR_NAME 30
#define R 0
#define D 1
#define A 2
#define I 3
#define L 4

void chooseFile(char* des, char* name, long ino, struct stat* file, int* opts);
void printFile(char* name, long ino, struct stat* file, int* opts);
void swapFile(struct stat* s1, struct stat* s2) {
    int s = sizeof(struct stat);
    struct stat* tmp = malloc(s);
    memcpy(tmp, s1, s);
    memcpy(s1, s2, s);
    memcpy(s2, tmp, s);
    free(tmp);
}
void swapName(char* s1, char* s2) {
    char tmp[MAX_FILE_NAME];
    strcpy(tmp, s1);
    strcpy(s1, s2);
    strcpy(s2, tmp);
}
void getSupDir(char* path, char* des) {
    strcpy(des, path);
    for (int i = strlen(des) - 1; i >= 0; --i) {
        if (des[i] == '/') {
            des[i] = '\0';
            break;
        } else {
            des[i] = '\0';
        }
    }
}

void main(int argc, char* argv[]) {
    // 初始化参数列表
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
    long fileInos[MAX_DIR_FILES];
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
                    // 切换工作目录，方便 lstat 使用相对路径
                    chdir(dirPath);
                    ok = lstat(curFile->d_name, entry);
                    if (ok == -1) {
                        printf("lstat error: %s\n", curFile->d_name);
                    } else {
                        // 文件名无法保存在 stat 中，因此用一个数组单独保存
                        strcpy(fileNames[file_idx], curFile->d_name);
                        fileInos[file_idx] = curFile->d_ino;
                        // 拷贝文件信息结构体
                        memcpy(files[file_idx], entry, sizeof(struct stat));
                        file_idx++;
                        filesNum++;
                    }
                }
                free(curFile);
            }
            closedir(dir);
            
            // 根据对 ls 的测试，输出应按照文件名字典序排序
            // . ..
            for (int i = 0; i < filesNum; ++i) {
                if (strcmp(fileNames[i], ".") == 0 && i != 0) {
                    swapFile(files[i], files[0]);
                    swapName(fileNames[i], fileNames[0]);
                    long tmp = fileInos[i];
                    fileInos[i] = fileInos[0];
                    fileInos[0] = fileInos[i];
                }
                if (strcmp(fileNames[i], "..") == 0 && i != 1) {
                    swapFile(files[i], files[1]);
                    swapName(fileNames[i], fileNames[1]);
                    long tmp = fileInos[i];
                    fileInos[i] = fileInos[1];
                    fileInos[1] = fileInos[i];
                }
            }
            //
            for (int i = 2; i < filesNum; ++i) {
                int idx = i;
                for (int j = i + 1; j < filesNum; ++j) {
                    int bias1 = fileNames[idx][0] == '.' ? 1 : 0;
                    int bias2 = fileNames[j][0] == '.' ? 1 : 0;
                    if (strcmp(fileNames[idx] + bias1, fileNames[j] + bias2) > 0) {
                        idx = j;
                    }
                }
                if (idx != i) {
                    swapFile(files[idx], files[i]);
                    swapName(fileNames[idx], fileNames[i]);
                    long tmp = fileInos[i];
                    fileInos[i] = fileInos[idx];
                    fileInos[idx] = tmp;
                }
            }

            // 根据参数 r 决定输出顺序
            if (opts[R] == 1) {
                for (int i = filesNum - 1; i >= 0; --i) {
                    chooseFile(dirPath, fileNames[i], fileInos[i], files[i], opts);
                }
            } else {
                for (int i = 0; i < filesNum; ++i) {
                    chooseFile(dirPath, fileNames[i], fileInos[i], files[i], opts);
                }
            }
        // 当目标路径是文件时，直接读取并存储
        } else {
            ok = lstat(dirPath, entry);
            if (ok == -1) {
                printf("lstat error: %s\n", dirPath);
            } else {
                // 通过读取父目录来获取单个文件的 inode 号
                char supDirPath[MAX_FILE_NAME];
                getSupDir(dirPath, supDirPath);
                DIR* superDir = opendir(supDirPath);
                if (superDir == NULL) printf("read super dir error");
                else {
                    struct dirent* curFile = (struct dirent*) malloc(sizeof(struct dirent));
                    while((curFile = readdir(superDir)) != NULL) {
                        // 通过文件名来判断是否为此文件
                        int match = 1;
                        for (int i = 0; i < strlen(curFile->d_name); ++i) {
                            if (curFile->d_name[i] != dirPath[strlen(dirPath) - strlen(curFile->d_name) + i]) {
                                match = 0;
                            }
                        }
                        // 防止 a 和 test/spa 这种情况的干扰
                        if (match == 1 && dirPath[strlen(dirPath) - strlen(curFile->d_name) - 1] == '/') {
                            chooseFile(dirPath, dirPath, curFile->d_ino, entry, opts);
                        }
                    }
                    free(curFile);
                }
            }
        }

        if (opts[L] != 1) {
            printf("\n");
        }
        free(files);
        free(entry);
    }
}

// 判断文件类型
char getFileType(mode_t m) {
    if (S_ISREG(m)) {
        return '-';
    } else if (S_ISDIR(m)) {
        return 'd';
    } else if (S_ISCHR(m)) {
        return 'c';
    } else if (S_ISBLK(m)) {
        return 'b';
    } else if (S_ISFIFO(m)) {
        return 'p';
    } else if (S_ISLNK(m)) {
        return 'l';
    // no S_ISSOCK
    } else {
        printf("unknown file type\n");
        return '?';
    }
}

// 打印文件权限
void printFileAuth(mode_t m) {
    char auth[10];
    auth[0] = m & S_IRUSR ? 'r' : '-';
    auth[1] = m & S_IWUSR ? 'w' : '-';
    auth[2] = m & S_IXUSR ? 'x' : '-';
    auth[3] = m & S_IRGRP ? 'r' : '-';
    auth[4] = m & S_IWGRP ? 'w' : '-';
    auth[5] = m & S_IXGRP ? 'x' : '-';
    auth[6] = m & S_IROTH ? 'r' : '-';
    auth[7] = m & S_IWOTH ? 'w' : '-';
    auth[8] = m & S_IXOTH ? 'x' : '-';
    auth[9] = '\0';
    printf("%s", auth);
}

// 打印用户名
void printFileAuthor(uid_t u) {
    struct passwd* pwd;
    pwd = getpwuid(u);
    printf("%s", pwd->pw_name);
}

// 打印组名
void printFileGroup(gid_t g) {
    struct group* grp;
    grp = getgrgid(g);
    printf("%s", grp->gr_name);
}

// 打印最后修改时间
void printFileModTime(time_t t) {
    struct tm* tm = localtime(&t);
    printf("%d月  %2d %02d:%02d", tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min);
}

// 目录字体为蓝色加粗
void printName(mode_t m, char* name) {
    if (S_ISDIR(m) == 1) {
        printf("\033[1;34m%s\033[0m", name);
    } else {
        printf("%s", name);
    }
}

// 根据参数 d 和 a 选择需要输出的文件
void chooseFile(char* des, char* name, long ino, struct stat* file, int* opts) {
    // 经过对 ls 的测试发现：优先级 d > a
    if (opts[D] == 1) {
        // d：只输出目标路径文件
        if (strcmp(name, ".") == 0 || strcmp(des, name) == 0) {
            printFile(des, ino, file, opts);
        }
    } else if (opts[A] == 1) {
        // a：输出全部文件
        printFile(name, ino, file, opts);
    } else {
        // 默认：不输出开头为 . 的文件
        if (name[0] != '.') {
            printFile(name, ino, file, opts);
        }
    }
}

// 根据参数 l 和 i 选择需要输出的信息
void printFile(char* name, long ino, struct stat* file, int* opts) {
    // 经过对 ls 的测试发现：优先级 l > i
    if (opts[L] == 1) {
        // 文件类型
        printf("%c", getFileType(file->st_mode));
        // 文件权限
        printFileAuth(file->st_mode);
        // 硬链接数
        printf(" %lu ", file->st_nlink);
        // 所属用户和组
        printFileAuthor(file->st_uid);
        printf(" ");
        printFileGroup(file->st_gid);
        // 文件大小
        printf("%5ld ", file->st_size);
        // 最后修改时间
        printFileModTime(file->st_mtime);
        printf(" ");
        // 文件名
        printName(file->st_mode, name);
        // 由于不做要求，这里不处理链接文件
        if (S_ISLNK(file->st_mode)) {}
        printf("\n");
    } else if (opts[I] == 1) {
        // inode 号
        printf("%ld ", ino);
        printName(file->st_mode, name);
        printf("  ");
    } else {
        // 默认只打印文件名
        printName(file->st_mode, name);
        printf("  ");
    }
}
