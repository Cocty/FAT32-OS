#ifndef __FS__
#define __FS__

#include<stdio.h>
#include<string.h>
#include<stdint.h>

#define ARGNUM      10   /* 最大参数数量 */
#define ARGLEN      1024 /*  单一参数最大长度 */
#define SUCCESS     0    /* 成功返回值 */
#define ERROR       -1   /* 失败返回值 */
#define TRUE        1
#define FALSE       0
#define INF         (1<<30)
#define BLOCKSIZE   512
#define SecPerClus  8
#define SPCSIZE     4096
#define K           1024

/* 截断写 */
#define TRUNCATION  0 
/* 追加写 */
#define ADDITIONAL  1
/* 覆盖写 */
#define COVER       2



#define ATTR_READ_ONLY  0x01     // 只读属性
#define ATTR_HIDDEN     0x02     // 隐藏属性
#define ATTR_SYSTEM     0x04     // 系统属性
#define ATTR_VOLUME_ID  0x08     // 卷标属性
#define ATTR_DIRECTORY  0x10     // 目录属性
#define ATTR_ARCHIVE    0x20     // 存档属性
#define ATTR_LONG_NAME  (ATTR_READ_ONLY|ATTR_HIDDEN|ATTR_SYSTEM|ATTR_VOLUME_ID)  // 长文件名属性,0x0f

#define FAT_SAVE    0x0ffffff0  // 保留值
#define FAT_END     0x0fffffff  // FAT表结束标志，表示文件结束
#define FAT_BAD     0x0ffffff7  // 坏簇标志，表示该簇已经损坏
#define FAT_FREE    0x0         // 表示该簇尚未分配给任何文件，空闲可用

#define DIR_d  ".          "
#define DIR_dd "..         "
#define OPENFILESIZE    10
typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;


#define __DEBUG__

#ifdef __DEBUG__
    #define DEBUG printf
#endif
#ifndef __DEBUG__
    int debug_in(u8 * format,...);
    #define DEBUG debug_in
    
#endif//__DEBUG__


typedef struct _OPENDFILE{
    //是否有效，为Ture有效
    int flag;
    /* 读指针位置 */
    u32 readp;
    /* 写指针位置 */
    u32 writep;
    //目录簇号
    u32 Dir_Clus;
    /* 文件结构项位置编号 */
    u32 numID;
    //文件簇号起始
    u32 File_Clus;
    //文件名字
    u8 File_name[12];
}Opendfile,*Opendfilep;

//文件系统基本信息 重要
/*
    常用数据 
    以下数据涉及的扇区号都从绝对磁盘簇起
    若为定义扇区数则默认为该簇第0扇
*/

typedef struct __FileSystemInfo{
    /*  结构体是否有效TRUE 或FALSE */
    u32 flag;       
    /* .vhd路径 */
    u8 fileName[ARGLEN];
    /* 读写文件指针 */
    FILE* fp;       
    /* 根目录扇区号 */
    u32 rootNum;
    /* 分区表位置 簇 */
    u32 FAT[8];
    /* 当前路径  定义为 / 防止转义爆炸  */
    u8 path[ARGLEN];
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
    u8  BPB_SecPerClus; 
    /* 保留扇区数目 通常为32 */
    u16 BPB_RsvdSecCnt;
    /* FAT表数量 */ 
    u8  BPB_NumFATs;    
    /* 该分区前隐藏扇区数 */
    u32 BPB_HiddSec;    
    /* 该分区总扇区数 */
    u32 BPB_TotSec32;   
    /* fat表所占扇区数 */
    u32 BPB_FATSz32;    
    /* 根本目录所在第一扇区数  逻辑*/
    u32 BPB_RootClis;   
    /* 保留区引导扇所占扇区数 通常为6 */
    u16 BPB_BkBootSec;
    //打开的文件信息
    Opendfile Opendf[OPENFILESIZE];

}FileSystemInfo,*FileSystemInfop;

#pragma pack(1)


