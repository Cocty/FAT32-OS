#ifndef __FS__
#define __FS__

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <wchar.h>
#include <stdlib.h>
#include <windows.h>
#include <memory.h>
#include <ctype.h>
#include <math.h>

#define ARGNUM 10   /* 最大参数数量 */
#define ARGLEN 1024 /*  单一参数最大长度 */

#define TRUE 1
#define FALSE 0
#define INF (1 << 30)
#define BLOCKSIZE 512
#define SecPerClus 8
#define SPCSIZE 4096
#define K 1024
#define MAXLENGTH 255

/* 截断写 */
#define TRUNCATION 0
/* 追加写 */
#define ADDITIONAL 1
/* 覆盖写 */
#define COVER 2

#define ATTR_READ_ONLY 0x01
#define ATTR_HIDDEN 0x02
#define ATTR_SYSTEM 0x04
#define ATTR_VOLUME_ID 0x08
#define ATTR_DIRECTORY 0x10
#define ATTR_ARCHIVE 0x20
#define ATTR_LONG_NAME (ATTR_READ_ONLY | \
                        ATTR_HIDDEN |    \
                        ATTR_SYSTEM |    \
                        ATTR_VOLUME_ID)
#define FAT_SAVE 0x0ffffff0
#define FAT_END 0x0fffffff
#define FAT_BAD 0x0ffffff7
#define FAT_FREE 0x0
#define DIR_d ".          "
#define DIR_dd "..         "
#define OPENFILESIZE 10

#define FILEOPENED 10000          //文件已打开
#define DIR_NOT_NULL 10001        //文件夹非空
#define WRONG_WRITE_PATTERN 10002 //写入模式非法
#define WRONG_COVER_POS 10003     //覆盖位置非法
#define FILE_NOTOPENED 10004      //文件未打开
#define MAX_FILE_SIGNAL 10005     //打开文件达到最大数量
#define DIR_EXSIST 10006          //文件夹已经存在，有重名
#define NOT_ENOUGHSPACE 10007     //磁盘空间不足
#define FILE_ERROR 10008          //文件打开量错误
#define WRONG_FILESYS 10009       //非法的文件系统
#define WRONG_INPUT 10010         //输入非法
#define WRONG_CAPACITY 10011      //磁盘容量错误
#define FILE_EXSIST 10012         //文件已经存在，有重名

#define NONE_FILESYS 1000  //未指定文件系统
#define WRONG_PARANUM 1001 //参数数量错误
#define NULL_FILENAME 1002 //空文件名
#define HELP_STR 1003      //打印帮助文档
#define FILE_NOTFOUND 1004 //	文件不存在

#define SUC 1  /* 成功返回值 */
#define ERR -1 /* 失败返回值 */

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;

#define __DEBUG__
#ifdef __DEBUG__
#define DEBUG printf
#endif
#ifndef __DEBUG__
int debug_in(char *format, ...);
#define DEBUG debug_in

#endif //__DEBUG__

typedef struct _OPENDFILE
{
    //是否有效，为Ture有效
    int flag;
    /* 读指针位置 */
    u32 readp;
    /* 写指针位置 */
    u32 writep;
    //目录簇号
    u32 Dir_Clus;
    /* 文件目录项在簇中的编号 */
    u32 numID;
    //文件起始簇号
    u32 File_Clus;
    //文件名字
    char File_name[12];
} Opendfile, *Opendfilep;

//文件系统基本信息 重要
/*
    常用数据 
    以下数据涉及的扇区号都从绝对磁盘簇起
    若为定义扇区数则默认为该簇第0扇
*/

