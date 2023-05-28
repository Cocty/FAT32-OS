#include "fs.h"
#include "tool.h"

int my_write(const ARGP arg, FileSystemInfop fileSystemInfop, char **helpstr)
{
    *helpstr =
        "\
功能        写入当前目录下的某个文件，以文件尾为结束\n\
语法格式    write name type [offset]\n\
name        写入文件名\n\
type        写入模式0截断 1追加 2覆盖\n\
默认是截断写\n\
若是覆盖写则offset有效，为覆盖的起始位置\n ";
    FAT_DS_BLOCK4K fat_ds;
    char name[ARGLEN];
    int type = 0;
    u32 offset = 0;
    if (fileSystemInfop->flag == FALSE)
    {
        return NONE_FILESYS;
    }

    if (arg->len == 3)
    {
        offset = ctoi(arg->argv[2]);
        type = ctoi(arg->argv[1]);
        if (type != 2 && offset == INF)
        {
            return WRONG_WRITE_PATTERN;
        }
    }
    else if (arg->len == 2)
    {
        type = ctoi(arg->argv[1]);
        if (type != 0 && type != 1)
        {
            return WRONG_WRITE_PATTERN;
        }
    }
    else if (arg->len == 1)
    {
        if (strcmp(arg->argv[0], "/?") == 0)
        {
            return HELP_STR;
        }
    }
    else if (arg->len == 0)
    {
        return NULL_FILENAME;
    }
    else
    {
        return WRONG_PARANUM;
    }

    if (nameCheckChange(arg->argv[0], name) == SUC)
    {
        return write_sfn(fileSystemInfop, name, fat_ds, type, offset);
    }
    else
    {
        strcpy(name, arg->argv[0]);
        return write_lfn(fileSystemInfop, name, fat_ds, type, offset);
    }
}

int write_sfn(FileSystemInfop fileSystemInfop, char *name, FAT_DS_BLOCK4K fat_ds, int type, int offset)
{
    u32 pathNum = fileSystemInfop->pathNum;
    Opendfilep opendf;
    u32 cut;
    do
    {
        do_read_block4k(fileSystemInfop->fp, (BLOCK4K *)&fat_ds, L2R(fileSystemInfop, pathNum));
        for (cut = 0; cut < SPCSIZE / 32; cut++)
        {
            char lin[12];
            strncpy(lin, fat_ds.fat[cut].name, 11);
            lin[11] = '\0';
            if (fat_ds.fat[cut].name[0] == '\xE5' || fat_ds.fat[cut].name[0] == '\x00')
            {
                continue;
            }
            else if (strcmp(lin, name) == 0 && (fat_ds.fat[cut].DIR_Attr & ATTR_ARCHIVE) != 0)
            {
                for (int i = 0; i < OPENFILESIZE; i++)
                {
                    opendf = &(fileSystemInfop->Opendf[i]);
                    if (pathNum == opendf->Dir_Clus && opendf->flag == TRUE && strcmp(opendf->File_name, name) == 0)
                    {
                        if (offset > fat_ds.fat[cut].DIR_FileSize)
                        {
                            return WRONG_COVER_POS;
                        }
                        int num = 0;
                        char buf[ARGLEN * 10];
                        int first = 0;
                        int writelen = 0;
                        while (scanf("%c", &buf[num]) != EOF && buf[num] != 26)
                        {
                            num++;
                            if (num >= ARGLEN * 10)
                            {
                                first++;
                                writelen += write_in(i, type, offset + writelen, num, (void *)buf, fileSystemInfop);
                                num = 0;
                            }
                        }
                        clearerr(stdin);
                        write_in(i, type, offset + writelen, num, (void *)buf, fileSystemInfop);

                        return SUC;
                    }
                }
                return FILE_NOTOPENED;
            }
        }
        pathNum = getNext(fileSystemInfop, pathNum);
    } while (pathNum != 0 && pathNum != FAT_END);
    return FILE_NOTFOUND;
}

