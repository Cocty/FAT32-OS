#include "tool.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

int do_write_block4k(FILE *fp, BLOCK4K *block4k, int offset)
{
    int ret = -1;
    if (offset == -1)
    {
        ret = fwrite(block4k, sizeof(BLOCK4K), 1, fp);
    }
    else
    {
        fseek(fp, offset * SPCSIZE, SEEK_SET);
        ret = fwrite(block4k, sizeof(BLOCK4K), 1, fp);
    }
    return ret;
}
int do_write_block(FILE *fp, BLOCK *block, int offset, int num)
{
    int ret = -1;
    if (offset == -1)
    {
        ret = fwrite(block, sizeof(BLOCK), 1, fp);
    }
    else
    {
        fseek(fp, offset * SPCSIZE + num * BLOCKSIZE, SEEK_SET);
        ret = fwrite(block, sizeof(BLOCK), 1, fp);
    }
    return ret;
}

int do_read_block4k(FILE *fp, BLOCK4K *block4k, int offset)
{

    int ret = -1;
    if (offset == -1)
    {
        ret = fread(block4k, sizeof(BLOCK4K), 1, fp);
    }
    else
    {
        DEBUG("read4k 物理簇号%d 范围%x-%x\n", offset, offset * SPCSIZE, offset * SPCSIZE + sizeof(BLOCK4K));
        fseek(fp, offset * SPCSIZE, SEEK_SET);
        ret = fread(block4k, sizeof(BLOCK4K), 1, fp);
    }
    return ret;
}
int do_read_block(FILE *fp, BLOCK *block, int offset, int num)
{

    int ret = -1;
    if (offset == -1)
    {
        ret = fread(block, sizeof(BLOCK), 1, fp);
    }
    else
    {
        DEBUG("read %x-%x\n", offset * SPCSIZE + num * BLOCKSIZE, offset * SPCSIZE + num * BLOCKSIZE + sizeof(BLOCK));
        fseek(fp, offset * SPCSIZE + num * BLOCKSIZE, SEEK_SET);
        ret = fread(block, sizeof(BLOCK), 1, fp);
    }
    return ret;
}

u32 L2R(FileSystemInfop fsip, u32 num)
{
    return (fsip->MBR_start + fsip->BPB_RsvdSecCnt + fsip->BPB_FATSz32 * fsip->BPB_NumFATs) / fsip->BPB_SecPerClus + num - 2;
}

u32 getNext(FileSystemInfop fsip, u32 num)
{
    if (num / (512 / 4) > fsip->BPB_FATSz32)
    {
        return 0;
    }
    u32 cuNum = num / (512 / 4);
    u32 index = num % (512 / 4);
    FAT fat;
    do_read_block(fsip->fp, (BLOCK *)&fat, (fsip->FAT[0] + cuNum) / 8, (fsip->FAT[0] + cuNum) % 8);
    return fat.fat[index];
}

int newfree(FileSystemInfop fsip, u32 num)
{

    FAT fat;
    u32 cuNum = num / (BLOCKSIZE / 4);
    u32 index = num % (BLOCKSIZE / 4);
    for (u32 i = 0; i < fsip->BPB_FATSz32; i++) //遍历fat表
    {
        do_read_block(fsip->fp, (BLOCK *)&fat, (fsip->FAT[0] + i) / 8, (fsip->FAT[0] + i) % 8);
        for (int j = 0; j < BLOCKSIZE / 4; j++) // 遍历FAT一个扇区的每个表项
        {
            if (fat.fat[j] == FAT_FREE)
            {
                fat.fat[j] = FAT_END;
                for (int k = 0; k < fsip->BPB_NumFATs; k++)
                {
                    do_write_block(fsip->fp, (BLOCK *)&fat, (fsip->FAT[k] + i) / 8, (fsip->FAT[k] + i) % 8);
                }
                if (num != 0) //当num不为0时，连接簇号为num的下一个簇为i*(512/4)+j
                {
                    do_read_block(fsip->fp, (BLOCK *)&fat, (fsip->FAT[0] + cuNum) / 8, (fsip->FAT[0] + cuNum) % 8);
                    fat.fat[index] = i * (512 / 4) + j;
                    for (int k = 0; k < fsip->BPB_NumFATs; k++)
                    {
                        do_write_block(fsip->fp, (BLOCK *)&fat, (fsip->FAT[k] + cuNum) / 8, (fsip->FAT[k] + cuNum) % 8);
                    }
                }
                BLOCK4K block4k;
                memset(&block4k, 0, SPCSIZE); //当num为0时，只分配不连接
                do_write_block4k(fsip->fp, &block4k, L2R(fsip, i * (512 / 4) + j));
                DEBUG("new link %d->%d %d %d\n", num, i * (512 / 4) + j, i, j);
                return i * (512 / 4) + j;
            }
        }
    }
    return 0;
}