typedef struct __FAT_DS{    // 定义结构体类型 __FAT_DS
    u8 name[8];             // 文件名，最长8个字符
    u8 named[3];            // 文件扩展名，最长3个字符
    u8 DIR_Attr;            // 文件属性
    u8 DIR_NTRes;           // 保留字段
    u8 DIR_CrtTimeTeenth;   // 文件创建时间（百分之一秒）
    u16 DIR_CrtTime;        // 文件创建时间（时分秒）
    u16 DIR_CrtDate;        // 文件创建日期
    u16 DIR_LastAccDate;    // 文件上次访问日期
    u16 DIR_FstClusHI;      // 文件起始簇号的高16位
    u16 DIR_WriTime;        // 文件最后修改时间（时分秒）
    u16 DIR_WrtDate;        // 文件最后修改日期
    u16 DIR_FstClusLO;      // 文件起始簇号的低16位
    u32 DIR_FileSize;       // 文件大小，以字节为单位
} FAT_DS, *FAT_DSp;         // 定义别名 FAT_DS 和指针类型别名 FAT_DSp


typedef struct __MBR_in{ //分区表16个字节
    u8 flag;           //若值为80H表示活动分区，若值为00H表示非活动分区。
    u8 start;           //磁头号——第2字节；扇区号——第3字节的低6位；柱面号——为第3字节高2位+第4字节8位。
    u16 starts_c;
    /* 0x0B */
    u8 FSflag;
    /* 结束磁头号 */
    u8 end_c;           //磁头号——第6字节；扇区号——第7字节的低6位；柱面号——第7字节的高2位+第8字节。
    u16 end_sector;
    /* 分区起始扇区号 */
    u32 start_sec;
    /* 分区总扇区数 */
    u32 all;
}MBR_in,*MBR_inp;

typedef struct __MBR{
    u8 recoverd[446];
    MBR_in mbr_in[4];
    u32 end;
}MBR,*MBRp;

//FAT32DBR区
typedef struct __BS_BPB{ 
    u8 BS_jmpBoot[3];           // 引导代码跳转指令
    u8 BS_OEMName[8];           // OEM名称
    u16 BPB_BytsPerSec;         // 每个扇区的字节数
    u8 BPB_SecPerClus;          // 每簇的扇区数
    /* 保留扇区数 */
    u16 BPB_RsvdSecCnt;         // 保留扇区数
    /* FAT数量 */
    u8 BPB_NumFATs;             // FAT表数量
    u16 BPB_RootEntCnt;         // 根目录最大文件数
    u16 BPB_TotSec16;           // 扇区总数（16位）
    u8 BPB_Media;               // 媒体描述符
    u16 BPB_FATSz16;            // 每个FAT表占用扇区数（16位）
    u16 BPB_SecPerTrk;          // 每个磁道的扇区数
    u16 BPB_NumHeads;           // 磁头数
    u32 BPB_HiddSec;            // 隐藏扇区数
    u32 BPB_TotSec32;           // 扇区总数（32位）

    /* FAT扇区数 */
    u32 BPB_FATSz32;            // 每个FAT表占用扇区数（32位）
    u16 BPB_ExtFlags;           // 文件系统标志
    u16 BPB_FSVer;              // 文件系统版本号
    /* 根目录簇号 */
    u32 BPB_RootClis;           // 根目录的簇号
    u16 BPB_FSInfo;             // FSINFO扇区的扇区号
    u16 BPB_BkBootSec;          // 备份引导扇区的扇区号
    u8 BPB_Reserved[12];        // 保留字节
    u8 BS_DrvNum;               // INT 13H驱动器号
    u8 BS_Reserved1;            // 保留字节
    u8 BS_BootSig;              // 扩展引导标志
    u8 BS_VolID[4];             // 卷序列号
    u8 BS_VolLab[11];           // 卷标
    u8 BS_FilSysType[8];        // 文件系统类型
    u8 recover[420];            // 恢复用的保留字节
    u16 end;                    // 结束标志
}BS_BPB,*BS_BPBp;


typedef struct __FSInfo{
    u32 FSI_LeadSig;
    u8 recoved[480];
    u32 FSI_StrucSig;
    u32 FSI_Free_Count;
    u32 FSI_Nxt_free;
    u8 FSI_Reserved2[12];
    u32 end;
}FSInfo,*FSInfop;

//512FAT表
typedef struct __FAT{
    u32 fat[BLOCKSIZE/4];
}FAT,FATp;

typedef struct __FAT4K{
    u32 fat[SPCSIZE/4];
}FAT4K,FAT4Kp;

