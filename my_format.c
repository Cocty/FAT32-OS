#include"fs.h"
#include"tool.h"
#include<stdlib.h>
#include<string.h>
#include<ctype.h>
/* 存在随机根目录dir乱码 原因不明 */
/*
    接受一个参数 不得小于256MB 不得大于2097152MB(2TB)
    默认一个磁盘块512B 一簇4KB 8个块
*/
#define MIN 256
#define MAX 2097152

//单位字节
void vhdset(HD_FTRp vhd,u64 size){
    strcpy(vhd->cookie,"conectix");
    vhd->features=BigtoLittle32(0x00000002);
    vhd->ff_version=BigtoLittle32(0x00010000);
    vhd->data_offset=BigtoLittle64(0xFFFFFFFFFFFFFFFF);
    vhd->timestamp=0x00000000;
    strcpy(vhd->crtr_app,"myfs");
    vhd->crtr_ver=BigtoLittle32(0x00060001);
    strcpy((char*)&vhd->crtr_os,"Wi2k");
    vhd->crtr_os=BigtoLittle32(0x5769326b);
    vhd->orig_size=BigtoLittle64(size);
    vhd->curr_size=BigtoLittle64(size);
    vhd->geometry=BigtoLittle32(0x03eb0c11);
    vhd->type=BigtoLittle32(2); //类型
    vhd->checksum=0;
    // strcmp(vhd->uuid,"                ");
    memset(vhd->uuid,0xff,sizeof(vhd->uuid));
    memset(vhd->reserved,0,sizeof(vhd->reserved));
    u32 chksum=0;
    u8* p=(u8*)vhd;
    for(int i=0;i<512;i++){
        chksum+=p[i];
    }
    vhd->checksum=BigtoLittle32(~chksum);
}
//size为字节数
void MBRset(MBRp mbr,int size){
    (mbr->mbr_in[0]).flag=0;  //
    (mbr->mbr_in[0]).FSflag=0x0b; //FAT32基本分区
    mbr->mbr_in[0].start_sec=0x00000008;
    mbr->mbr_in[0].all=size/BLOCKSIZE-mbr->mbr_in[0].start_sec;
    mbr->end=0xAA55;
}
void FSInfoset(FSInfop fsi){
    memset(fsi,0,BLOCKSIZE);
    fsi->FSI_LeadSig=0x41615252;
    fsi->FSI_StrucSig=0x61417272;
    fsi->FSI_Free_Count=0xffffffff;
    fsi->FSI_Nxt_free=0xFFFFFFFF;
    fsi->end=0xAA550000;
}

//size 为字节数
void BS_BPSset(BS_BPBp bs_bpb,int size,int hiden){
    my_strcpy(bs_bpb->BS_jmpBoot,"\xEB\x58\x90",sizeof(bs_bpb->BS_jmpBoot));
    my_strcpy(bs_bpb->BS_OEMName,"MSDOS5.0",sizeof(bs_bpb->BS_OEMName));
    bs_bpb->BPB_BytsPerSec=512;
    bs_bpb->BPB_SecPerClus=8;//
    bs_bpb->BPB_RsvdSecCnt=32;
    bs_bpb->BPB_NumFATs=2;
    bs_bpb->BPB_RootEntCnt=0;
    bs_bpb->BPB_TotSec16=0;
    bs_bpb->BPB_Media=0xf8;
    bs_bpb->BPB_FATSz16=0;
    bs_bpb->BPB_SecPerTrk=0x3f;
    bs_bpb->BPB_NumHeads=0xff;
    bs_bpb->BPB_HiddSec=hiden;
    bs_bpb->BPB_TotSec32=size/BLOCKSIZE;

    int x=size/SPCSIZE/(512/4);
    bs_bpb->BPB_FATSz32=x+(8-x%8);
    bs_bpb->BPB_ExtFlags=0;
    bs_bpb->BPB_FSVer=0;
    bs_bpb->BPB_RootClus=2;
    bs_bpb->BPB_FSInfo=1;
    bs_bpb->BPB_BkBootSec=6;
    memset(bs_bpb->BPB_Reserved,0,sizeof(bs_bpb->BPB_Reserved));
    bs_bpb->BS_DrvNum=0x80;
    bs_bpb->BS_Reserved1=0;
    bs_bpb->BS_BootSig=0x29;
    memcpy(bs_bpb->BS_VolLab,"NO NAME    ",11);
    memcpy(bs_bpb->BS_FilSysType,"FAT32    ",8);
    bs_bpb->end=0xAA55;
}

