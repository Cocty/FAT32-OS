/*
    实现按名称加载虚拟磁盘并解析文件系统
    返回必要参数
    接受 虚拟磁盘名称
*/
#include "../fs.h"
#include "../tool.h"
#include <string.h>
//打开文件但未关闭
int my_load(const ARGP arg, FileSystemInfop fileSystemInfop, char **helpstr)
{
    char fileName[ARGLEN] = "fs.vhd";
    *helpstr =
        "\
功能        加载文件系统\n\
语法格式    load [namefile]\n\
namefile    虚拟磁盘文件路径（当前目录下开始） 默认 fs.vhd\n";
    FILE *fp = NULL;
    MBR mbr;
    BS_BPB bpb;

    if (arg->len == 1)
    {
        if (strcmp(arg->argv[0], "/?") == 0)
        {
            return HELP_STR;
        }
        strcpy(fileName, arg->argv[0]);
    }

    fp = fopen(fileName, "rb+");
    if (fp == NULL)
    {
        return FILE_ERROR;
    }
    memset(fileSystemInfop, 0, sizeof(FileSystemInfo));
    fileSystemInfop->flag = TRUE;
    fileSystemInfop->fp = fp;
    strcpy(fileSystemInfop->fileName, fileName);
    strcpy(fileSystemInfop->path, "/");

    //MBR部分
    DEBUG("读取MBR部分数据\n");
    do_read_block(fp, (BLOCK *)&mbr, 0, 0);
    fileSystemInfop->MBR_size = mbr.mbr_in[0].all;
    fileSystemInfop->MBR_start = mbr.mbr_in[0].strart_chan;

    DEBUG("读取DBR的数据\n");
    do_read_block(fp, (BLOCK *)&bpb, fileSystemInfop->MBR_start / 8, fileSystemInfop->MBR_start % 8);
    fileSystemInfop->BPB_BytsPerSec = bpb.BPB_BytsPerSec;
    fileSystemInfop->BPB_SecPerClus = bpb.BPB_SecPerClus;
    fileSystemInfop->BPB_RsvdSecCnt = bpb.BPB_RsvdSecCnt;
    fileSystemInfop->BPB_NumFATs = bpb.BPB_NumFATs;
    fileSystemInfop->BPB_HiddSec = bpb.BPB_HiddSec;
    fileSystemInfop->BPB_TotSec32 = bpb.BPB_TotSec32;
    fileSystemInfop->BPB_FATSz32 = bpb.BPB_FATSz32;
    fileSystemInfop->BPB_RootClis = bpb.BPB_RootClis;
    fileSystemInfop->BPB_BkBootSec = bpb.BPB_BkBootSec;

    fileSystemInfop->rootNum = fileSystemInfop->MBR_start +
                               fileSystemInfop->BPB_RsvdSecCnt +
                               fileSystemInfop->BPB_FATSz32 * fileSystemInfop->BPB_NumFATs +
                               (fileSystemInfop->BPB_RootClis - 2) * fileSystemInfop->BPB_SecPerClus;
    u32 start = fileSystemInfop->MBR_start +
                fileSystemInfop->BPB_RsvdSecCnt;
    for (u32 i = 0; i < fileSystemInfop->BPB_NumFATs; i++)
    {
        fileSystemInfop->FAT[i] = start + i * fileSystemInfop->BPB_FATSz32;
    }
    fileSystemInfop->pathNum = fileSystemInfop->BPB_RootClis;
    /* 初始化打开文件目录 */
    for (int i = 0; i < OPENFILESIZE; i++)
    {
        fileSystemInfop->Opendf[i].flag = FALSE;
        memset(fileSystemInfop->Opendf[i].File_name, ' ', 11);
    }
    if (bpb.end != 0xaa55)
    {
        fileSystemInfop->flag = FALSE;
        return WRONG_FILESYS;
    }

    DEBUG("%s 加载成功!\n", fileSystemInfop->fileName);
    DEBUG("根目录物理扇区号 %d\n", fileSystemInfop->rootNum);
    DEBUG("根目录的逻辑簇号为 %d\n", fileSystemInfop->pathNum);
    DEBUG("FAT表位置的物理扇区号 (fat1)%d (fat2)%d\n", fileSystemInfop->FAT[0], fileSystemInfop->FAT[1]);
    return SUC;
}