/* fat目录项512 */
typedef struct __FAT_DS_BLOCK{
    FAT_DS fat[BLOCKSIZE/sizeof(FAT_DS)];
}FAT_DS_BLOCK,*FAT_DS_BLOCKp;

/* fat目录项4K */
typedef struct __FAT_DS_BLOCK4K{
    FAT_DS fat[SPCSIZE/sizeof(FAT_DS)];
}FAT_DS_BLOCK4K,*FAT_DS_BLOCK4Kp;


//vhd结构体
typedef struct __hd_ftr { 
  u8   cookie[8];       /* Identifies original creator of the disk      */ 
  u32    features;        /* Feature Support -- see below                 */ 
  u32    ff_version;      /* (major,minor) version of disk file           */ 
  u64    data_offset;     /* Abs. offset from SOF to next structure       */ 
  u32    timestamp;       /* Creation time.  secs since 1/1/2000GMT       */ 
  u8   crtr_app[4];     /* Creator application                          */ 
  u32    crtr_ver;        /* Creator version (major,minor)                */ 
  u32    crtr_os;         /* Creator host OS                              */ 
  u64    orig_size;       /* Size at creation (bytes)                     */ 
  u64    curr_size;       /* Current size of disk (bytes)                 */ 
  u32    geometry;        /* Disk geometry                                */ 
  u32    type;            /* Disk type                                    */ 
  u32    checksum;        /* 1's comp sum of this struct.                 */ 
  u8   uuid[16];        /* Unique disk ID, used for naming parents      */ 
  u8   saved;           /* one-bit -- is this disk/VM in a saved state? */ 
  u8   hidden;          /* tapdisk-specific field: is this vdi hidden?  */ 
  u8   reserved[426];   /* padding                                      */ 
}HD_FTR,*HD_FTRp; 

#pragma pack()

//全局错误结构体
typedef struct __ERROR{
    u8 msg[ARGLEN];
}ERR;
extern ERR error;

//块
typedef struct __BLOCK{
    u8 data[512];
}BLOCK;

//4K块
typedef struct __4KBLOCK{
    BLOCK block[8];
}BLOCK4K;


//参数结构体
typedef struct __ARGV{
    /* 参数数量 */
    int len;   
    /* 参数数组 */
    u8 argv[ARGNUM][ARGLEN];  
}ARG,*ARGP;

int my_help();    

/*成功返回SUCCESS 失败返回ERROR*/
//命令行可调用
int my_format(const ARGP arg);
//打开文件但未关闭
int my_load(const ARGP arg,FileSystemInfop fileSystemInfop);
//退出文件系统
int my_exitsys(const ARGP arg,FileSystemInfop fileSystemInfop);

int my_cd(const ARGP arg,FileSystemInfop fileSystemInfop);
int my_mkdir(const ARGP arg,FileSystemInfop fileSystemInfop);
int my_rmdir(const ARGP arg,FileSystemInfop fileSystemInfop);
int my_dir(const ARGP arg,FileSystemInfop fileSystemInfop);
/* 创建一个文件但不分配磁盘块 */
int my_create(const ARGP arg,FileSystemInfop fileSystemInfop);
/* 移除一个文件并释放磁盘块 */
int my_rm(const ARGP arg,FileSystemInfop fileSystemInfop);
/* 用户与程序使用的文件打开 */
int my_open(const ARGP arg,FileSystemInfop fileSystemInfop);
/* 用户使用的文件关闭 */
int my_close(const ARGP arg,FileSystemInfop fileSystemInfop);
/* 用户使用的文件写 */
int my_write(const ARGP arg,FileSystemInfop fileSystemInfop);
/*  程序使用的文件度*/
int my_read(const ARGP arg,FileSystemInfop fileSystemInfop);

/* 程序用关闭 fnum为文件描述符 */
int close_in(int fnum,FileSystemInfop fileSystemInfop);
/* 程序用度函数 */
int read_in(int fnum,int start,int len,void *buf,FileSystemInfop fileSystemInfop);
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
int write_in(int fnum,int type,u32 start,u32 size,void* buf,FileSystemInfop fileSystemInfop);
/* 真正的写函数 */
int write_real(int fnum,u32 start,u32 size,void* buf,FileSystemInfop fileSystemInfop);
/* 真正读出函数  size=0位读出所有*/
int read_real(int fnum,u32 start,u32 size,void* buf,FileSystemInfop fileSystemInfop);
#endif //__FS__
