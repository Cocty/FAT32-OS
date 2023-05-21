#include <stdio.h>
#include "fs.h"
#include "tool.h"
#include <memory.h>
#include <ctype.h>
int my_open(const ARGP arg, FileSystemInfop fileSystemInfop)
{
	char name[12];
	const char helpstr[] =
		"\
功能        打开当前目录的文件\n\
语法格式    open name\n\
		   \n";
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
		else
		{
			if (nameCheckChange(arg->argv[0], name) == ERROR)
			{
				strcpy(error.msg, "文件名过长或存在非法字符\n\x00");
				printf("文件名过长或存在非法字符\n");
				return ERROR;
			}
			for (int i = 0; i < 11; i++)
			{
				name[i] = toupper(name[i]);
			}
			name[11] = '\0';
			DEBUG("|%s|\n", name);
		}
	}
	else if (arg->len == 0)
	{
		printf("文件名为空！");
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
	u32 fileclus;
	Opendfilep opendfp;
	do
	{
		do_read_block4k(fileSystemInfop->fp, (BLOCK4K *)&fat_ds, L2R(fileSystemInfop, pathNum)); //读取当前簇
		for (cut = 0; cut < SPCSIZE / 32; cut++)												 //遍历目录项
		{
			char lin[12];
			strncpy(lin, fat_ds.fat[cut].name, 11);
			lin[11] = '\0';

			if (strcmp(lin, name) == 0)
			{
				if (fat_ds.fat[cut].DIR_Attr & ATTR_DIRECTORY)
				{
					printf("不能打开文件夹\n");
					strcpy(error.msg, "不能打开文件夹\n\x00");
					return SUCCESS;
				}
				else
				{
					fileclus = (u32)((((u32)fat_ds.fat[cut].DIR_FstClusHI) << 16) | (u32)fat_ds.fat[cut].DIR_FstClusLO);
					for (int i = 0; i < OPENFILESIZE; i++)
					{
						//只能打开一次文件
						opendfp = &(fileSystemInfop->Opendf[i]);
						if (opendfp->flag == TRUE)
						{
							if ((opendfp->Dir_Clus == pathNum) && (opendfp->File_Clus == fileclus) && (strcmp(opendfp->File_name, name) == 0))
							{
								printf("文件已打开\n");
								return SUCCESS;
							}
						}
						else
						{
							opendfp->flag = TRUE;
							opendfp->Dir_Clus = pathNum;
							opendfp->File_Clus = fileclus;
							opendfp->readp = 0;
							opendfp->writep = 0;
							opendfp->numID = cut;
							strcpy(opendfp->File_name, name);
							printf("%s%d\n", "打开文件成功 文件描述符是", i);
							return SUCCESS;
						}
					}
					printf("打开文件数已达到最大，打开失败\n");
					return SUCCESS;
				}
			}
		}
		pathNum = getNext(fileSystemInfop, pathNum);
	} while (pathNum != FAT_END && pathNum != 0);
	printf("未找到目标文件，打开失败!\n");
	return SUCCESS;
}