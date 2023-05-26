#include <stdio.h>
#include "fs.h"
#include "tool.h"
#include <memory.h>
#include <ctype.h>
//暂时设定为只能删除当前目录下的文件,不包含非空目录
int my_rm(const ARGP arg, FileSystemInfop fileSystemInfop)
{
	char delname[ARGLEN];
	FAT_DS_BLOCK4K fat_ds;
	const char helpstr[] =
		"\
功能		删除文件\n\
格式		rm name\n\
name	  想要删除的文件名\n";

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
			if (nameCheckChange(arg->argv[0], delname) == SUCCESS)
			{
				rm_sfn(fileSystemInfop, delname, fat_ds);
			}
			else
			{
				strcpy(delname, arg->argv[0]);
				rm_lfn(fileSystemInfop, delname, fat_ds);
			}
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

int rm_sfn(FileSystemInfop fileSystemInfop, char *delname, FAT_DS_BLOCK4K fat_ds)
{
	u32 pathNum = fileSystemInfop->pathNum;
	u32 cut;
	u32 delfileNum;
	Opendfilep opendf;
	do
	{
		do_read_block4k(fileSystemInfop->fp, (BLOCK4K *)&fat_ds, L2R(fileSystemInfop, pathNum)); //读取当前簇号所在的物理簇
		for (cut = 0; cut < SPCSIZE / 32; cut++)
		{ //遍历每个目录项
			char name[12];
			strncpy(name, fat_ds.fat[cut].name, 11);
			name[11] = '\0';
			if (fat_ds.fat[cut].name[0] == '\xe5')
			{
				//被删除的
				continue;
			}
			if ((fat_ds.fat[cut].DIR_Attr & ATTR_ARCHIVE) && strcmp(delname, name) == 0) // 找到了被删除的文件
			{
				for (int i = 0; i < OPENFILESIZE; i++)
				{
					opendf = &(fileSystemInfop->Opendf[i]);
					if (pathNum == opendf->Dir_Clus && opendf->flag == TRUE && strcmp(opendf->File_name, name) == 0)
					{
						printf("文件已打开不能删除\n");
						return SUCCESS;
					}
				}
				delfileNum = (u32)((((u32)fat_ds.fat[cut].DIR_FstClusHI) << 16) | (u32)fat_ds.fat[cut].DIR_FstClusLO); //找到该文件所在的起始簇号
				while (delfileNum != FAT_END && delfileNum != 0)
				{
					delfileNum = delfree(fileSystemInfop, delfileNum);
				}
				fat_ds.fat[cut].name[0] = '\xe5';
				do_write_block4k(fileSystemInfop->fp, (BLOCK4K *)&fat_ds, L2R(fileSystemInfop, pathNum)); //回写当前簇号所在的物理簇
				return SUCCESS;
			}
		}
		pathNum = getNext(fileSystemInfop, pathNum);
	} while (pathNum != FAT_END && pathNum != 0);
	printf("文件不存在\n");
	return SUCCESS;
}

int rm_lfn(FileSystemInfop fileSystemInfop, char *delname, FAT_DS_BLOCK4K fat_ds)
{
	u32 pathNum = fileSystemInfop->pathNum;
	u32 cut;
	u32 delfileNum;
	Opendfilep opendf;
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

				if ((fat_ds.fat[cut].DIR_Attr & ATTR_ARCHIVE) && strcmp(delname, filename) == 0) // 找到了被删除的文件
				{
					for (int i = 0; i < OPENFILESIZE; i++)
					{
						opendf = &(fileSystemInfop->Opendf[i]);
						if (pathNum == opendf->Dir_Clus && opendf->flag == TRUE && strcmp(opendf->File_name, filename) == 0)
						{
							printf("文件已打开不能删除\n");
							return SUCCESS;
						}
					}
					delfileNum = (u32)((((u32)fat_ds.fat[cut].DIR_FstClusHI) << 16) | (u32)fat_ds.fat[cut].DIR_FstClusLO); //找到该文件所在的起始簇号
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
					fat_ds.fat[cut].name[0] = '\xe5';														  //同时也标记短目录项的第一个字节为删除标志
					do_write_block4k(fileSystemInfop->fp, (BLOCK4K *)&fat_ds, L2R(fileSystemInfop, pathNum)); //回写当前簇号所在的物理簇
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
				cut++; //如果不是长文件名的话就将指针后移继续寻找
			}
		}
		pathNum = getNext(fileSystemInfop, pathNum);
	} while (pathNum != FAT_END && pathNum != 0);
	printf("文件不存在\n");
	return SUCCESS;
}