typedef struct __FileSystemInfo
{
    /*  结构体是否有效TRUE 或FALSE */
    u32 flag;
    /* .vhd路径 */
    char fileName[ARGLEN];
    /* 读写文件指针 */
    FILE *fp;
    /* 根目录扇区号 */
    u32 rootNum;
    /* 分区表位置 簇 */
    u32 FAT[2];
    /* 当前路径  定义为 / 防止转义爆炸  */
    char path[ARGLEN];
    /* 当前路径簇号 逻辑的 */
    u32 pathNum;

    /* MBR表部分 */
    /* 区前隐藏扇区数 */
    u32 MBR_start;
    /* 区扇区数 */
    u32 MBR_size;

    /*  fat一扇区BPB、BS部分 */
    /* 每扇区字节数 通常为512 */
    u16 BPB_BytsPerSec;
    /* 每簇扇区数  通常为8 */
    u8 BPB_SecPerClus;
    /* 保留扇区数目 通常为32 */
    u16 BPB_RsvdSecCnt;
    /* FAT表数量 */
    u8 BPB_NumFATs;
    /* 该分区前隐藏扇区数 */
    u32 BPB_HiddSec;
    /* 该分区总扇区数 */
    u32 BPB_TotSec32;
    /* fat表所占扇区数 */
    u32 BPB_FATSz32;
    /* 根本目录所在的簇号  逻辑*/
    u32 BPB_RootClis;
    /* 保留区引导扇所占扇区数 通常为6 */
    u16 BPB_BkBootSec;
    //打开的文件信息
    Opendfile Opendf[OPENFILESIZE];

} FileSystemInfo, *FileSystemInfop;

#pragma pack(1)

typedef struct __FAT_DS
{
    union
    {
        struct
        {
            char name[8];
            char named[3];
        };
        char nameext[11]; // 文件名
    };

    u8 DIR_Attr;
    u8 DIR_NTRes;
    u8 DIR_CrtTimeTeenth;
    u16 DIR_CrtTime;
    u16 DIR_CrtDate;
    u16 DIR_LastAccDate;
    u16 DIR_FstClusHI;
    u16 DIR_WriTime;
    u16 DIR_WrtDate;
    u16 DIR_FstClusLO;
    u32 DIR_FileSize;
} FAT_DS, *FAT_DSp;

typedef struct LONG_FILENAME_TERM //长文件名目录项
{
    u8 LDIR_Ord;        // 目录项序号
    u16 LDIR_Name1[5];  // 字符1-5
    u8 LDIR_Attr;       // 属性位（固定为0x0F）
    u8 LDIR_Type;       // 目录项类型（固定为0）
    u8 LDIR_Chksum;     // 校验和
    u16 LDIR_Name2[6];  // 字符6-11
    u16 LDIR_FstClusLO; // 簇号的低16位
    u16 LDIR_Name3[2];  // 字符12-13
} Longfile_term, *Longfile_termp;

typedef struct __MBR_in
{
    u8 flag;
    u8 start;
    /* 起始扇区磁头号 */
    u16 starts_c;
    /* 0x0B */
    u8 FSflag;
    /* 结束磁头号 */
    u8 end_c;
    u16 end_sector;
    /* 分区起始扇区号 */
    u32 strart_chan;
    /* 分区总扇区数 */
    u32 all;
} MBR_in, *MBR_inp;

typedef struct __MBR
{
    char recoverd[446];
    MBR_in mbr_in[4];
    u16 end;
} MBR, *MBRp;

typedef struct __BS_BPB
{
    char BS_jmpBoot[3];
    char BS_OEMName[8];
    u16 BPB_BytsPerSec;
    u8 BPB_SecPerClus;
    /* 保留扇区数 */
    u16 BPB_RsvdSecCnt;
    /* FAT数量 */
    u8 BPB_NumFATs;
    u16 BPB_RootEntCnt;
    u16 BPB_TotSec16;
    u8 BPB_Media;
    u16 BPB_FATSz16;
    u16 BPB_SecPerTrk;
    u16 BPB_NumHeads;
    u32 BPB_HiddSec;
    u32 BPB_TotSec32;

    /* FAT扇区数 */
    u32 BPB_FATSz32;
    u16 BPB_ExtFlags;
    u16 BPB_FSVer;
    /* 根目录簇号 */
    u32 BPB_RootClis;
    u16 BPB_FSInfo;
    u16 BPB_BkBootSec;
    char BPB_Reserved[12];
    u8 BS_DrvNum;
    u8 BS_Reserved1;
    u8 BS_BootSig;
    char BS_VolID[4];
    char BS_VolLab[11];
    char BS_FilSysType[8];
    char recover[420];
    // MBR mbr[4];
    u16 end;
} BS_BPB, *BS_BPBp;

