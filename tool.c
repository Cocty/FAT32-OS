#include "fs.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
        // DEBUG("write %x-%x\n", offset * SPCSIZE + num * BLOCKSIZE, offset * SPCSIZE + num * BLOCKSIZE + sizeof(BLOCK));
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
        return SUCCESS;
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
    return SUCCESS;
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
        return SUCCESS;
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
    return SUCCESS;
}

// 将UTF-16格式的wchar_t字符串转换为GBK格式的char字符串
char *UTF16ToGBK(const wchar_t *utf16Str)
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