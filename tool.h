#include "fs.h"
#ifndef __TOOL__
#define __TOOL__

//大小端转换,因为windows本身fwrite函数是小段字节序写入，但是读取是大端字节序，所以如果要按小端读就要转化。
//int64
#define BigtoLittle64(A) ((((u64)(A)&0xff00000000000000L) >> 56) | \
                          (((u64)(A)&0x00ff000000000000L) >> 40) | \
                          (((u64)(A)&0x0000ff0000000000L) >> 24) | \
                          (((u64)(A)&0x000000ff00000000L) >> 8) |  \
                          (((u64)(A)&0x00000000ff000000L) << 8) |  \
                          (((u64)(A)&0x0000000000ff0000L) << 24) | \
                          (((u64)(A)&0x000000000000ff00L) << 40) | \
                          (((u64)(A)&0x00000000000000ffL) << 56))

//int32
#define BigtoLittle32(A) ((((u32)(A)&0xff000000) >> 24) | \
                          (((u32)(A)&0x00ff0000) >> 8) |  \
                          (((u32)(A)&0x0000ff00) << 8) |  \
                          (((u32)(A)&0x000000ff) << 24))

//int16
#define BigtoLittle16(A) ((((u16)(A)&0xff00) >> 8) | \
                          (((u16)(A)&0x00ff) << 8))

#define GET_BIT(x, bit) (((x) >> (bit)) & 1) /* 获取第bit位 */

//字符转数字 出错返回INF
int ctoi(const char *ch);
//取得参数
int getargv(ARGP argp);
//逻辑簇转实际簇号
u32 L2R(FileSystemInfop fsip, u32 num);

//实际写入函数 offset为物理簇号 为-1时保持文件默认指针
int do_write_block4k(FILE *fp, BLOCK4K *block4k, int offset);
//实际写入函数 offset为物理簇号 为-1时保持文件默认指针 num 为第几块0-7
int do_write_block(FILE *fp, BLOCK *block, int offset, int num);
//实际读入函数 offset为物理簇号 为-1时保持文件默认指针
int do_read_block4k(FILE *fp, BLOCK4K *block4k, int offset);
//实际读入函数 offset为物理簇号 为-1时保持文件默认指针 num 为第几块0-7
int do_read_block(FILE *fp, BLOCK *block, int offset, int num);

/* 取得一个空闲簇簇号 返回0失败，否则返回空闲簇号 num为当前簇号，若num=0则不连接，直接分配一个簇，否则num簇的FAT表项为新分配的表项的簇号
*/
int newfree(FileSystemInfop fsip, u32 num);

//释放一个已用簇 num簇号，返回被删除的下一块的簇号
int delfree(FileSystemInfop fsip, u32 num);

/* 取得num簇的下一簇 返回下一簇编号 出错返回0*/
u32 getNext(FileSystemInfop fsip, u32 num);
/* 把输入的文件名转换为83明明格式的文件名 若文件名合法则返回SUC否则返回ERROR */
int nameCheckChange(const char name[ARGLEN], char name38[12]);

//检查短目录项
int nameCheck(char name[ARGLEN]);

/*将GBK的char数组转化为UTF16编码格式的数组*/
wchar_t *GBKToUTF16(const char *gbkStr);

// 将UTF-16格式的wchar_t字符串转换为GBK格式的char字符串
char *UTF16ToGBK(wchar_t *utf16Str);

//将文件名表示成 ：文件名前六个字符~1扩展名的形式
void parsename(char *filename, char *new_filename);

/*逆置字符串*/
void reverseString(wchar_t *str, int length);

/*将长度为11的文件名（文件名 空格 加扩展名）转变为文件名*/
void findreal_filename(char *oldname, char *newname);
/*设置时间*/
void setCreationTime(FAT_DSp file);
void setLastAccessTime(FAT_DSp file);
void setLastWriteTime(FAT_DSp file);
/*判断是否重名，即创建文件时是否与当前目录下的某个文件重名*/
int Is_repeat(char *name, FileSystemInfop fileSystemInfop, FAT_DS_BLOCK4K fat_ds);

#endif