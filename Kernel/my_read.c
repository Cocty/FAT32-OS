#include "../fs.h"
#include "../tool.h"

int my_read(const ARGP arg, FileSystemInfop fileSystemInfop, char **helpstr, char content[])
{
	char name[ARGLEN];
	u32 start = 0;
	/* 0为读取所有 */
	u32 len = 0;
	*helpstr =
		"\
功能        读取文件内容\n\
语法格式    read name [len[start]]\n\
name	   要读的文件名\n\
len			读取得文件长度 默认为所有\n\
start		读取的开始位置 默认为0\n";
	FAT_DS_BLOCK4K fat_ds;
	if (fileSystemInfop->flag == FALSE)
	{
		return NONE_FILESYS;
	}

	if (arg->len == 3)
	{
		start = ctoi(arg->argv[2]);
		if (start == INF)
		{
			return WRONG_PARANUM;
		}
	}
	else if (arg->len == 2)
	{
		len = ctoi(arg->argv[1]);
		if (len == INF)
		{
			return WRONG_PARANUM;
		}
	}
	else if (arg->len == 1)
	{
		if (strcmp(arg->argv[0], "/?") == 0)
		{
			return HELP_STR;
		}
		else
		{
			if (nameCheckChange(arg->argv[0], name) == SUC)
			{
				return read_sfn(fileSystemInfop, name, fat_ds, len, start, content);
			}
			else
			{
				strcpy(name, arg->argv[0]);
				return read_lfn(fileSystemInfop, name, fat_ds, len, start, content);
			}
		}
	}
	else if (arg->len == 0)
	{
		return WRONG_PARANUM;
	}
}

int read_sfn(FileSystemInfop fileSystemInfop, char *name, FAT_DS_BLOCK4K fat_ds, int len, int start, char content[])
{
	u32 pathNum = fileSystemInfop->pathNum;
	u32 cut;
	Opendfilep opendf;
	u32 index = 0;
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
			if ((fat_ds.fat[cut].DIR_Attr & ATTR_ARCHIVE) && strcmp(name, lin) == 0)
			{
				//文件
				for (int i = 0; i < OPENFILESIZE; i++)
				{
					opendf = &(fileSystemInfop->Opendf[i]);
					if (pathNum == opendf->Dir_Clus && opendf->flag == TRUE && strcmp(opendf->File_name, name) == 0)
					{
						char buf[ARGLEN * 10];
						int readlen = 0;
						if (len == 0)
						{
							len = fat_ds.fat[cut].DIR_FileSize;
						}
						while (len - readlen > ARGLEN * 10)
						{
							readlen += read_real(i, start + readlen, ARGLEN * 10, (void *)buf, fileSystemInfop);
							for (int i = 0; i < ARGLEN * 10; i++)
							{
								// printf("%c", buf[i]);
								content[index++] = buf[i];
							}
						}
						int lin = read_real(i, start + readlen, len - readlen, (void *)buf, fileSystemInfop);
						for (int i = 0; i < lin; i++)
						{
							// printf("%c", buf[i]);
							content[index++] = buf[i];
						}
						// printf("\n");
						setLastAccessTime(&fat_ds.fat[cut]);
						do_write_block4k(fileSystemInfop->fp, (BLOCK4K *)&fat_ds, L2R(fileSystemInfop, pathNum));
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

int read_lfn(FileSystemInfop fileSystemInfop, char *name, FAT_DS_BLOCK4K fat_ds, int len, int start, char content[])
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

				if ((fat_ds.fat[cut].DIR_Attr & ATTR_ARCHIVE) && strcmp(name, filename) == 0) //找到了要读的文件
				{
					//文件
					for (int i = 0; i < OPENFILESIZE; i++)
					{
						opendf = &(fileSystemInfop->Opendf[i]);
						if (pathNum == opendf->Dir_Clus && opendf->flag == TRUE && strcmp(opendf->File_name, name) == 0)
						{
							char buf[ARGLEN * 10];
							int readlen = 0;
							u32 index = 0;
							if (len == 0)
							{
								len = fat_ds.fat[cut].DIR_FileSize;
							}
							while (len - readlen > ARGLEN * 10)
							{
								readlen += read_real(i, start + readlen, ARGLEN * 10, (void *)buf, fileSystemInfop);
								for (int i = 0; i < ARGLEN * 10; i++)
								{
									// printf("%c", buf[i]);
									content[index++] = buf[i];
								}
							}
							int lin = read_real(i, start + readlen, len - readlen, (void *)buf, fileSystemInfop);
							for (int i = 0; i < lin; i++)
							{
								// printf("%c", buf[i]);
								content[index++] = buf[i];
							}
							// printf("\n");
							setLastAccessTime(&fat_ds.fat[cut]);
							do_write_block4k(fileSystemInfop->fp, (BLOCK4K *)&fat_ds, L2R(fileSystemInfop, pathNum));
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

int read_real(int fnum, u32 start, u32 size, void *buf, FileSystemInfop fileSystemInfop)
{
	FAT_DS_BLOCK4K fat_ds;
	BLOCK4K block4k;
	/* 文件描述符非法 */
	if (fnum < 0 && fnum >= OPENFILESIZE)
	{
		return -1;
	}
	Opendfilep opendf = &(fileSystemInfop->Opendf[fnum]);

	do_read_block4k(fileSystemInfop->fp, (BLOCK4K *)&fat_ds, L2R(fileSystemInfop, opendf->Dir_Clus));
	if (start > fat_ds.fat[opendf->numID].DIR_FileSize)
	{
		return -1;
	}
	if (start + size > fat_ds.fat[opendf->numID].DIR_FileSize)
	{
		size = fat_ds.fat[opendf->numID].DIR_FileSize - start;
	}

	int where = SPCSIZE;
	u32 fileclus = opendf->File_Clus;
	u32 fileclusold = 0;
	for (u32 i = 0; i < start / SPCSIZE; i++)
	{
		fileclus = getNext(fileSystemInfop, fileclus);
		where += SPCSIZE;
	}
	u32 len = size;
	opendf->readp = start;
	/* 4k读入入补齐 */
	int readlen = 0;
	if (opendf->readp % SPCSIZE != 0)
	{
		do_read_block4k(fileSystemInfop->fp, &block4k, L2R(fileSystemInfop, fileclus));
		int lin;
		if (len - readlen < (SPCSIZE - opendf->readp % SPCSIZE))
		{
			lin = len - readlen;
		}
		else
		{
			lin = (SPCSIZE - opendf->readp % SPCSIZE);
		}
		readlen += lin;
		strncpy((char *)buf, &(((char *)&block4k)[(opendf->readp % SPCSIZE)]), lin);
		opendf->readp += lin;
		fileclusold = fileclus;
		fileclus = getNext(fileSystemInfop, fileclus);
	}
	while (len - readlen > 0)
	{
		do_read_block4k(fileSystemInfop->fp, &block4k, L2R(fileSystemInfop, fileclus));
		int lin;
		if (len - readlen < SPCSIZE)
		{
			lin = len - readlen;
		}
		else
		{
			lin = SPCSIZE;
		}
		strncpy((char *)(&((char *)buf)[readlen]), ((char *)(&block4k)), lin);
		readlen += lin;
		opendf->readp += lin;
		fileclusold = fileclus;
		fileclus = getNext(fileSystemInfop, fileclus);
	}
	return readlen;
}