typedef struct __FSInfo
{
    u32 FSI_LeadSig;
    char recoved[480];
    u32 FSI_StrucSig;
    u32 FSI_Free_Count;
    u32 FSI_Nxt_free;
    char FSI_Reserved2[12];
    u32 end;
} FSInfo, *FSInfop;

//表示FAT表中一个扇区有128个FAT表项
typedef struct __FAT
{
    u32 fat[BLOCKSIZE / 4];
} FAT, FATp;

//一个簇中128个目录项
typedef struct __FAT_DS_BLOCK4K
{
    FAT_DS fat[SPCSIZE / sizeof(FAT_DS)];
} FAT_DS_BLOCK4K, *FAT_DS_BLOCK4Kp;

//vhd结构体
typedef struct __hd_ftr
{
    char cookie[8];     /* Identifies original creator of the disk      */
    u32 features;       /* Feature Support -- see below                 */
    u32 ff_version;     /* (major,minor) version of disk file           */
    u64 data_offset;    /* Abs. offset from SOF to next structure       */
    u32 timestamp;      /* Creation time.  secs since 1/1/2000GMT       */
    char crtr_app[4];   /* Creator application                          */
    u32 crtr_ver;       /* Creator version (major,minor)                */
    u32 crtr_os;        /* Creator host OS                              */
    u64 orig_size;      /* Size at creation (bytes)                     */
    u64 curr_size;      /* Current size of disk (bytes)                 */
    u32 geometry;       /* Disk geometry                                */
    u32 type;           /* Disk type                                    */
    u32 checksum;       /* 1's comp sum of this struct.                 */
    char uuid[16];      /* Unique disk ID, used for naming parents      */
    char saved;         /* one-bit -- is this disk/VM in a saved state? */
    char hidden;        /* tapdisk-specific field: is this vdi hidden?  */
    char reserved[426]; /* padding                                      */
} HD_FTR, *HD_FTRp;

#pragma pack()

//块操纵员素
typedef struct __BLOCK
{
    u8 data[512];
} BLOCK;

//4K块
typedef struct __4KBLOCK
{
    BLOCK block[8];
} BLOCK4K;

//参数结构体
typedef struct __ARGV
{
    /* 参数数量 */
    int len;
    /* 参数数组 */
    char argv[ARGNUM][ARGLEN];
} ARG, *ARGP;

int my_help(char **helpstr);

/*成功返回SUC 失败返回ERR*/
//命令行可调用
int my_format(const ARGP arg, char **helpstr);
//打开文件但未关闭
int my_load(const ARGP arg, FileSystemInfop fileSystemInfop, char **helpstr);
//退出文件系统
int my_exitsys(const ARGP arg, FileSystemInfop fileSystemInfop, char **helpstr);

int my_cd(const ARGP arg, FileSystemInfop fileSystemInfop, char **helpstr);
int my_mkdir(const ARGP arg, FileSystemInfop fileSystemInfop, char **helpstr);
int my_rmdir(const ARGP arg, FileSystemInfop fileSystemInfop, char **helpstr);
int my_dir(const ARGP arg, FileSystemInfop fileSystemInfop, char **helpstr);
/* 创建一个文件但不分配磁盘块 */
int my_create(const ARGP arg, FileSystemInfop fileSystemInfop, char **helpstr);
/* 移除一个文件并释放磁盘块 */
int my_rm(const ARGP arg, FileSystemInfop fileSystemInfop, char **helpstr);
/* 用户与程序使用的文件打开 */
int my_open(const ARGP arg, FileSystemInfop fileSystemInfop, char **helpstr);
/* 用户使用的文件关闭 */
int my_close(const ARGP arg, FileSystemInfop fileSystemInfop, char **helpstr);
/* 用户使用的文件写 */
int my_write(const ARGP arg, FileSystemInfop fileSystemInfop, char **helpstr);
/*  程序使用的文件读*/
int my_read(const ARGP arg, FileSystemInfop fileSystemInfop, char **helpstr, char content[]);

//显示文件系统信息
int my_info(const ARGP arg, FileSystemInfop fileSystemInfop, char **helpstr);
//重命名文件
int my_rename(const ARGP arg, FileSystemInfop fileSystemInfop, char **helpstr);