int write_lfn(FileSystemInfop fileSystemInfop, char *name, FAT_DS_BLOCK4K fat_ds, int type, int offset)
{
    u32 pathNum = fileSystemInfop->pathNum;
    Opendfilep opendf;
    u32 cut;
    do
    {
        do_read_block4k(fileSystemInfop->fp, (BLOCK4K *)&fat_ds, L2R(fileSystemInfop, pathNum)); //读取当前簇号所在的物理簇
        cut = 0;
        while (cut < SPCSIZE / 32)
        { //遍历每个目录项
            char lin[12];
            strncpy(lin, fat_ds.fat[cut].name, 11);
            lin[11] = '\0';
            if (fat_ds.fat[cut].name[0] == '\xE5' || fat_ds.fat[cut].name[0] == '\x00')
            {
                continue;
            }

            if (fat_ds.fat[cut].DIR_Attr == ATTR_LONG_NAME) //表示是长文件名目录项
            {
                //开始获取长文件名
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
                new_filename[index++] = L'\0';
                reverseString(new_filename, wcslen(new_filename));
                char *filename = UTF16ToGBK(new_filename); //UTF16转化成GBK格式以显示中文
                //获取长文件名完成

                cut += length; //此时现在指针指向了紧挨着的短文件名目录项

                if (strcmp(filename, name) == 0 && (fat_ds.fat[cut].DIR_Attr & ATTR_ARCHIVE))
                {
                    for (int i = 0; i < OPENFILESIZE; i++)
                    {
                        opendf = &(fileSystemInfop->Opendf[i]);
                        if (pathNum == opendf->Dir_Clus && opendf->flag == TRUE && strcmp(opendf->File_name, name) == 0)
                        {
                            if (offset > fat_ds.fat[cut].DIR_FileSize)
                            {
                                return WRONG_COVER_POS;
                            }
                            int num = 0;
                            char buf[ARGLEN * 10];
                            int first = 0;
                            int writelen = 0;
                            while (scanf("%c", &buf[num]) != EOF && buf[num] != 26)
                            {
                                num++;
                                if (num >= ARGLEN * 10)
                                {
                                    first++;
                                    writelen += write_in(i, type, offset + writelen, num, (void *)buf, fileSystemInfop);
                                    num = 0;
                                }
                            }
                            clearerr(stdin);
                            write_in(i, type, offset + writelen, num, (void *)buf, fileSystemInfop);
                            return SUC;
                        }
                    }
                    return FILE_NOTOPENED;
                }
                else //不是要写的文件
                {
                    cut += 2; //目录项指针指向下一个目录项（要跳过长文件名的短目录项）
                }
            }
            else
            {
                cut++; //如果不是长文件名的话就将指针后移继续寻找
            }
        }
        pathNum = getNext(fileSystemInfop, pathNum);
    } while (pathNum != FAT_END && pathNum != 0);
    return FILE_NOTFOUND;
}

int write_in(int fnum, int type, u32 start, u32 size, void *buf, FileSystemInfop fileSystemInfop)
{
    if (fnum < 0 && fnum >= OPENFILESIZE)
    {
        return -1;
    }
    FAT_DS_BLOCK4K fat_ds;
    Opendfilep opendf = &(fileSystemInfop->Opendf[fnum]);
    /* 写入未打开的文件 */
    if (opendf->flag == FALSE)
    {
        return 0;
    }
    do_read_block4k(fileSystemInfop->fp, (BLOCK4K *)&fat_ds, L2R(fileSystemInfop, opendf->Dir_Clus));
    u32 lin;
    switch (type)
    {
    case TRUNCATION:
        if (fat_ds.fat[opendf->numID].DIR_FileSize != 0 && start == 0)
        {
            lin = (u32)((((u32)fat_ds.fat[opendf->numID].DIR_FstClusHI) << 16) | (u32)fat_ds.fat[opendf->numID].DIR_FstClusLO);
            while (lin != FAT_END && lin != FAT_SAVE && lin != FAT_FREE)
            {
                lin = delfree(fileSystemInfop, lin);
            }
            fat_ds.fat[opendf->numID].DIR_FileSize = 0;
            fat_ds.fat[opendf->numID].DIR_FstClusLO = 0;
            fat_ds.fat[opendf->numID].DIR_FstClusHI = 0;
            do_write_block4k(fileSystemInfop->fp, (BLOCK4K *)&fat_ds, L2R(fileSystemInfop, opendf->Dir_Clus));
        }
        return write_real(fnum, start, size, buf, fileSystemInfop);
        break;
    case ADDITIONAL:
        lin = fat_ds.fat[opendf->numID].DIR_FileSize;
        return write_real(fnum, lin, size, buf, fileSystemInfop);
        break;
    case COVER:
        lin = start;
        if (lin + size > fat_ds.fat[opendf->numID].DIR_FileSize)
        {
            size = fat_ds.fat[opendf->numID].DIR_FileSize - lin;
        }
        return write_real(fnum, lin, size, buf, fileSystemInfop);
        break;
    default:
        /* 类型非法 */
        return -2;
    }
}

