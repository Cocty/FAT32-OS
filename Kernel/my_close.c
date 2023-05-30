#include "../fs.h"
#include "../tool.h"

int my_close(const ARGP arg, FileSystemInfop fileSystemInfop, char **helpstr)
{
	char name[ARGLEN];
	FAT_DS_BLOCK4K fat_ds;
	*helpstr =
		"\
功能        关闭当前目录的某个文件\n\
语法格式    close name\n\
		   \n";
	// FAT_DS_BLOCK4K fat_ds;
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
				return close_sfn(fileSystemInfop, name, fat_ds);
			}
			else
			{
				strcpy(name, arg->argv[0]);
				return close_lfn(fileSystemInfop, name, fat_ds);
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

int close_in(int fnum, FileSystemInfop fileSystemInfop)
{
	/* 文件描述符非法 */
	if (fnum < 0 && fnum >= OPENFILESIZE)
	{
		return ERROR;
	}
	Opendfilep opendf = &(fileSystemInfop->Opendf[fnum]);
	opendf->flag = FALSE;
	return SUC;
}

int close_sfn(FileSystemInfop fileSystemInfop, char *name, FAT_DS_BLOCK4K fat_ds)
{

	u32 pathNum = fileSystemInfop->pathNum;
	u32 cut;
	Opendfilep opendf;
	do
	{
		do_read_block4k(fileSystemInfop->fp, (BLOCK4K *)&fat_ds, L2R(fileSystemInfop, pathNum));
		for (cut = 0; cut < SPCSIZE / 32; cut++)
		{
			char lin[12];
			strncpy(lin, fat_ds.fat[cut].name, 11);
			lin[11] = '\0';
			if (fat_ds.fat[cut].name[0] == '\xe5')
			{
				//被删除的
				continue;
			}
			if ((fat_ds.fat[cut].DIR_Attr & ATTR_ARCHIVE) && strcmp(lin, name) == 0)
			{
				//文件
				for (int i = 0; i < OPENFILESIZE; i++)
				{
					opendf = &(fileSystemInfop->Opendf[i]);
					if (pathNum == opendf->Dir_Clus && opendf->flag == TRUE && strcmp(opendf->File_name, name) == 0)
					{
						close_in(i, fileSystemInfop);
						return SUC;
					}
				}
				return FILE_NOTOPENED;
			}
		}
		pathNum = getNext(fileSystemInfop, pathNum);
	} while (pathNum != FAT_FREE && pathNum != FAT_END);

	return SUC;
}

int close_lfn(FileSystemInfop fileSystemInfop, char *name, FAT_DS_BLOCK4K fat_ds)
{
	u32 pathNum = fileSystemInfop->pathNum;
	u32 cut;
	Opendfilep opendf;
	do
	{
		do_read_block4k(fileSystemInfop->fp, (BLOCK4K *)&fat_ds, L2R(fileSystemInfop, pathNum)); //读取当前簇号所在的物理簇
		cut = 0;
		while (cut < SPCSIZE / 32)
		{ //遍历每个目录项
			char lin[12];
			strncpy(lin, fat_ds.fat[cut].name, 11);
			lin[11] = '\0';
			if (fat_ds.fat[cut].name[0] == '\xe5')
			{
				cut++;
				//被删除的
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

				cut += length; //此时现在指针指向了紧挨着的短文件名目录项

				if ((fat_ds.fat[cut].DIR_Attr & ATTR_ARCHIVE) && strcmp(filename, name) == 0)
				{
					//文件
					for (int i = 0; i < OPENFILESIZE; i++)
					{
						opendf = &(fileSystemInfop->Opendf[i]);
						if (pathNum == opendf->Dir_Clus && opendf->flag == TRUE && strcmp(opendf->File_name, name) == 0)
						{
							close_in(i, fileSystemInfop);
							return SUC;
						}
					}
					return FILE_NOTOPENED;
				}
				else //不是要打开的文件
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