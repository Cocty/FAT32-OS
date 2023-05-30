#include "fs.h"
#include "tool.h"
#include <stdio.h>
#include <stdlib.h>
const char WXML[] = "无效命令\n";
int main()
{
    char cmd[ARGLEN];
    ARG argv;
    FileSystemInfo fileSysInfo;
    fileSysInfo.flag = FALSE;
    int flag; //执行状态标志位
    char *helpstr = NULL;
    int value;
    printf("欢迎使用文件系统\n");
    my_help(&helpstr);
    printf(helpstr);
    while (TRUE)
    {
        if (fileSysInfo.flag)
        {
            printf("%s > ", fileSysInfo.path);
        }
        else
        {
            printf("> ");
        }
        scanf("%s", cmd);
        if (getargv(&argv) == ERROR)
        {
            printf(WXML);
        }
        if (strcmp(cmd, "format") == 0)
        {
            value = my_format(&argv, &helpstr);
            if (value == HELP_STR)
            {
                printf(helpstr);
            }
            else if (value == WRONG_PARANUM)
            {
                printf("参数数量错误！\n");
            }
            else if (value == WRONG_CAPACITY)
            {
                printf("磁盘容量错误！\n");
            }
            else if (value == FILE_ERROR)
            {
                printf("磁盘文件错误！\n");
            }
            else if (value == FILE_ERROR)
            {
                printf("磁盘文件错误！\n");
            }
        }
        else if (strcmp(cmd, "load") == 0)
        {
            value = my_load(&argv, &fileSysInfo, &helpstr);
            if (value == HELP_STR)
            {
                printf(helpstr);
            }
            else if (value == FILE_ERROR)
            {
                printf("磁盘文件错误！\n");
            }
            else if (value == WRONG_FILESYS)
            {
                printf("非法的文件系统！\n");
            }
        }
        else if (strcmp(cmd, "mkdir") == 0)
        {
            value = my_mkdir(&argv, &fileSysInfo, &helpstr);
            if (value == HELP_STR)
            {
                printf(helpstr);
            }
            else if (value == NULL_FILENAME)
            {
                printf("未输入目录名！\n");
            }
            else if (value == WRONG_PARANUM)
            {
                printf("参数数量错误！\n");
            }
            else if (value == NONE_FILESYS)
            {
                printf("未加载文件系统！\n");
            }
            else if (value == DIR_EXSIST)
            {
                printf("目录已存在！\n");
            }
            else if (value == NOT_ENOUGHSPACE)
            {
                printf("磁盘空间不足！\n");
            }
        }
        else if (strcmp(cmd, "cd") == 0)
        {
            value = my_cd(&argv, &fileSysInfo, &helpstr);
            if (value == HELP_STR)
            {
                printf(helpstr);
            }
            else if (value == NULL_FILENAME)
            {
                printf("未输入目录名！\n");
            }
            else if (value == WRONG_PARANUM)
            {
                printf("参数数量错误！\n");
            }
            else if (value == NONE_FILESYS)
            {
                printf("未加载文件系统！\n");
            }
            else if (value == FILE_NOTFOUND)
            {
                printf("未找到指定目录！\n");
            }
        }
        else if (strcmp(cmd, "create") == 0)
        {
            value = my_create(&argv, &fileSysInfo, &helpstr);
            if (value == HELP_STR)
            {
                printf(helpstr);
            }
            else if (value == NULL_FILENAME)
            {
                printf("未输入目录名！\n");
            }
            else if (value == WRONG_PARANUM)
            {
                printf("参数数量错误！\n");
            }
            else if (value == NONE_FILESYS)
            {
                printf("未加载文件系统！\n");
            }
            else if (value == FILE_EXSIST)
            {
                printf("文件重名！\n");
            }
            else if (value == NOT_ENOUGHSPACE)
            {
                printf("磁盘空间不足！\n");
            }
        }
        else if (strcmp(cmd, "dir") == 0 || strcmp(cmd, "ls") == 0)
        {
            value = my_dir(&argv, &fileSysInfo, &helpstr);
            if (value == HELP_STR)
            {
                printf(helpstr);
            }
            else if (value == WRONG_PARANUM)
            {
                printf("参数数量错误！\n");
            }
            else if (value == NONE_FILESYS)
            {
                printf("未加载文件系统！\n");
            }
        }
        else if (strcmp(cmd, "rm") == 0)
        {
            value = my_rm(&argv, &fileSysInfo, &helpstr);
            if (value == HELP_STR)
            {
                printf(helpstr);
            }
            else if (value == WRONG_PARANUM)
            {
                printf("参数数量错误！\n");
            }
            else if (value == NONE_FILESYS)
            {
                printf("未加载文件系统！\n");
            }
            else if (value == NULL_FILENAME)
            {
                printf("未输入文件名\n");
            }
            else if (value == FILEOPENED)
            {
                printf("文件已打开不能删除\n");
            }
            else if (value == FILE_NOTFOUND)
            {
                printf("未找到指定文件\n");
            }
        }
        else if (strcmp(cmd, "rmdir") == 0)
        {
            value = my_rmdir(&argv, &fileSysInfo, &helpstr);
            if (value == HELP_STR)
            {
                printf(helpstr);
            }
            else if (value == WRONG_PARANUM)
            {
                printf("参数数量错误！\n");
            }
            else if (value == NONE_FILESYS)
            {
                printf("未加载文件系统！\n");
            }
            else if (value == NULL_FILENAME)
            {
                printf("未输入文件名\n");
            }
            else if (value == FILEOPENED)
            {
                printf("文件已打开不能删除\n");
            }
            else if (value == FILE_NOTFOUND)
            {
                printf("未找到指定文件\n");
            }
            else if (value == DIR_NOT_NULL)
            {
                printf("目录非空\n");
            }
        }
        else if (strcmp(cmd, "open") == 0)
        {
            value = my_open(&argv, &fileSysInfo, &helpstr);
            if (value == HELP_STR)
            {
                printf(helpstr);
            }
            else if (value == WRONG_PARANUM)
            {
                printf("参数数量错误！\n");
            }
            else if (value == NONE_FILESYS)
            {
                printf("未加载文件系统！\n");
            }
            else if (value == NULL_FILENAME)
            {
                printf("未输入文件名\n");
            }
            else if (value == FILEOPENED)
            {
                printf("文件已经打开\n");
            }
            else if (value == FILE_NOTFOUND)
            {
                printf("未找到指定文件\n");
            }
            else if (value == MAX_FILE_SIGNAL)
            {
                printf("打开文件数量达到最大\n");
            }
            else
            {
                printf("%s%d\n", "打开文件成功 文件描述符是", value);
            }
        }
        else if (strcmp(cmd, "close") == 0)
        {
            value = my_close(&argv, &fileSysInfo, &helpstr);
            if (value == HELP_STR)
            {
                printf(helpstr);
            }
            else if (value == WRONG_PARANUM)
            {
                printf("参数数量错误！\n");
            }
            else if (value == NONE_FILESYS)
            {
                printf("未加载文件系统！\n");
            }
            else if (value == NULL_FILENAME)
            {
                printf("未输入文件名\n");
            }
            else if (value == FILE_NOTOPENED)
            {
                printf("文件未打开\n");
            }
            else if (value == FILE_NOTFOUND)
            {
                printf("未找到指定文件\n");
            }
        }
        else if (strcmp(cmd, "write") == 0)
        {
            value = my_write(&argv, &fileSysInfo, &helpstr);
            if (value == HELP_STR)
            {
                printf(helpstr);
            }
            else if (value == WRONG_PARANUM)
            {
                printf("参数数量错误！\n");
            }
            else if (value == NONE_FILESYS)
            {
                printf("未加载文件系统！\n");
            }
            else if (value == NULL_FILENAME)
            {
                printf("未输入文件名\n");
            }
            else if (value == WRONG_WRITE_PATTERN)
            {
                printf("写入模式非法\n");
            }
            else if (value == FILE_NOTFOUND)
            {
                printf("未找到指定文件\n");
            }
            else if (value == WRONG_COVER_POS)
            {
                printf("覆盖位置非法\n");
            }
            else if (value == FILE_NOTOPENED)
            {
                printf("文件未打开\n");
            }
        }
        else if (strcmp(cmd, "read") == 0)
        {
            char content[ARGLEN * 100];
            value = my_read(&argv, &fileSysInfo, &helpstr, content);
            if (value == HELP_STR)
            {
                printf(helpstr);
            }
            else if (value == WRONG_PARANUM)
            {
                printf("参数数量错误！\n");
            }
            else if (value == NONE_FILESYS)
            {
                printf("未加载文件系统！\n");
            }
            else if (value == NULL_FILENAME)
            {
                printf("未输入文件名\n");
            }
            else if (value == FILE_NOTFOUND)
            {
                printf("未找到指定文件\n");
            }
            else if (value == FILE_NOTOPENED)
            {
                printf("文件未打开\n");
            }
            else
            {
                for (int i = 0; i < strlen(content); i++)
                {
                    printf("%c", content[i]);
                }
                printf("\n");
            }
            content[0] = '\0';
        }
        else if (strcmp(cmd, "exit") == 0)
        {
            flag = my_exitsys(&argv, &fileSysInfo, &helpstr);
            if (flag == SUC)
            {
                break;
            }
            else if (flag == HELP_STR)
            {
                printf(helpstr);
            }
            else if (flag == WRONG_PARANUM)
            {
                printf("参数数量错误\n");
            }
        }
        else if (strcmp(cmd, "help") == 0)
        {
            my_help(&helpstr);
            printf(helpstr);
        }
        else if (strcmp(cmd, "info") == 0)
        {
            value = my_info(&argv, &fileSysInfo, &helpstr);
            if (value == HELP_STR)
            {
                printf(helpstr);
            }
            else if (value == WRONG_PARANUM)
            {
                printf("参数数量错误！\n");
            }
            else if (value == NONE_FILESYS)
            {
                printf("未加载文件系统！\n");
            }
            else if (value == WRONG_INPUT)
            {
                printf("输入错误\n");
            }
        }
        else if (strcmp(cmd, "rename") == 0)
        {
            value = my_rename(&argv, &fileSysInfo, &helpstr);
            if (value == HELP_STR)
            {
                printf(helpstr);
            }
            else if (value == WRONG_PARANUM)
            {
                printf("参数数量错误！\n");
            }
            else if (value == NONE_FILESYS)
            {
                printf("未加载文件系统！\n");
            }
            else if (value == FILE_NOTFOUND)
            {
                printf("未找到文件\n");
            }
            else if (value == NULL_FILENAME)
            {
                printf("未输入文件名\n");
            }
            else if (value == FILE_EXSIST)
            {
                printf("重命名之后的文件重名\n");
            }
        }
        else
        {
            printf(WXML);
        }
    }

    return 0;
}