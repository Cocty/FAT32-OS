#include <stdio.h>
#include "fs.h"
#include "tool.h"
#include <memory.h>
#include <ctype.h>

int my_dir(const ARGP arg, FileSystemInfop fileSystemInfop)
{
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
	if (arg->len == 1)
	{
		if (strcmp(arg->argv[0], "/?") == 0)
		{
			printf(helpstr);
			return SUCCESS;
		}
		else
		{
			strcpy(error.msg, "参数数量错误\n\x00");
			printf("参数数量错误\n");
			return ERROR;
		}
	}

	u32 pathNum = fileSystemInfop->pathNum;
	u32 cut;
	int attr = 0;
	int file = 0, filesize = 0;
	do
	{
		do_read_block4k(fileSystemInfop->fp, (BLOCK4K *)&fat_ds, L2R(fileSystemInfop, pathNum)); //读取当前簇
		cut = 0;
		while (cut < SPCSIZE / 32) // 遍历当前簇的目录项
		{
			if (fat_ds.fat[cut].name[0] == '\xe5')
			{
				//被删除的
				cut++;
				continue;
			}
			char lin[12];
			strncpy(lin, fat_ds.fat[cut].name, 11);
			lin[11] = '\0';

			if (strcmp(lin, DIR_d) == 0 || strcmp(lin, DIR_dd) == 0) //跳过父目录和此目录，即跳过..与.
			{
				cut++;
				continue;
			}
			if (fat_ds.fat[cut].DIR_Attr == 0x0f) //表示是长文件名目录项
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

					long_file_term++; // 指针向后移动，即获取下一个长文件名目录项
				}
				long_file_term = NULL;
				new_filename[index++] = L'\0';
				reverseString(new_filename, wcslen(new_filename));
				// wprintf(L"%ls\n", new_filename);
				cut += length; //此时现在指针指向了紧挨着的短文件名目录项
				if (fat_ds.fat[cut].DIR_Attr & ATTR_ARCHIVE)
				{
					//文件
					filesize += fat_ds.fat[cut].DIR_FileSize;
					file++;
					wprintf(L"%6s %10d %ls\n",
							L"<FILE>", fat_ds.fat[cut].DIR_FileSize, new_filename);
				}
				else if (fat_ds.fat[cut].DIR_Attr & ATTR_DIRECTORY)
				{
					//目录
					attr++;
					wprintf(L"%6s %10s %ls\n",
							L"<DIR>", L"", new_filename);
				}
				cut++;
			}
			else //否则是短目录项
			{
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
						   "<FILE>", fat_ds.fat[cut].DIR_FileSize, fat_ds.fat[cut].name, fat_ds.fat[cut].named);
				}
				cut++;
			}
		}
		pathNum = getNext(fileSystemInfop, pathNum); //取下一个簇
	} while (pathNum != FAT_END);
	printf("%10d个文件   %10d字节\n", file, filesize);
	printf("%10d个文件夹\n", attr);
	return SUCCESS;
}

void reverseString(wchar_t *str, int length) //逆置字符串
{
	int start = 0;
	int end = length - 1;
	wchar_t temp;

	while (start < end)
	{
		temp = str[start];
		str[start] = str[end];
		str[end] = temp;
		start++;
		end--;
	}
}
