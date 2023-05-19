#include <stdio.h>
#include "fs.h"
#include "tool.h"
#include <memory.h>
#include <ctype.h>

int my_dir(const ARGP arg, FileSystemInfop fileSystemInfop)
{
	char name[12];
	const char helpstr[] =
		"\
功能        显示当前目录的文件文件夹\n\
语法格式    dir\n\
		   ls\n";
	FAT_DS_BLOCK4K fat_ds;
	if (fileSystemInfop->flag == FALSE)
	{
		strcpy(error.msg, "未指定文件系统\n\x00");
		printf("未指定文件系统\n");
		return ERROR;
	}
	switch (arg->len)
	{
	case 1:
		if (strcmp(arg->argv[0], "/?") == 0)
		{
			printf(helpstr);
			return SUCCESS;
		}
		else
		{
			goto error;
		}
	case 0:
		break;
	default:
	error:;
		strcpy(error.msg, "参数数量错误\n\x00");
		printf("参数数量错误\n");
		return ERROR;
	}

	u32 pathNum = fileSystemInfop->pathNum;
	u32 cut;
	//	while(TRUE){
	int attr = 0;
	int file = 0, filesize = 0;
	do
	{
		do_read_block4k(fileSystemInfop->fp, (BLOCK4K *)&fat_ds, L2R(fileSystemInfop, pathNum)); //读取当前簇
		for (cut = 0; cut < SPCSIZE / 32; cut++)												 // 遍历当前簇的目录项
		{
			char lin[12];
			strncpy(lin, fat_ds.fat[cut].name, 11);
			lin[11] = '\0';
			if (fat_ds.fat[cut].name[0] == '\xe5')
			{
				//被删除的
				continue;
			}
			if (strcmp(lin, DIR_d) == 0 || strcmp(lin, DIR_dd) == 0) //跳过父目录和此目录，即跳过..与.
			{
				continue;
			}

			if ((fat_ds.fat[cut].DIR_Attr & ATTR_DIRECTORY))
			{
				//目录
				attr++;
				printf("%6s %10s %8.8s%3.3s\n",
					   "<DIR>", "", fat_ds.fat[cut].name, fat_ds.fat[cut].named);
			}
			else if ((fat_ds.fat[cut].DIR_Attr & ATTR_ARCHIVE))
			{
				//文件
				filesize += fat_ds.fat[cut].DIR_FileSize;
				file++;
				printf("%6s %10d %8.8s %3.3s\n",
					   "", fat_ds.fat[cut].DIR_FileSize, fat_ds.fat[cut].name, fat_ds.fat[cut].named);
			}
		}
		pathNum = getNext(fileSystemInfop, pathNum);
	} while (pathNum != FAT_END);
	printf("%10d个文件   %10d字节\n", file, filesize);
	printf("%10d个文件夹\n", attr);
	return SUCCESS;
}
