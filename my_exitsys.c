#include "fs.h"
#include "tool.h"

int my_exitsys(const ARGP arg, FileSystemInfop fileSystemInfop, char **helpstr)
{
    *helpstr =
        "\
功能        退出文件系统\n\
语法格式    exit\n";
    if (arg->len == 1)
    {
        if (strcmp(arg->argv[0], "/?") == 0)
        {
            return HELP_STR;
        }
        else
        {
            return WRONG_PARANUM;
        }
    }

    if (fileSystemInfop->flag)
    {
        fclose(fileSystemInfop->fp);
        memset(fileSystemInfop, 0, sizeof(FileSystemInfo));
    }
    return SUC;
}