int delfree(FileSystemInfop fsip, u32 num)
{
    FAT fat;
    u32 cuNum = num / (512 / 4);
    u32 index = num % (512 / 4);
    do_read_block(fsip->fp, (BLOCK *)&fat, (fsip->FAT[0] + cuNum) / 8, (fsip->FAT[0] + cuNum) % 8);
    u32 ret = fat.fat[index];
    fat.fat[index] = FAT_FREE;
    for (int i = 0; i < fsip->BPB_NumFATs; i++) //写回到fat表
    {
        do_write_block(fsip->fp, (BLOCK *)&fat, (fsip->FAT[i] + cuNum) / 8, (fsip->FAT[i] + cuNum) % 8);
    }
    return ret;
}

// 取得命令参数
int getargv(ARGP argp)
{
    char cmd[ARGLEN * 10];
    if (gets(cmd) == NULL)
    {
        argp->len = 0;
        return SUC;
    }
    int num = 0;
    int len = 0;
    int flag = 0;
    for (u32 i = 0; i < strlen(cmd); i++)
    {
        if (len >= ARGLEN)
        {
            return ERROR;
        }
        if (num >= ARGNUM)
        {
            return ERROR;
        }
        if (cmd[i] == ' ' || cmd[i] == '\t' || cmd[i] == '\r')
        {
            flag = 0;
            continue;
        }
        if (flag == 0)
        {
            num++;
            len = 0;
        }
        (argp->argv)[num - 1][len] = cmd[i];
        (argp->argv)[num - 1][len + 1] = 0;
        len++;
        flag = 1;
    }
    argp->len = num;
    return SUC;
}

// 数字字符串转换为int类型数字,如果非法返回INF，不考虑溢出
int ctoi(const char *ch)
{
    int ret = 0;
    for (u32 i = 0; i < strlen(ch); i++)
    {
        ret *= 10;
        if ('0' <= ch[i] && ch[i] <= '9')
        {
            ret += ch[i] - '0';
        }
        else
        {
            return INF;
        }
    }
    return ret;
}

int nameCheckChange(const char name[ARGLEN], char name38[12])
{
    if (strlen(name) > 12 || strlen(name) <= 0)
    {
        return ERROR;
    }
    int point = -1;
    for (u32 i = 0; i < strlen(name); i++)
    {
        if (name[i] == '.')
        {
            point = i;
            continue;
        }
        // if (!(isalnum(name[i]) || isalpha(name[i]) || isspace(name[i]) || name[i] == '$' || name[i] == '%' || name[i] == '\'' || name[i] == '-' || name[i] == '_' || name[i] == '@' || name[i] == '~' || name[i] == '`' || name[i] == '!' || name[i] == '(' || name[i] == ')' || name[i] == '{' || name[i] == '}' || name[i] == '^' || name[i] == '#' || name[i] == '&'))
        // {
        //     return ERROR;
        // }
    }
    if (((point != -1) && (point <= 8 && strlen(name) - point - 1 <= 3)) || (point == -1 && strlen(name) <= 8))
    {
        memset(name38, ' ', 11);
        name38[11] = '\0';
        if (point == 0)
        {
            return ERROR;
        }
        else if (point != -1)
        {
            for (int i = 0; i < point; i++)
            {
                name38[i] = name[i];
            }
            for (int i = point + 1; i < (int)strlen(name); i++)
            {
                name38[i - point + 8 - 1] = name[i];
            }
        }
        else
        {
            for (int i = 0; i < (int)strlen(name); i++)
            {
                name38[i] = name[i];
            }
        }
        return SUC;
    }
    return ERROR;
}

