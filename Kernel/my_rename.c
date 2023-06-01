#include "../fs.h"
#include "../tool.h"
int my_rename(const ARGP arg, FileSystemInfop fileSystemInfop, char **helpstr)
{
    *helpstr =
        "\
功能        重命名文件\n\
语法格式    rename old_name new_name\n\
name        重命名文件\n\
备注       文件名要符合三八命名方式\n";
    char old_name[ARGLEN];
    char new_name[ARGLEN];
    FAT_DS_BLOCK4K fat_ds;
    if (fileSystemInfop->flag == FALSE)
    {
        return NONE_FILESYS;
    }
    if (arg->len == 2)
    {
        if ((nameCheckChange(arg->argv[0], old_name) == SUC) && (nameCheckChange(arg->argv[1], new_name) == SUC))
        {
            return rename_sfn(fileSystemInfop, old_name, new_name, fat_ds);
        }
        else
        {
            strncpy(old_name, arg->argv[0], strlen(arg->argv[0]));
            strncpy(new_name, arg->argv[1], strlen(arg->argv[1]));
            return rename_lfn(fileSystemInfop, old_name, new_name, fat_ds);
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
}

int rename_sfn(FileSystemInfop fileSystemInfop, char *old_name, char *new_name, FAT_DS_BLOCK4K fat_ds)
{
    u32 pathNum = fileSystemInfop->pathNum;
    u32 cut;
    do
    {
        do_read_block4k(fileSystemInfop->fp, (BLOCK4K *)&fat_ds, L2R(fileSystemInfop, pathNum)); //读取当前簇号所在的物理簇
        for (cut = 0; cut < SPCSIZE / 32; cut++)
        { //遍历每个目录项
            char lin[12];
            strncpy(lin, fat_ds.fat[cut].name, 11);
            lin[11] = '\0';
            if (strcmp(lin, old_name) == 0) //说明找到要重命名的文件了
            {
                strncpy(fat_ds.fat[cut].name, new_name, 11); //将新文件名赋值
                do_write_block4k(fileSystemInfop->fp, (BLOCK4K *)&fat_ds, L2R(fileSystemInfop, pathNum));
                return SUC;
            }
            else if (strcmp(new_name, lin) == 0)
            {
                return FILE_EXSIST;
            }
        }
        pathNum = getNext(fileSystemInfop, pathNum);
    } while (pathNum != FAT_END && pathNum != 0);

    return FILE_NOTFOUND;
}

int rename_lfn(FileSystemInfop fileSystemInfop, char *old_name, char *new_name, FAT_DS_BLOCK4K fat_ds)
{
    char content[ARGLEN * 100];
    if (open_lfn(fileSystemInfop, old_name, fat_ds) != FILEOPENED) //如果文件没有打开
    {
        open_lfn(fileSystemInfop, old_name, fat_ds);
    }
    read_lfn(fileSystemInfop, old_name, fat_ds, 0, 0, content);
    close_lfn(fileSystemInfop, old_name, fat_ds);
    rm_lfn(fileSystemInfop, old_name, fat_ds);

    create_lfn(fileSystemInfop, new_name, fat_ds);
    open_lfn(fileSystemInfop, new_name, fat_ds);

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
            if (fat_ds.fat[cut].name[0] == '\xE5')
            {
                cut++;
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

                if (strcmp(filename, new_name) == 0 && (fat_ds.fat[cut].DIR_Attr & ATTR_ARCHIVE))
                {
                    for (int i = 0; i < OPENFILESIZE; i++)
                    {
                        opendf = &(fileSystemInfop->Opendf[i]);
                        if (pathNum == opendf->Dir_Clus && opendf->flag == TRUE && strcmp(opendf->File_name, new_name) == 0)
                        {
                            if (0 > fat_ds.fat[cut].DIR_FileSize)
                            {
                                return WRONG_COVER_POS;
                            }
                            int num = 0;
                            char buf[ARGLEN * 10];
                            int first = 0;
                            int writelen = 0;
                            setLastWriteTime(&fat_ds.fat[cut]);
                            do_write_block4k(fileSystemInfop->fp, (BLOCK4K *)&fat_ds, L2R(fileSystemInfop, pathNum));

                            while (num < strlen(content))
                            {
                                buf[num++] = content[num++];
                                if (num >= ARGLEN * 10)
                                {
                                    first++;
                                    writelen += write_in(i, 0, 0 + writelen, num, (void *)buf, fileSystemInfop);
                                    num = 0;
                                }
                            }
                            clearerr(stdin);
                            write_in(i, 0, 0 + writelen, num, (void *)buf, fileSystemInfop);
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
    close_lfn(fileSystemInfop, new_name, fat_ds);
    return SUC;
}