#include "fs.h"
#include "tool.h"
int my_info(const ARGP arg, FileSystemInfop fileSystemInfop, char **helpstr)
{
    *helpstr =
        "\
功能        显示文件系统信息\n\
语法格式    info item offset\n\
item       要显示的信息\n\
备注       如果item为fat，则显示fat表的占用情况，默认读取一个扇区\n\
            如果item为detail,则显示当前已占用多少簇，剩余有多少簇\n\
            默认显示空间使用情况";
    char item[ARGLEN];
    FAT fat;
    const int str_fat1 = fileSystemInfop->BPB_RsvdSecCnt + fileSystemInfop->MBR_start; //Fat表1起始扇区数
    if (fileSystemInfop->flag == FALSE)
    {
        return NONE_FILESYS;
    }
    if (strcmp(arg->argv[0], "fat") == 0)
    {
        int len;
        if (arg->len == 1)
        {
            len = 1; //默认读取fat表的第一个扇区
        }
        else if (arg->len == 2)
        {
            len = ctoi(arg->argv[1]);
            if (len > fileSystemInfop->BPB_FATSz32 || len == INF)
            {
                return WRONG_INPUT;
            }
        }
        else
        {
            return WRONG_PARANUM;
        }
        for (int i = 0; i < len; i++)
        {
            do_read_block(fileSystemInfop->fp, (BLOCK *)&fat, (fileSystemInfop->FAT[0] + i) / 8, (fileSystemInfop->FAT[0] + i) % 8);
            int j;
            for (j = 0; j < BLOCKSIZE / 4; j++) //遍历FAT一个扇区的每个表项
            {
                printf("%08x ", BigtoLittle32(fat.fat[j]));
                if ((j + 1) % 4 == 0)
                {
                    printf("\n");
                }
            }
        }
    }
    else if (strcmp(arg->argv[0], "detail") == 0 || arg->len == 0)
    {
        int all_sec = fileSystemInfop->BPB_TotSec32;      //该分区总的扇区数
        int all_clus = fileSystemInfop->BPB_TotSec32 / 8; //该分区总的簇数
        int rest_sec;                                     //剩余的扇区数
        int rest_clus = 0;
        FAT fat;
        for (int i = 0; i < fileSystemInfop->BPB_FATSz32; i++) //遍历fat表所有扇区
        {
            do_read_block(fileSystemInfop->fp, (BLOCK *)&fat, (fileSystemInfop->FAT[0] + i) / 8, (fileSystemInfop->FAT[0] + i) % 8);
            for (int j = 0; j < BLOCKSIZE / 4; j++) //遍历fat表某一扇区的所有表项
            {
                if (fat.fat[j] == 0x0000) //如果有未分配的
                {
                    rest_clus++;
                }
            }
        }
        rest_sec = rest_clus * fileSystemInfop->BPB_SecPerClus;
        printf("该分区总容量为%10dMB\n", all_sec * fileSystemInfop->BPB_BytsPerSec / 1024 / 1024);
        printf("该分区的总扇区数为%10d\n", all_sec);
        printf("该分区的剩余扇区数为%10d\n", rest_sec);
        printf("该分区的总簇数为%10d\n", all_clus);
        printf("该分区的剩余簇数为%10d\n", rest_clus);
        printf("空间占用率为%.2f\n", (float)(all_sec - rest_sec) / all_sec);
    }
}