/* 程序用关闭 fnum为文件描述符 */
int close_in(int fnum, FileSystemInfop fileSystemInfop);
/* 程序用度读函数 */
int read_in(int fnum, int start, int len, void *buf, FileSystemInfop fileSystemInfop);
/* 
    程序用写函数 正确返回写入长度错误返回负数
    特别规定
    type
        0截断写把文件清零不限制读写长度
        1追加写在后面添加内容
        2覆盖写原文件长度不变（不可增加）
    返回值 -1 文件描述符非法
    返回值 -2 文件操作类型非法
    start 只在type 为2 时有用
*/
int write_in(int fnum, int type, u32 start, u32 size, void *buf, FileSystemInfop fileSystemInfop);
/* 真正的写函数 */
int write_real(int fnum, u32 start, u32 size, void *buf, FileSystemInfop fileSystemInfop);
/* 真正读出函数  size=0位读出所有*/
int read_real(int fnum, u32 start, u32 size, void *buf, FileSystemInfop fileSystemInfop);

/*创建短名文件*/
int create_sfn(FileSystemInfop fileSystemInfop, char *name, FAT_DS_BLOCK4K fat_ds);
/*创建长名文件*/
int create_lfn(FileSystemInfop fileSystemInfop, char *name, FAT_DS_BLOCK4K fat_ds);
/*创建短名目录*/
int create_sfn_dir(FileSystemInfop fileSystemInfop, char *name, FAT_DS_BLOCK4K fat_ds);
/*创建长名目录*/
int create_lfn_dir(FileSystemInfop fileSystemInfop, char *name, FAT_DS_BLOCK4K fat_ds);

/*移除短名文件*/
int rm_sfn(FileSystemInfop fileSystemInfop, char *name, FAT_DS_BLOCK4K fat_ds);
/*移除长名文件*/
int rm_lfn(FileSystemInfop fileSystemInfop, char *name, FAT_DS_BLOCK4K fat_ds);

/*移除长文件名目录*/
int rm_lfn_dir(FileSystemInfop fileSystemInfop, char *name, FAT_DS_BLOCK4K fat_ds);

/*移除短文件名目录*/
int rm_sfn_dir(FileSystemInfop fileSystemInfop, char *name, FAT_DS_BLOCK4K fat_ds);

/*打开文件*/
int open_sfn(FileSystemInfop fileSystemInfop, char *name, FAT_DS_BLOCK4K fat_ds);
int open_lfn(FileSystemInfop fileSystemInfop, char *name, FAT_DS_BLOCK4K fat_ds);
/*关闭文件*/
int close_sfn(FileSystemInfop fileSystemInfop, char *name, FAT_DS_BLOCK4K fat_ds);
int close_lfn(FileSystemInfop fileSystemInfop, char *name, FAT_DS_BLOCK4K fat_ds);
/*写文件*/
int write_sfn(FileSystemInfop fileSystemInfop, char *name, FAT_DS_BLOCK4K fat_ds, int type, int offset);
int write_lfn(FileSystemInfop fileSystemInfop, char *name, FAT_DS_BLOCK4K fat_ds, int type, int offset);
/*读文件*/
int read_sfn(FileSystemInfop fileSystemInfop, char *name, FAT_DS_BLOCK4K fat_ds, int len, int start, char content[]);
int read_lfn(FileSystemInfop fileSystemInfop, char *name, FAT_DS_BLOCK4K fat_ds, int len, int start, char content[]);

/*进入文件夹*/
int cd_sfn_dir(FileSystemInfop fileSystemInfop, char *name, FAT_DS_BLOCK4K fat_ds);
int cd_lfn_dir(FileSystemInfop fileSystemInfop, char *name, FAT_DS_BLOCK4K fat_ds);

/*重命名文件*/
int rename_sfn(FileSystemInfop fileSystemInfop, char *old_name, char *new_name, FAT_DS_BLOCK4K fat_ds);
int rename_lfn(FileSystemInfop fileSystemInfop, char *old_name, char *new_name, FAT_DS_BLOCK4K fat_ds);

#endif //__FS__
