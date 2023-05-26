#include <stdio.h>
#include "fs.h"
#include "tool.h"
#include <memory.h>
#include <ctype.h>

int my_cd(const ARGP arg, FileSystemInfop fileSystemInfop)
{
	char name[12];
	const char helpstr[] =
		"\
功能        进入文件夹\n\
语法格式    cd name\n\
name       进入文件夹的名字\n\
备注       文件名强制转为大写，文件名最长不超过8位\n";
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
		else if (nameCheck(arg->argv[0]) == SUCCESS) //短目录项
		{
			memset(name, ' ', 12);
			strncpy(name, arg->argv[0], strlen(arg->argv[0]));
			name[11] = '\0';
			cd_sfn_dir(fileSystemInfop, name, fat_ds);
		}
		else if (nameCheck(arg->argv[0]) != SUCCESS)
		{
			strcpy(name, arg->argv[0]);
			cd_lfn_dir(fileSystemInfop, name, fat_ds);
		}
	}
	else if (arg->len == 0)
	{
		DEBUG("未输入文件名\n");
		return ERROR;
	}
	else
	{
		strcpy(error.msg, "参数数量错误\n\x00");
		printf("参数数量错误\n");
		return ERROR;
	}
}

int cd_sfn_dir(FileSystemInfop fileSystemInfop, char *name, FAT_DS_BLOCK4K fat_ds)
{
	u32 pathNum = fileSystemInfop->pathNum;
	u32 cut;

	do
	{
		do_read_block4k(fileSystemInfop->fp, (BLOCK4K *)&fat_ds, L2R(fileSystemInfop, pathNum));
		for (cut = 0; cut < SPCSIZE / 32; cut++)
		{
			char lin[12];
			strncpy(lin, fat_ds.fat[cut].name, 11);
			lin[11] = '\0';
			if ((fat_ds.fat[cut].DIR_Attr & ATTR_DIRECTORY) && strcmp(name, lin) == 0)
			{
				fileSystemInfop->pathNum = (u32)((((u32)fat_ds.fat[cut].DIR_FstClusHI) << 16) | (u32)fat_ds.fat[cut].DIR_FstClusLO);

				if (strcmp(lin, DIR_d) == 0)
				{
				}
				else if (strcmp(lin, DIR_dd) == 0)
				{
					int lin = 0;
					for (int i = strlen(fileSystemInfop->path) - 1; i >= 0; i--)
					{
						if (fileSystemInfop->path[i] == '/')
						{
							lin++;
							if (lin == 2)
							{
								fileSystemInfop->path[i + 1] = 0x00;
								break;
							}
						}
					}
				}
				else
				{
					strcat(fileSystemInfop->path, lin);
					for (int i = strlen(fileSystemInfop->path) - 1; i >= 0; i--)
					{
						if (fileSystemInfop->path[i] == ' ')
						{
							fileSystemInfop->path[i] = 0x00;
						}
						else
						{
							break;
						}
					}
					strcat(fileSystemInfop->path, "/");
					fileSystemInfop->path[strlen(fileSystemInfop->path)] = '\x0';
				}
				return SUCCESS;
			}
		}
		pathNum = getNext(fileSystemInfop, pathNum);
	} while (pathNum != FAT_FREE && pathNum != FAT_END);
	printf("文件夹不存在\n");
	return SUCCESS;
}

int cd_lfn_dir(FileSystemInfop fileSystemInfop, char *name, FAT_DS_BLOCK4K fat_ds)
{
	u32 pathNum = fileSystemInfop->pathNum;
	u32 cut;
	do
	{
		do_read_block4k(fileSystemInfop->fp, (BLOCK4K *)&fat_ds, L2R(fileSystemInfop, pathNum));
		while (cut < SPCSIZE / 32)
		{													//遍历每个目录项
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
					long_file_term->LDIR_Ord = '\xe5'; //标记每个长目录项的第一个字节为0xE5 表示删除
					long_file_term++;				   // 指针向后移动，即获取下一个长文件名目录项
				}

				new_filename[index++] = L'\0';
				reverseString(new_filename, wcslen(new_filename));
				char *filename = UTF16ToGBK(new_filename); //UTF16转化成GBK格式以显示中文

				cut += length; //此时现在指针指向了紧挨着的短文件名目录项

				if ((fat_ds.fat[cut].DIR_Attr & ATTR_DIRECTORY) && strcmp(name, filename) == 0) //找到了要进入的文件夹
				{
					fileSystemInfop->pathNum = (u32)((((u32)fat_ds.fat[cut].DIR_FstClusHI) << 16) | (u32)fat_ds.fat[cut].DIR_FstClusLO);

					if (strcmp(filename, DIR_d) == 0)
					{
					}
					else if (strcmp(filename, DIR_dd) == 0)
					{
						int lin = 0;
						for (int i = strlen(fileSystemInfop->path) - 1; i >= 0; i--)
						{
							if (fileSystemInfop->path[i] == '/')
							{
								lin++;
								if (lin == 2)
								{
									fileSystemInfop->path[i + 1] = 0x00;
									break;
								}
							}
						}
					}
					else
					{
						strcat(fileSystemInfop->path, filename);
						for (int i = strlen(fileSystemInfop->path) - 1; i >= 0; i--)
						{
							if (fileSystemInfop->path[i] == ' ')
							{
								fileSystemInfop->path[i] = 0x00;
							}
							else
							{
								break;
							}
						}
						strcat(fileSystemInfop->path, "/");
						fileSystemInfop->path[strlen(fileSystemInfop->path)] = '\x0';
					}
					cut++;
					return SUCCESS;
				}
				else //不是要找的文件夹
				{
					cut += 2; //目录项指针指向下一个目录项（要跳过长文件名的短目录项）
				}
			}
			else
			{
				cut++;
			}
		}
		pathNum = getNext(fileSystemInfop, pathNum);
	} while (pathNum != FAT_FREE && pathNum != FAT_END);
	printf("文件夹不存在\n");
	return SUCCESS;
}