int my_format(const ARGP arg){
    char fatName[8]="mfs    ";
    char fileName[ARGLEN]="fs.vhd";
    const char helpstr[]=
"\
功能         格式化文件系统\n\
语法格式     format size [name [namefile]]\n\
szie        磁盘大小 单位MB\n\
fatName        卷标名  默认 mfs\n\
namefile    虚拟磁盘文件路径（当前目录下开始） 默认 fs.vhd\n";
    BLOCK4K block4k;
    FILE *fp=NULL;
    switch(arg->len){
        case 3:
            my_strcpy(fileName,arg->argv[2],ARGLEN);
        case 2:
            my_strcpy(fatName,arg->argv[1],8);
        case 1:
            if(strcmp(arg->argv[0],"/?")==0 && arg->len==1){
                printf(helpstr);
                return SUCCESS;
            }else{
                break;
            }
            break;
        default:
            strcpy(error.msg,"参数数量错误\n\x00");
            printf("参数数量错误\n");
            return ERROR;
    }
   
    
    int size=ctoi(arg->argv[0]);
    DEBUG("磁盘容量大小为%dMB\n",size);

    if(size<MIN||size>MAX){
        strcpy(error.msg,"磁盘容量量错误\n\x00");
        printf("磁盘容量量错误\n");
        return ERROR;
    }
    int cut=size*K*K/SPCSIZE;


    fp=fopen(fileName,"wb");
    if(fp==NULL){
        strcpy(error.msg,"文件打开错误\n\x00");
        printf("文件打开量错误\n");
        return ERROR;
    }

    //生成文件本体
    memset(&block4k,0,SPCSIZE);
    for(int i=0;i<cut;i++){
        do_write_block4k(fp,&block4k,-1);
    }

    //处理vhd格式,在最后一个扇区
    HD_FTR vhd;
    vhdset(&vhd,size*K*K);
    do_write_block(fp,(BLOCK*)&vhd,-1,0);
    
    //磁盘分区，在第0号扇区写mbr
    MBR mbr;
    MBRset(&mbr,size*K*K);
    do_write_block(fp,(BLOCK*)&mbr,0,0);
    DEBUG("MBR 分区的起始扇区号为%d，MBR分区的总扇区数 %d\n",mbr.mbr_in[0].start_sec,mbr.mbr_in[0].all);

    //fatBPB
    BS_BPB bs_pbp;
    BS_BPSset(&bs_pbp,size*K*K-mbr.mbr_in[0].start_sec*BLOCKSIZE,mbr.mbr_in[0].start_sec);
    DEBUG("fat表占用的扇区数为%d\n",bs_pbp.BPB_FATSz32);
    do_write_block(fp,(BLOCK*)&bs_pbp,mbr.mbr_in[0].start_sec*BLOCKSIZE/SPCSIZE,(mbr.mbr_in[0].start_sec*BLOCKSIZE%SPCSIZE)/BLOCKSIZE);
    
    //初始化根目录
    int start=bs_pbp.BPB_FATSz32*bs_pbp.BPB_NumFATs+bs_pbp.BPB_RsvdSecCnt+mbr.mbr_in[0].start_sec+(bs_pbp.BPB_RootClus-2)*bs_pbp.BPB_SecPerClus;
    DEBUG("开始扇区 %d\n",start); 

    BLOCK4K block;
    FAT_DSp fatdsp=(FAT_DSp)&block;
    memset(&block, 0, sizeof(block));
    memcpy(fatdsp->name,fatName,8);
    memcpy(fatdsp->named,"   ",3);
    fatdsp->DIR_Attr=ATTR_VOLUME_ID;
    do_write_block4k(fp,&block,start/8);
    DEBUG("根目录大小 %d\n",sizeof(block));

    FSInfo fsi;
    FSInfoset(&fsi);
    do_write_block(fp,(BLOCK*)&fsi,mbr.mbr_in[0].start_sec*BLOCKSIZE/SPCSIZE,(mbr.mbr_in[0].start_sec*BLOCKSIZE%SPCSIZE)/BLOCKSIZE+1);
    
    //fat设置
    int str_fat1;
    str_fat1=bs_pbp.BPB_RsvdSecCnt+mbr.mbr_in[0].start_sec;

    int str_fat2=bs_pbp.BPB_RsvdSecCnt+bs_pbp.BPB_FATSz32+mbr.mbr_in[0].start_sec;
    DEBUG("fat表1和fat表2的位置：%d %d\n",str_fat1,str_fat2);
    FAT fat;
    fat.fat[0]=0x0ffffff8;
    for(u32 i=1;i<=bs_pbp.BPB_RootClus;i++){
        fat.fat[i]=FAT_END;
    }
    do_write_block(fp,(BLOCK*)&fat,str_fat1/8,str_fat1%8);
    do_write_block(fp,(BLOCK*)&fat,str_fat2/8,str_fat2%8);

    fclose(fp);
    DEBUG("磁盘申请成功\n");
    // if()
    return SUCCESS;
}
