#include "../fs.h"
#include "../tool.h"

int my_dir(const ARGP arg, FileSystemInfop fileSystemInfop, char **helpstr)
{
	*helpstr =
		"\
功能        显示当前目录的文件文件夹\n\
语法格式    dir\n\
		   ls\n";
	FAT_DS_BLOCK4K fat_ds;
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
			return WRONG_PARANUM;
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

					long_file_term++; // 指针向后移动，即获取下一个长文件名目录项
				}
				long_file_term = NULL;
				new_filename[index++] = L'\0';
				reverseString(new_filename, wcslen(new_filename));

				char *filename = UTF16ToGBK(new_filename); //UTF16转化成GBK格式以显示中文

				cut += length; //此时现在指针指向了紧挨着的短文件名目录项
				if (fat_ds.fat[cut].DIR_Attr & ATTR_ARCHIVE)
				{
					//文件
					filesize += fat_ds.fat[cut].DIR_FileSize;
					file++;
					char creation_date[ARGLEN];
					char creation_time[ARGLEN];
					char write_date[ARGLEN];
					char write_time[ARGLEN];
					char access_date[ARGLEN];

					sprintf(creation_date, "%04d-%02d-%02d/", ((fat_ds.fat[cut].DIR_CrtDate & 0xFE00) >> 9) + 1980, (fat_ds.fat[cut].DIR_CrtDate & 0x01E0) >> 5,
							fat_ds.fat[cut].DIR_CrtDate & 0x001F);
					sprintf(creation_time, "%02d:%02d:%02d", (fat_ds.fat[cut].DIR_CrtTime & 0xF800) >> 11, (fat_ds.fat[cut].DIR_CrtTime & 0x07E0) >> 5, (fat_ds.fat[cut].DIR_CrtTime & 0x001F) << 1);
					strcat(creation_date, creation_time);

					sprintf(write_date, "%04d-%02d-%02d/", ((fat_ds.fat[cut].DIR_WrtDate & 0xFE00) >> 9) + 1980, (fat_ds.fat[cut].DIR_WrtDate & 0x01E0) >> 5,
							fat_ds.fat[cut].DIR_WrtDate & 0x001F);
					sprintf(write_time, "%02d:%02d:%02d", (fat_ds.fat[cut].DIR_WriTime & 0xF800) >> 11, (fat_ds.fat[cut].DIR_WriTime & 0x07E0) >> 5, (fat_ds.fat[cut].DIR_WriTime & 0x001F) << 1);
					strcat(write_date, write_time);

					sprintf(access_date, "%04d-%02d-%02d", ((fat_ds.fat[cut].DIR_LastAccDate & 0xFE00) >> 9) + 1980, (fat_ds.fat[cut].DIR_LastAccDate & 0x01E0) >> 5,
							fat_ds.fat[cut].DIR_LastAccDate & 0x001F);

					printf("%6s %10d %s %10s %20s %10s %20s %10s %10s\n",
						   "<FILE>", fat_ds.fat[cut].DIR_FileSize, filename, "创建时间:", creation_date, "最后写入时间:", write_date, "最后访问日期:", access_date);
				}
				else if (fat_ds.fat[cut].DIR_Attr & ATTR_DIRECTORY)
				{
					//目录
					attr++;
					char creation_date[ARGLEN];
					char creation_time[ARGLEN];
					char write_date[ARGLEN];
					char write_time[ARGLEN];
					char access_date[ARGLEN];

					sprintf(creation_date, "%04d-%02d-%02d/", ((fat_ds.fat[cut].DIR_CrtDate & 0xFE00) >> 9) + 1980, (fat_ds.fat[cut].DIR_CrtDate & 0x01E0) >> 5,
							fat_ds.fat[cut].DIR_CrtDate & 0x001F);
					sprintf(creation_time, "%02d:%02d:%02d", (fat_ds.fat[cut].DIR_CrtTime & 0xF800) >> 11, (fat_ds.fat[cut].DIR_CrtTime & 0x07E0) >> 5, (fat_ds.fat[cut].DIR_CrtTime & 0x001F) << 1);
					strcat(creation_date, creation_time);

					sprintf(write_date, "%04d-%02d-%02d/", ((fat_ds.fat[cut].DIR_WrtDate & 0xFE00) >> 9) + 1980, (fat_ds.fat[cut].DIR_WrtDate & 0x01E0) >> 5,
							fat_ds.fat[cut].DIR_WrtDate & 0x001F);
					sprintf(write_time, "%02d:%02d:%02d", (fat_ds.fat[cut].DIR_WriTime & 0xF800) >> 11, (fat_ds.fat[cut].DIR_WriTime & 0x07E0) >> 5, (fat_ds.fat[cut].DIR_WriTime & 0x001F) << 1);
					strcat(write_date, write_time);

					sprintf(access_date, "%04d-%02d-%02d", ((fat_ds.fat[cut].DIR_LastAccDate & 0xFE00) >> 9) + 1980, (fat_ds.fat[cut].DIR_LastAccDate & 0x01E0) >> 5,
							fat_ds.fat[cut].DIR_LastAccDate & 0x001F);
					printf("%6s %10s %s %10s %20s %10s %20s %10s %10s\n",
						   "<DIR>", "", filename, "创建时间:", creation_date, "最后写入时间:", write_date, "最后访问日期:", access_date);
				}
				cut++;
			}
			else //否则是短目录项
			{
				if ((fat_ds.fat[cut].DIR_Attr & ATTR_DIRECTORY))
				{
					//目录
					attr++;
					char creation_date[ARGLEN];
					char creation_time[ARGLEN];
					char write_date[ARGLEN];
					char write_time[ARGLEN];
					char access_date[ARGLEN];

					sprintf(creation_date, "%04d-%02d-%02d/", ((fat_ds.fat[cut].DIR_CrtDate & 0xFE00) >> 9) + 1980, (fat_ds.fat[cut].DIR_CrtDate & 0x01E0) >> 5,
							fat_ds.fat[cut].DIR_CrtDate & 0x001F);
					sprintf(creation_time, "%02d:%02d:%02d", (fat_ds.fat[cut].DIR_CrtTime & 0xF800) >> 11, (fat_ds.fat[cut].DIR_CrtTime & 0x07E0) >> 5, (fat_ds.fat[cut].DIR_CrtTime & 0x001F) << 1);
					strcat(creation_date, creation_time);

					sprintf(write_date, "%04d-%02d-%02d/", ((fat_ds.fat[cut].DIR_WrtDate & 0xFE00) >> 9) + 1980, (fat_ds.fat[cut].DIR_WrtDate & 0x01E0) >> 5,
							fat_ds.fat[cut].DIR_WrtDate & 0x001F);
					sprintf(write_time, "%02d:%02d:%02d", (fat_ds.fat[cut].DIR_WriTime & 0xF800) >> 11, (fat_ds.fat[cut].DIR_WriTime & 0x07E0) >> 5, (fat_ds.fat[cut].DIR_WriTime & 0x001F) << 1);
					strcat(write_date, write_time);

					sprintf(access_date, "%04d-%02d-%02d", ((fat_ds.fat[cut].DIR_LastAccDate & 0xFE00) >> 9) + 1980, (fat_ds.fat[cut].DIR_LastAccDate & 0x01E0) >> 5,
							fat_ds.fat[cut].DIR_LastAccDate & 0x001F);

					printf("%6s %10s %8.8s%3.3s %10s %20s %10s %20s %10s %10s\n",
						   "<DIR>", "", fat_ds.fat[cut].name, fat_ds.fat[cut].named, "创建时间:", creation_date, "最后写入时间:", write_date, "最后访问日期:", access_date);
				}
				else if ((fat_ds.fat[cut].DIR_Attr & ATTR_ARCHIVE))
				{
					//文件
					filesize += fat_ds.fat[cut].DIR_FileSize;
					file++;
					char filename[8];
					findreal_filename(fat_ds.fat[cut].name, filename);
					char creation_date[ARGLEN];
					char creation_time[ARGLEN];
					char write_date[ARGLEN];
					char write_time[ARGLEN];
					char access_date[ARGLEN];

					sprintf(creation_date, "%04d-%02d-%02d/", ((fat_ds.fat[cut].DIR_CrtDate & 0xFE00) >> 9) + 1980, (fat_ds.fat[cut].DIR_CrtDate & 0x01E0) >> 5,
							fat_ds.fat[cut].DIR_CrtDate & 0x001F);
					sprintf(creation_time, "%02d:%02d:%02d", (fat_ds.fat[cut].DIR_CrtTime & 0xF800) >> 11, (fat_ds.fat[cut].DIR_CrtTime & 0x07E0) >> 5, (fat_ds.fat[cut].DIR_CrtTime & 0x001F) << 1);
					strcat(creation_date, creation_time);

					sprintf(write_date, "%04d-%02d-%02d/", ((fat_ds.fat[cut].DIR_WrtDate & 0xFE00) >> 9) + 1980, (fat_ds.fat[cut].DIR_WrtDate & 0x01E0) >> 5,
							fat_ds.fat[cut].DIR_WrtDate & 0x001F);
					sprintf(write_time, "%02d:%02d:%02d", (fat_ds.fat[cut].DIR_WriTime & 0xF800) >> 11, (fat_ds.fat[cut].DIR_WriTime & 0x07E0) >> 5, (fat_ds.fat[cut].DIR_WriTime & 0x001F) << 1);
					strcat(write_date, write_time);

					sprintf(access_date, "%04d-%02d-%02d", ((fat_ds.fat[cut].DIR_LastAccDate & 0xFE00) >> 9) + 1980, (fat_ds.fat[cut].DIR_LastAccDate & 0x01E0) >> 5,
							fat_ds.fat[cut].DIR_LastAccDate & 0x001F);

					printf("%6s %10d %s.%s %10s %20s %10s %20s %10s %10s\n",
						   "<FILE>", fat_ds.fat[cut].DIR_FileSize, filename, fat_ds.fat[cut].named, "创建时间:", creation_date, "最后写入时间:", write_date, "最后访问日期:", access_date);
				}
				cut++;
			}
		}
		pathNum = getNext(fileSystemInfop, pathNum); //取下一个簇
	} while (pathNum != FAT_END);
	printf("%10d个文件   %10d字节\n", file, filesize);
	printf("%10d个文件夹\n", attr);
	return SUC;
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

void findreal_filename(char *oldname, char *newname)
{
	int index = 0;
	for (int i = 0; i < 8; i++)
	{
		if (oldname[i] != ' ')
		{
			newname[index++] = oldname[i];
		}
		else
		{
			break;
		}
	}
	newname[index] = '\0';
}