int nameCheck(char name[ARGLEN])
{
    if (strlen(name) > 11 || strlen(name) <= 0)
    {
        return ERROR;
    }
    for (int i = strlen(name); i < 11; i++)
    {
        name[i] = ' ';
    }
    name[11] = '\x0';
    return SUC;
}

// 将UTF-16格式的wchar_t字符串转换为GBK格式的char字符串
char *UTF16ToGBK(wchar_t *utf16Str)
{
    int gbkLength = WideCharToMultiByte(CP_ACP, 0, utf16Str, -1, NULL, 0, NULL, NULL);
    char *gbkStr = (char *)malloc((gbkLength + 1) * sizeof(char));
    WideCharToMultiByte(CP_ACP, 0, utf16Str, -1, gbkStr, gbkLength, NULL, NULL);
    return gbkStr;
}

// 将GBK格式的char字符串转换为UTF-16格式的wchar_t字符串
wchar_t *GBKToUTF16(const char *gbkStr)
{
    int utf16Length = MultiByteToWideChar(CP_ACP, 0, gbkStr, -1, NULL, 0);
    wchar_t *utf16Str = (wchar_t *)malloc((utf16Length + 1) * sizeof(wchar_t));
    MultiByteToWideChar(CP_ACP, 0, gbkStr, -1, utf16Str, utf16Length);
    return utf16Str;
}
void parsename(char *filename, char *new_filename)
{
    int dot_pos = -1;
    for (int i = 0; i < strlen(filename); i++)
    {
        if (filename[i] == '.')
        {
            dot_pos = i;
            break;
        }
    }
    if (dot_pos != -1) //文件
    {
        for (int i = 0; i < 6; i++)
        {
            new_filename[i] = filename[i];
        }
        new_filename[6] = '~';
        new_filename[7] = '1';
        for (int i = dot_pos + 1, j = 8; j < 11; i++, j++)
        {
            new_filename[j] = filename[i];
        }
    }
    else
    {
        for (int i = 0; i < 11; i++)
        {
            new_filename[i] = filename[i];
        }
    }
    /*大写*/
    for (int i = 0; i < 11; i++)
    {
        new_filename[i] = toupper(new_filename[i]);
    }
}

int debug_in(char *format, ...)
{
    return 0;
}