int write_real(int fnum, u32 start, u32 size, void *buf, FileSystemInfop fileSystemInfop)
{
    FAT_DS_BLOCK4K fat_ds;
    /* 文件描述符非法 */
    if (fnum < 0 && fnum >= OPENFILESIZE)
    {
        return -1;
    }
    /* 起始位置非法 */
    /* 写入长度非法 */
    if (size == 0)
    {
        return 0;
    }
    Opendfilep opendf = &(fileSystemInfop->Opendf[fnum]);
    /* 写入未打开的文件 */
    if (opendf->flag == FALSE)
    {
        return 0;
    }
    /* 强制移动文件指针 */
    do_read_block4k(fileSystemInfop->fp, (BLOCK4K *)&fat_ds, L2R(fileSystemInfop, opendf->Dir_Clus));
    int fileclus = (u32)((((u32)fat_ds.fat[opendf->numID].DIR_FstClusHI) << 16) | (u32)fat_ds.fat[opendf->numID].DIR_FstClusLO);
    /* 0扩展标志 */
    int flagZero = FALSE;
    if (fileclus == FAT_FREE)
    {
        fileclus = newfree(fileSystemInfop, 0);
        fat_ds.fat[opendf->numID].DIR_FstClusHI = (u16)(fileclus >> 16);
        fat_ds.fat[opendf->numID].DIR_FstClusLO = (u16)(fileclus & 0x0000ffff);
        fat_ds.fat[opendf->numID].DIR_FileSize = SPCSIZE;
        opendf->File_Clus = fileclus;
        flagZero = TRUE;
    }
    opendf->writep = start;
    BLOCK4K block4k;
    /* 寻找要写的磁盘块 */
    fileclus = (u32)((((u32)fat_ds.fat[opendf->numID].DIR_FstClusHI) << 16) | (u32)fat_ds.fat[opendf->numID].DIR_FstClusLO);
    for (u32 i = 0; i < opendf->writep / SPCSIZE; i++)
    {
        int old = fileclus;
        fileclus = getNext(fileSystemInfop, fileclus);
        if (fileclus == FAT_END || fileclus == FAT_SAVE || FAT_FREE)
        {
            flagZero = TRUE;
            fileclus = newfree(fileSystemInfop, old);
        }
    }
    if (fat_ds.fat[opendf->numID].DIR_FileSize <= start)
    {
        flagZero = TRUE;
    }
    if (flagZero == TRUE)
    {
        fat_ds.fat[opendf->numID].DIR_FileSize = opendf->writep;
    }
    /* 开始写入 */
    int len = size;
    int fileclusold;
    /* 4k写入补齐 */
    int writelen = 0;
    if (opendf->writep % SPCSIZE != 0)
    {
        do_read_block4k(fileSystemInfop->fp, &block4k, L2R(fileSystemInfop, fileclus));
        int lin;
        /* 剩余空间比写入的空间大 */
        if (len - writelen < (int)(SPCSIZE - (opendf->writep % SPCSIZE)))
        {
            lin = len - writelen;
        }
        else
        {
            /* 补齐 */
            lin = (SPCSIZE - opendf->writep % SPCSIZE);
        }
        strncpy(&(((char *)&block4k)[(opendf->writep % SPCSIZE)]), (char *)(&((char *)buf)[writelen]), lin);
        writelen += lin;

        do_write_block4k(fileSystemInfop->fp, &block4k, L2R(fileSystemInfop, fileclus));
        opendf->writep += lin;
        if (opendf->writep > fat_ds.fat[opendf->numID].DIR_FileSize)
        {
            fat_ds.fat[opendf->numID].DIR_FileSize = opendf->writep;
        }
        fileclusold = fileclus;
        fileclus = getNext(fileSystemInfop, fileclus);
    }

    while (len - writelen > 0)
    {
        /* 没写完但到了最后一块 */
        if (fileclus == FAT_END || fileclus == FAT_SAVE || FAT_FREE)
        {
            fileclus = newfree(fileSystemInfop, fileclusold);
        }
        do_read_block4k(fileSystemInfop->fp, &block4k, L2R(fileSystemInfop, fileclus));
        int lin;
        if (len - writelen < SPCSIZE)
        {
            lin = len - writelen;
            strncpy((char *)(&(block4k.block[0])), (char *)(&((char *)buf)[writelen]), lin);
            writelen = len;
        }
        else
        {
            lin = SPCSIZE;
            strncpy((char *)(&(block4k.block[0])), (char *)(&((char *)buf)[writelen]), lin);
            writelen += SPCSIZE;
        }

        do_write_block4k(fileSystemInfop->fp, &block4k, L2R(fileSystemInfop, fileclus));
        opendf->writep += lin;
        if (opendf->writep > fat_ds.fat[opendf->numID].DIR_FileSize)
        {
            fat_ds.fat[opendf->numID].DIR_FileSize = opendf->writep;
        }
        fileclusold = fileclus;
        fileclus = getNext(fileSystemInfop, fileclus);
    }
    do_write_block4k(fileSystemInfop->fp, (BLOCK4K *)&fat_ds, L2R(fileSystemInfop, opendf->Dir_Clus));
    return writelen;
}
