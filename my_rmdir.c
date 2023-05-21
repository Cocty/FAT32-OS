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
	char delname[12];
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
			if (nameCheckChange(arg->argv[0], delname) == ERROR)
			{
				strcpy(error.msg, "文件夹名过长或存在非法字符\n\x00");
				printf("文件夹名过长或存在非法字符\n");
				return ERROR;
			}
			for (int i = 0; i < 11; i++)
			{
				delname[i] = toupper(delname[i]);
			}
			delname[11] = '\0';

			if (strcmp(arg->argv[0], "..") == 0 || strcmp(arg->argv[0], ".") == 0)
			{
				printf("文件夹不存在\n");
				return SUCCESS;
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

	u32 pathNum = fileSystemInfop->pathNum;
	u32 cut;
	FAT_DS_BLOCK4K fat_ds;
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
			// DEBUG("|%s|\n|%s|\n", delname, name);
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
				while (delfileNum != FAT_END)
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