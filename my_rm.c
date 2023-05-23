#include <stdio.h>
#include "fs.h"
#include "tool.h"
#include <memory.h>
#include <ctype.h>
//暂时设定为只能删除当前目录下的文件,不包含非空目录
int my_rm(const ARGP arg, FileSystemInfop fileSystemInfop)
{
	char delname[ARGLEN];
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

				for (int i = 0; i < 11; i++)
				{
					delname[i] = toupper(delname[i]);
				}
				delname[11] = '\0';
			}
			else
			{
				strcpy(error.msg, "文件名过长或存在非法字符\n\x00");
				printf("文件名过长或存在非法字符\n");
				return ERROR;
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

	u32 pathNum = fileSystemInfop->pathNum;
	u32 cut;
	FAT_DS_BLOCK4K fat_ds;
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
			DEBUG("|%s|\n|%s|\n", delname, name);
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