#include "../fs.h"
#include "../tool.h"

int my_create(const ARGP arg, FileSystemInfop fileSystemInfop, char **helpstr)
{
    *helpstr =
        "\
功能        创建文件\n\
语法格式    create name\n\
name       创建文件的名字\n";
    char name[ARGLEN];
    FAT_DS_BLOCK4K fat_ds;
    if (fileSystemInfop->flag == FALSE)
    {
        return NONE_FILESYS;
    }
    if (arg->len == 1)
    {
        if (strcmp(arg->argv[0], "/?") == 0)
        {
            return HELP_STR;
        }
        else
        {
            if (nameCheckChange(arg->argv[0], name) == SUC)
            {
                return create_sfn(fileSystemInfop, name, fat_ds);
            }
            else
            {
                strcpy(name, arg->argv[0]);
                if (Is_repeat(name, fileSystemInfop, fat_ds) != ERROR)
                {
                    return create_lfn(fileSystemInfop, name, fat_ds);
                }
            }
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

int create_sfn(FileSystemInfop fileSystemInfop, char name[], FAT_DS_BLOCK4K fat_ds)
{
    u32 pathNum = fileSystemInfop->pathNum;
    DEBUG("当前目录的逻辑簇号%d\n", pathNum);
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
                return FILE_EXSIST;
            }
        }
        //取得下一个簇
        if (cut == SPCSIZE / 32)
        {
            u32 lin = pathNum;
            pathNum = getNext(fileSystemInfop, pathNum);
            DEBUG("文件分配的新簇号为%d\n", pathNum);
            if (pathNum == FAT_END)
            {
                //全都没有分配一个
                pathNum = newfree(fileSystemInfop, lin);
                if (pathNum == 0)
                {
                    return NOT_ENOUGHSPACE;
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
            setCreationTime(&fat_ds.fat[cut]);

            //在当前目录簇写入新建文件的目录项
            do_write_block4k(fileSystemInfop->fp, (BLOCK4K *)&fat_ds, L2R(fileSystemInfop, pathNum));
            break;
        }
    }
    return SUC;
}

int create_lfn(FileSystemInfop fileSystemInfop, char name[], FAT_DS_BLOCK4K fat_ds)
{
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
                return FILE_EXSIST;
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
                    return NOT_ENOUGHSPACE;
                }
            }
        }
        else
        {
            u32 pathnumd = newfree(fileSystemInfop, 0); //分配一个新簇，且不连接

            wchar_t *long_name = GBKToUTF16(name); // 转换为UTF-16格式的字符串
            int length = wcslen(long_name);
            int fdt_num = length / 13 + 1; //每个长文件目录项最多容纳13个utf16字符

            // 分配目录项数组
            Longfile_term lfn_term[fdt_num];
            memset(lfn_term, 0, fdt_num * sizeof(Longfile_term));
            int u = 0;
            for (int i = fdt_num - 1, j = 1; i >= 0; i--, j++) //倒序过来，先从紧挨着短文件名目录项的长文件名目录项开始
            {
                lfn_term[i].LDIR_Attr = ATTR_LONG_NAME;
                lfn_term[i].LDIR_Type = 0x00;
                lfn_term[i].LDIR_FstClusLO = 0x0000; //此时长文件名的目录项簇号低16位为0000，是无效的
                if (i == 0)
                {
                    lfn_term[i].LDIR_Ord = (lfn_term[i].LDIR_Ord & 0x0F) | 0x40; // 设置高四位为0100
                }
                else
                {
                    lfn_term[i].LDIR_Ord = (lfn_term[i].LDIR_Ord & 0x0F) | 0x00; // 设置高四位为0000
                }
                lfn_term[i].LDIR_Ord |= (u8)j; //低8位序号为倒序
                for (u32 k = 0; k < 5; k++)    //设置第一块name
                {
                    if (u < length)
                    {
                        u16 temp = (unsigned short)long_name[u++];
                        lfn_term[i].LDIR_Name1[k] = temp;
                    }
                    else
                    {
                        lfn_term[i].LDIR_Name1[k] = 0xffff;
                    }
                }
                for (u32 k = 0; k < 6; k++) //设置第二块name
                {
                    if (u < length)
                    {
                        u16 temp = (unsigned short)long_name[u++];
                        lfn_term[i].LDIR_Name2[k] = temp;
                    }
                    else
                    {
                        lfn_term[i].LDIR_Name2[k] = 0xffff;
                    }
                }
                for (u32 k = 0; k < 2; k++) //设置第三块name
                {
                    if (u < length)
                    {
                        u16 temp = (unsigned short)long_name[u++];
                        lfn_term[i].LDIR_Name3[k] = temp;
                    }
                    else
                    {
                        lfn_term[i].LDIR_Name3[k] = 0xffff;
                    }
                }
            }
            memcpy(&fat_ds.fat[cut], &lfn_term, sizeof(lfn_term));

            memset(&fat_ds.fat[cut + fdt_num], 0, sizeof(FAT_DS)); //紧接着分配一个短目录项
            char new_filename[11];
            parsename(name, new_filename);

            strncpy(fat_ds.fat[cut + fdt_num].name, new_filename, 11);

            fat_ds.fat[cut + fdt_num].DIR_Attr = ATTR_ARCHIVE;
            fat_ds.fat[cut + fdt_num].DIR_FstClusHI = (u16)(pathnumd >> 16);
            fat_ds.fat[cut + fdt_num].DIR_FstClusLO = (u16)(pathnumd & 0x0000ffff);
            setCreationTime(&fat_ds.fat[cut + fdt_num]);
            //写入新建文件
            do_write_block4k(fileSystemInfop->fp, (BLOCK4K *)&fat_ds, L2R(fileSystemInfop, pathNum));

            break;
        }
    }
}