int Is_repeat(char *name, FileSystemInfop fileSystemInfop, FAT_DS_BLOCK4K fat_ds)
{
    u32 pathNum = fileSystemInfop->pathNum;
    u32 cut;
    do
    {
        do_read_block4k(fileSystemInfop->fp, (BLOCK4K *)&fat_ds, L2R(fileSystemInfop, pathNum)); //读取当前簇
        cut = 0;
        while (cut < SPCSIZE / 32) // 遍历当前簇的目录项
        {
            if (fat_ds.fat[cut].name[0] == '\xe5')
            {
                //被删除的
                cut++;
                continue;
            }
            char lin[12];
            strncpy(lin, fat_ds.fat[cut].name, 11);
            lin[11] = '\0';

            if (strcmp(lin, DIR_d) == 0 || strcmp(lin, DIR_dd) == 0) //跳过父目录和此目录，即跳过..与.
            {
                cut++;
                continue;
            }
            if (fat_ds.fat[cut].DIR_Attr == ATTR_LONG_NAME) //表示是长文件名目录项
            {
                Longfile_termp long_file_term = (Longfile_termp)&fat_ds.fat[cut];
                wchar_t new_filename[255];
                u8 length;
                u8 bit = GET_BIT(long_file_term->LDIR_Ord, 6);
                if (bit == 1) //判断高半位是否为4，即长文件名目录项开始处
                {
                    length = (u8)long_file_term->LDIR_Ord & 0x0f; //获取低4位的值，以确定分配多少目录项
                }
                int index = 0;
                for (int i = length - 1; i >= 0; i--)
                {
                    for (int j = 1; j >= 0; j--)
                    {
                        if (long_file_term->LDIR_Name3[j] != 0xFFFF)
                        {
                            new_filename[index++] = (unsigned short)long_file_term->LDIR_Name3[j];
                        }
                    }
                    for (int j = 5; j >= 0; j--)
                    {
                        if (long_file_term->LDIR_Name2[j] != 0xFFFF)
                        {
                            new_filename[index++] = (unsigned short)long_file_term->LDIR_Name2[j];
                        }
                    }

                    for (int j = 4; j >= 0; j--)
                    {
                        if (long_file_term->LDIR_Name1[j] != 0xFFFF)
                        {
                            new_filename[index++] = (unsigned short)long_file_term->LDIR_Name1[j];
                        }
                    }

                    long_file_term++; // 指针向后移动，即获取下一个长文件名目录项
                }
                long_file_term = NULL;
                new_filename[index++] = L'\0';
                reverseString(new_filename, wcslen(new_filename));

                char *filename = UTF16ToGBK(new_filename); //UTF16转化成GBK格式以显示中文

                cut += length;                               //此时现在指针指向了紧挨着的短文件名目录项
                if (fat_ds.fat[cut].DIR_Attr & ATTR_ARCHIVE) //是文件
                {
                    if (strcmp(filename, name) == 0) //说明有重复的
                    {
                        printf("文件已存在！\n");
                        return ERROR;
                    }
                }
                else if (fat_ds.fat[cut].DIR_Attr & ATTR_DIRECTORY) //是目录
                {
                    if (strcmp(filename, name) == 0) //说明有重复的
                    {
                        printf("文件夹已存在！\n");
                        return ERROR;
                    }
                }
                cut++;
            }
            cut++;
        }
        pathNum = getNext(fileSystemInfop, pathNum); //取下一个簇
    } while (pathNum != FAT_END);
    return SUC;
}
void setCreationTime(FAT_DSp file)
{
    time_t create;
    time_t current_time = time(&create);
    struct tm *tm_info = localtime(&current_time);

    int year = tm_info->tm_year + 1900;
    int month = tm_info->tm_mon + 1;
    int day = tm_info->tm_mday;
    int hour = tm_info->tm_hour;
    int minute = tm_info->tm_min;
    int second = tm_info->tm_sec;
    int milliseconds = 0; // 假设毫秒部分为0

    // 设置创建时间和日期
    file->DIR_CrtTime = ((hour & 0x1F) << 11) | ((minute & 0x3F) << 5) | ((second & 0x1F) >> 1);
    file->DIR_CrtDate = ((year - 1980) << 9) | (month << 5) | day;
    file->DIR_CrtTimeTeenth = (milliseconds / 10) & 0x1F; //10毫秒位

    // 设置最近修改时间和日期
    file->DIR_WriTime = file->DIR_CrtTime;
    file->DIR_WrtDate = file->DIR_CrtDate;

    // 设置最后访问日期
    file->DIR_LastAccDate = file->DIR_CrtDate;
}

void setLastWriteTime(FAT_DSp file)
{
    time_t write;
    time_t current_time = time(&write);
    struct tm *tm_info = localtime(&current_time);

    int year = tm_info->tm_year + 1900;
    int month = tm_info->tm_mon + 1;
    int day = tm_info->tm_mday;
    int hour = tm_info->tm_hour;
    int minute = tm_info->tm_min;
    int second = tm_info->tm_sec;

    // 设置最近修改时间和日期
    file->DIR_WriTime = ((hour & 0x1F) << 11) | ((minute & 0x3F) << 5) | ((second & 0x1F) >> 1);
    file->DIR_WrtDate = ((year - 1980) << 9) | (month << 5) | day;
}

void setLastAccessTime(FAT_DSp file)
{
    time_t access;
    time_t current_time = time(&access);
    struct tm *tm_info = localtime(&current_time);
    int year = tm_info->tm_year + 1900;
    int month = tm_info->tm_mon + 1;
    int day = tm_info->tm_mday;
    int hour = tm_info->tm_hour;
    int minute = tm_info->tm_min;
    int second = tm_info->tm_sec;
    // 设置最后访问日期
    file->DIR_LastAccDate = ((year - 1980) << 9) | (month << 5) | day;
}
