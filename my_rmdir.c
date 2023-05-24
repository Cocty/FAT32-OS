#include <stdio.h>
#include "fs.h"
#include "tool.h"
#include <memory.h>
#include <ctype.h>

int isEmpty(FileSystemInfop fileSystemInfop, u32 pathNum)
{
	u32 cut;
	FAT_DS_BLOCK4K fat_ds;
	// u32 delfileNum;
	do
	{
		do_read_block4k(fileSystemInfop->fp, (BLOCK4K *)&fat_ds, L2R(fileSystemInfop, pathNum));
		for (cut = 0; cut < SPCSIZE / 32; cut++)
		{
			if (fat_ds.fat[cut].name[0] == '\xe5' || fat_ds.fat[cut].name[0] == '\x00')
			{
				//被删除的 与空
				continue;
			}
			else
			{
				char name[12];
				strncpy(name, fat_ds.fat[cut].name, 11);
				name[11] = '\0';

				if (strcmp(name, DIR_d) == 0 || strcmp(name, DIR_dd) == 0) //跳过目录项..和.
				{
					continue;
				}
				else
				{
					return FALSE;
				}
			}
		}
		pathNum = getNext(fileSystemInfop, pathNum);
	} while (pathNum != FAT_END);
	return TRUE;
}

//暂时设定为只能删除当前目录下的文件,不包含非空目录
int my_rmdir(const ARGP arg, FileSystemInfop fileSystemInfop)
{
	char delname[ARGLEN];
	FAT_DS_BLOCK4K fat_ds;
	const char helpstr[] =
		"\
功能		删除文件夹\n\
格式		rm name\n\
name	  想要删除的文件夹名\n";

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
		else
		{
			if (nameCheck(arg->argv[0]) == SUCCESS)
			{
				strcpy(delname, arg->argv[0]);
				for (int i = 0; i < 11; i++)
				{
					delname[i] = toupper(delname[i]);
				}
				delname[11] = '\0';
				rm_sfn_dir(fileSystemInfop, delname, fat_ds);
			}
			else
			{
				strcpy(delname, arg->argv[0]);
				rm_lfn_dir(fileSystemInfop, delname, fat_ds);
			}
		}
	}
	else if (arg->len == 0)
	{
		DEBUG("未输入目录名\n");
		return ERROR;
	}
	else
	{
		strcpy(error.msg, "参数数量错误\n\x00");
		printf("参数数量错误\n");
		return ERROR;
	}
}

int rm_lfn_dir(FileSystemInfop fileSystemInfop, char *delname, FAT_DS_BLOCK4K fat_ds)
{
	u32 pathNum = fileSystemInfop->pathNum;
	u32 cut;
	u32 delfileNum;

	do
	{
		do_read_block4k(fileSystemInfop->fp, (BLOCK4K *)&fat_ds, L2R(fileSystemInfop, pathNum)); //读取当前簇号所在的物理簇
		cut = 0;
		while (cut < SPCSIZE / 32)
		{ //遍历每个目录项
			if (fat_ds.fat[cut].name[0] == '\xe5')
			{
				//被删除的
				cut++;
				continue;
			}
			else if (fat_ds.fat[cut].DIR_Attr == ATTR_LONG_NAME) //表示是长文件名目录项
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

				if ((fat_ds.fat[cut].DIR_Attr & ATTR_DIRECTORY) && strcmp(delname, filename) == 0)
				{
					//找到了要删的目录
					delfileNum = (u32)((((u32)fat_ds.fat[cut].DIR_FstClusHI) << 16) | (u32)fat_ds.fat[cut].DIR_FstClusLO); //要删除的文件的起始簇号
					if (isEmpty(fileSystemInfop, delfileNum) == FALSE)
					{
						//空判断
						strcpy(error.msg, "文件夹非空\n\x00");
						printf("文件夹非空\n");
						return ERROR;
					}
					while (delfileNum != FAT_END && delfileNum != 0)
					{
						delfileNum = delfree(fileSystemInfop, delfileNum);
					}
					long_file_term--; //首先退回到长目录项的最后一个
					for (int i = length - 1; i >= 0; i--)
					{
						long_file_term->LDIR_Ord = '\xe5';
						long_file_term--;
					}
					long_file_term = NULL;
					fat_ds.fat[cut].name[0] = '\xe5'; //同时将短目录项的第一个字节标记删除
					do_write_block4k(fileSystemInfop->fp, (BLOCK4K *)&fat_ds, L2R(fileSystemInfop, pathNum));
					cut++;
					return SUCCESS;
				}
				else //不是要删除的文件
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
	} while (pathNum != FAT_END && pathNum != 0);
	printf("文件不存在\n");
	return SUCCESS;
}

int rm_sfn_dir(FileSystemInfop fileSystemInfop, char *delname, FAT_DS_BLOCK4K fat_ds)
{
	u32 pathNum = fileSystemInfop->pathNum;
	u32 cut;
	u32 delfileNum;

	do
	{
		do_read_block4k(fileSystemInfop->fp, (BLOCK4K *)&fat_ds, L2R(fileSystemInfop, pathNum)); //读取该簇
		for (cut = 0; cut < SPCSIZE / 32; cut++)												 //遍历该簇的目录项
		{
			char name[12];
			strncpy(name, fat_ds.fat[cut].name, 11);
			name[11] = '\0';
			if (fat_ds.fat[cut].name[0] == '\xe5')
			{
				//被删除的
				continue;
			}
			if ((fat_ds.fat[cut].DIR_Attr & ATTR_DIRECTORY) && strcmp(delname, name) == 0)
			{
				//找到了要删的目录
				delfileNum = (u32)((((u32)fat_ds.fat[cut].DIR_FstClusHI) << 16) | (u32)fat_ds.fat[cut].DIR_FstClusLO); //要删除的文件的起始簇号
				if (isEmpty(fileSystemInfop, delfileNum) == FALSE)
				{
					//空判断
					strcpy(error.msg, "文件夹非空\n\x00");
					printf("文件夹非空\n");
					return ERROR;
				}
				while (delfileNum != FAT_END && delfileNum != 0)
				{
					delfileNum = delfree(fileSystemInfop, delfileNum);
				}
				fat_ds.fat[cut].name[0] = '\xe5';
				do_write_block4k(fileSystemInfop->fp, (BLOCK4K *)&fat_ds, L2R(fileSystemInfop, pathNum));
				return SUCCESS;
			}
		}
		pathNum = getNext(fileSystemInfop, pathNum);
	} while (pathNum != FAT_END && pathNum != 0);
	printf("文件夹不存在\n");
	return SUCCESS;
}