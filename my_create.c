#include "fs.h"
#include "tool.h"
#include <memory.h>
#include <ctype.h>

int my_create(const ARGP arg, FileSystemInfop fileSystemInfop)
{
    const char helpstr[] =
        "\
功能        创建文件\n\
语法格式    create name\n\
name       创建文件的名字\n\
备注       文件名强制转为大写，文件名要符合三八命名方式\n";
    char name[ARGLEN];
    FAT_DS_BLOCK4K fat_ds;
    if (fileSystemInfop->flag == FALSE)
    {
        strcpy(error.msg, "未指定文件系统\n\x00");
        printf("未指定文件系统\n");
        return ERROR;
    }
    if (arg->len == 1)
    {
        if (strcmp(arg->argv[0], "/?") == 0)
        {
            printf(helpstr);
            return SUCCESS;
        }
        else
        {
            if (nameCheckChange(arg->argv[0], name) == ERROR)
            {
                strcpy(error.msg, "文件名过长或存在非法字符\n\x00");
                printf("文件名过长或存在非法字符\n");
                return ERROR;
            }
            for (int i = 0; i < 11; i++)
            {
                name[i] = toupper(name[i]);
            }
            name[11] = '\0';
        }
    }
    else if (arg->len == 0)
    {
        DEBUG("文件名空\n");
        return ERROR;
    }
    else
    {
        strcpy(error.msg, "参数数量错误\n\x00");
        printf("参数数量错误\n");
        return ERROR;
    }

    u32 pathNum = fileSystemInfop->pathNum;
    u32 cut;
    while (TRUE)
    {

        do_read_block4k(fileSystemInfop->fp, (BLOCK4K *)&fat_ds, L2R(fileSystemInfop, pathNum)); //读取当前簇号所在簇的内容
        for (cut = 0; cut < SPCSIZE / 32; cut++)                                                 //遍历该簇下的每个目录项
        {
            char lin[12];
            strncpy(lin, fat_ds.fat[cut].name, 11);
            lin[11] = '\0';
            if (fat_ds.fat[cut].name[0] == '\x00') //说明该目录项空闲还没有分配
            {
                break;
            }
            else if (strcmp(name, lin) == 0)
            {
                strcpy(error.msg, "文件已存在\n\x00");
                printf("文件已存在\n");
                return ERROR;
            }
        }
        //取得下一个簇
        if (cut == SPCSIZE / 32)
        {
            u32 lin = pathNum;
            pathNum = getNext(fileSystemInfop, pathNum);
            if (pathNum == FAT_END)
            {
                //全都没有分配一个
                pathNum = newfree(fileSystemInfop, lin);
                if (pathNum == 0)
                {
                    strcpy(error.msg, "磁盘空间不足\n\x00");
                    printf("磁盘空间不足\n");
                    return ERROR;
                }
            }
        }
        else
        {
            u32 pathnumd = newfree(fileSystemInfop, 0); //分配一个新簇，且不连接
            memset(&fat_ds.fat[cut], 0, sizeof(FAT_DS));

            //开始写目录项
            strncpy(fat_ds.fat[cut].name, name, 11);
            fat_ds.fat[cut].DIR_Attr = ATTR_ARCHIVE;
            fat_ds.fat[cut].DIR_FstClusHI = (u16)(pathnumd >> 16);
            fat_ds.fat[cut].DIR_FstClusLO = (u16)(pathnumd & 0x0000ffff);

            //写入新建文件
            do_write_block4k(fileSystemInfop->fp, (BLOCK4K *)&fat_ds, L2R(fileSystemInfop, pathNum));
            DEBUG("创建成功\n");
            break;
        }
    }
    return SUCCESS;
}