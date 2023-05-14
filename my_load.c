/*
    实现按名称加载虚拟磁盘并解析文件系统
    返回必要参数
    接受 虚拟磁盘名称
*/
#include"fs.h"
#include"tool.h"
#include<string.h>
//打开文件但未关闭
int my_load(const ARGP arg,FileSystemInfop fileSystemInfop){
    char fileName[ARGLEN]="fs.vhd";
    const char helpstr[]=
"\
功能        加载文件系统\n\
语法格式    load [namefile]\n\
namefile    虚拟磁盘文件路径（当前目录下开始） 默认 fs.vhd\n";
    FILE *fp=NULL;
    MBR mbr;
    BS_BPB bpb;

    switch(arg->len){
        case 1:
            if(strcmp(arg->argv[0],"/?")==0 && arg->len==1){
                printf(helpstr);
                return SUCCESS;
            }else{
                strcpy(fileName,arg->argv[0]);
                break;
            }
            break;
        case 0:
            break;
        default:
            strcpy(error.msg,"参数数量错误\n\x00");
            printf("参数数量错误\n");
            return ERROR;
    }

    fp=fopen(fileName,"rb+");
    if(fp==NULL){
        strcpy(error.msg,"文件打开错误\n\x00");
        printf("文件打开量错误\n");
        return ERROR;
    }
    memset(fileSystemInfop,0,sizeof(FileSystemInfo));
    fileSystemInfop->flag=TRUE;
    fileSystemInfop->fp=fp;
    strcpy(fileSystemInfop->fileName,fileName);
    strcpy(fileSystemInfop->path,"/");

    //MBR部分
    do_read_block(fp,(BLOCK*)&mbr,0,0);
    fileSystemInfop->MBR_size=mbr.mbr_in[0].all;
    fileSystemInfop->MBR_start=mbr.mbr_in[0].start_sec;


    do_read_block(fp,(BLOCK*)&bpb,fileSystemInfop->MBR_start/8,fileSystemInfop->MBR_start%8);
    fileSystemInfop->BPB_BytsPerSec=bpb.BPB_BytsPerSec;
    fileSystemInfop->BPB_SecPerClus=bpb.BPB_SecPerClus;
    fileSystemInfop->BPB_RsvdSecCnt=bpb.BPB_RsvdSecCnt;
    fileSystemInfop->BPB_NumFATs=bpb.BPB_NumFATs;
    fileSystemInfop->BPB_HiddSec=bpb.BPB_HiddSec;
    fileSystemInfop->BPB_TotSec32=bpb.BPB_TotSec32;
    fileSystemInfop->BPB_FATSz32=bpb.BPB_FATSz32;
    fileSystemInfop->BPB_RootClus=bpb.BPB_RootClus;
    fileSystemInfop->BPB_BkBootSec=bpb.BPB_BkBootSec;


    fileSystemInfop->rootNum=fileSystemInfop->MBR_start+
                    fileSystemInfop->BPB_RsvdSecCnt+
                    fileSystemInfop->BPB_FATSz32*fileSystemInfop->BPB_NumFATs+
                    (fileSystemInfop->BPB_RootClus-2)*fileSystemInfop->BPB_SecPerClus; // 根目录起始扇区

    DEBUG("根目录的起始扇区为%d\n",fileSystemInfop->rootNum);

    u32 start=fileSystemInfop->MBR_start+
                    fileSystemInfop->BPB_RsvdSecCnt; //FAT表起始扇区

    for(u32 i=0;i<fileSystemInfop->BPB_NumFATs;i++){
        fileSystemInfop->FAT[i]=start+i*fileSystemInfop->BPB_FATSz32;
    }

    fileSystemInfop->pathNum=fileSystemInfop->BPB_RootClus;
    /* 初始化打开文件目录 */
    for(int i=0;i<OPENFILESIZE;i++){
        fileSystemInfop->Opendf[i].flag=FALSE;
        memset(fileSystemInfop->Opendf[i].File_name,' ',11);
    }
    if(bpb.end!=0xaa55){
        printf("这不是一个合法的文件系统!\n");
        fileSystemInfop->flag=FALSE;
        return SUCCESS;
    }

    DEBUG("%s 加载成功!\n",fileSystemInfop->fileName);
    DEBUG("根目录扇区号 %d\n",fileSystemInfop->rootNum);
    DEBUG("FAT表1和FAT表2 的扇区号为 %d %d\n",fileSystemInfop->FAT[0],fileSystemInfop->FAT[1]);
    return SUCCESS;
}