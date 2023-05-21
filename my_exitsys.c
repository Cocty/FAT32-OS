#include "fs.h"
#include "tool.h"
#include <memory.h>

int my_exitsys(const ARGP arg, FileSystemInfop fileSystemInfop)
{
    const char helpstr[] =
        "\
功能        退出文件系统\n\
语法格式    exit\n";
    if (arg->len == 1)
    {
        if (strcmp(arg->argv[0], "/?") == 0)
        {
            printf(helpstr);
            return ERROR;
        }
        else
        {
            strcpy(error.msg, "参数数量错误\n\x00");
            printf("参数数量错误\n");
            return ERROR;
        }
    }

    if (fileSystemInfop->flag)
    {
        fclose(fileSystemInfop->fp);
        memset(fileSystemInfop, 0, sizeof(FileSystemInfo));
    }
    return SUCCESS;
}