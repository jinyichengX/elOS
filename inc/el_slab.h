#ifndef EL_SLAB_H
#define EL_SLAB_H

#include "el_klist.h"
#include "el_type.h"

struct EL_SLABDispenserStruct;
typedef unsigned int SlabBitMapBaseType;

#define BIT_CONV_TYPE_NUM(TYPE,BIT_NUM) ( (BIT_NUM%(8*typeof(TYPE)))?(BIT_NUM/(8*typeof(TYPE))+1):(BIT_NUM/(8*typeof(TYPE))) )

#define SLAB_SIZE_STEP_TYPE_FIX 0
#define SLAB_SIZE_STEP_TYPE_PROPORTIONAL 1

#define SLAB_OBJ_SIZE_STEP_TYPE SLAB_SIZE_STEP_TYPE_PROPORTIONAL

#define ELOS_SLAB_BASIC_OBJECT_SIZE 16 /* 基本对象大小 */
#define ELOS_PER_SLAB_ZONE_DATA_SIZE 512 /* 每个slab区块用于可用于分配的总大小 */
#define ELOS_SLAB_CHUNK_MAX_LEVEL 3 /* slab最大层数 */
#define ELOS_SLABZONE_OBJ_MAXNUM (ELOS_PER_SLAB_ZONE_DATA_SIZE/ELOS_SLAB_BASIC_OBJECT_SIZE) /* 最多的块数目 */
typedef enum {
    SLAB_CR_GREEN, SLAB_CR_YELLOW, SLAB_CR_RED
}el_SlabColor_t;

typedef struct EL_SLABColorRemapTableStruct{
    el_SlabColor_t SlabColorRemapTable[2];
}el_SlabCrRmp_t;

typedef enum ELOS_SlabDispr_CallBackType{
    CURRENT_SLAB_FULL = 1,
    MODIFY_SLAB_COLOR = 2,
}el_slab_callevt_t;
struct EL_SLABControlBlockStruct;
typedef void (*fELOS_NotifySlabCtrEvt)(struct EL_SLABControlBlockStruct *, el_slab_callevt_t );
typedef fELOS_NotifySlabCtrEvt fELOS_HandleSlabDisprEvt;

/* slab控制块 */
typedef struct EL_SLABControlBlockStruct
{  
    //void * SlabPoolSurf;/* slab池面指针 */
    EL_UINT SlabZoneSize;/* 一个slab区块大小 */
    EL_UINT SlabZoneMountedCnt;/* 挂载的slab区块数 */
    EL_UINT SlabObjSize;/* 单个obj大小 */
    EL_UINT GreenSlabCnt;/* 绿slab统计 */
    EL_UINT YellowSlabCnt;/* 黄slab统计 */
    EL_UINT RedSlabCnt;/* 红slab统计 */
    struct list_head ckGreenListHead;/* 绿区块链表头,slab块未分配 */
    struct list_head ckYellowListHead;/* 黄区块链表头,slab已分配，有剩余 */
    struct list_head ckRedListHead;/* 红区块链表头，slab分配完 */
    fELOS_HandleSlabDisprEvt SlabDisprEvt;
    EL_UINT (*ckColorCnt)(LIST_HEAD);/* slab着色统计 */
}el_SlabPoolHead_t;

/* slab分配器 */
//typedef EL_CHAR (*fELOS_SlabColorConvIdxTake)(struct EL_SLABDispenserStruct *);
//typedef struct EL_SLABDispenserStruct
//{
//    struct list_head SlabNode;
//    void * obj_pool_data;/* 对象池池面地址 */
//    EL_UINT SlabObjCnt;/* obj总数目 */
//    EL_UINT SlabObjUsed;/* 已使用的obj数目 */
//    SlabBitMapBaseType bitmap[BIT_CONV_TYPE_NUM(SlabBitMapBaseType,ELOS_SLABZONE_OBJ_MAXNUM)];/* 分配器保存的位图 */
//    el_SlabColor_t cr_type;/* 着色器类型 */
//    void * SlabOwner;/* 所属的slab控制块节点 */
//    fELOS_NotifySlabCtrEvt NotifySlabCtrEvt;
////    void * (* NotifyControlModifyCr)(el_SlabDispr_t *,
////                fELOS_SlabColorConvIdxTake );
//}el_SlabDispr_t;

/* Slab控制块初始化 */
#define SLAB_POOL_HEAD_INIT(SLAB_POOLHEAD_PTR) do{\
    LIST_HEAD_INIT(&(*SLAB_POOLHEAD_PTR).ckGreenListHead);\
    LIST_HEAD_INIT(&(*SLAB_POOLHEAD_PTR).ckYellowListHead);\
    LIST_HEAD_INIT(&(*SLAB_POOLHEAD_PTR).ckRedListHead);\
    (*SLAB_POOLHEAD_PTR).GreenSlabCnt = 0;\
    (*SLAB_POOLHEAD_PTR).YellowSlabCnt = 0;\
    (*SLAB_POOLHEAD_PTR).RedSlabCnt = 0;\
}while(0);

/* Slab分配器初始化 */
#define SLAB_DISPENSER_INIT(SLAB_DISPENSER_PTR) do{\
    LIST_HEAD_INIT(&(*SLAB_DISPENSER_PTR).SlabNode);\
}while(0);

#define OS_SLAB_DIPSR_OWNER_SET( P_SLAB_DISPR,P_SLAB_HEAD ) (*P_SLAB_DISPR).SlabOwner=(void *)P_SLAB_HEAD
#define OS_SLAB_DIPSR_OWNER_GET( P_SLAB_DISPR ) ((*P_SLAB_DISPR).SlabOwner)
/**************************************************
 * >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
            s l a b 系 统 结 构
——————————————       _______________
|slab控制块2   |       | slab分配器   |
| （2^n）      |——————>|及分配器管理   |———————>........
_______________      |的内存空间     |
                     ———————————————— 
——————————————       _______________
|slab控制块2   |       | slab分配器   |
| （2^n+1）    |——————>|及分配器管理   |———————>........
_______________      |的内存空间     |
                     ———————————————— 
 * <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
————————————————————————————————————————————————————
为slab分配器分配一个zone时，以slab控制块参数为准
***************************************************/
#endif