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

#define ELOS_SLAB_BASIC_OBJECT_SIZE 16 /* ���������С */
#define ELOS_PER_SLAB_ZONE_DATA_SIZE 512 /* ÿ��slab�������ڿ����ڷ�����ܴ�С */
#define ELOS_SLAB_CHUNK_MAX_LEVEL 3 /* slab������ */
#define ELOS_SLABZONE_OBJ_MAXNUM (ELOS_PER_SLAB_ZONE_DATA_SIZE/ELOS_SLAB_BASIC_OBJECT_SIZE) /* ���Ŀ���Ŀ */
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

/* slab���ƿ� */
typedef struct EL_SLABControlBlockStruct
{  
    //void * SlabPoolSurf;/* slab����ָ�� */
    EL_UINT SlabZoneSize;/* һ��slab�����С */
    EL_UINT SlabZoneMountedCnt;/* ���ص�slab������ */
    EL_UINT SlabObjSize;/* ����obj��С */
    EL_UINT GreenSlabCnt;/* ��slabͳ�� */
    EL_UINT YellowSlabCnt;/* ��slabͳ�� */
    EL_UINT RedSlabCnt;/* ��slabͳ�� */
    struct list_head ckGreenListHead;/* ����������ͷ,slab��δ���� */
    struct list_head ckYellowListHead;/* ����������ͷ,slab�ѷ��䣬��ʣ�� */
    struct list_head ckRedListHead;/* ����������ͷ��slab������ */
    fELOS_HandleSlabDisprEvt SlabDisprEvt;
    EL_UINT (*ckColorCnt)(LIST_HEAD);/* slab��ɫͳ�� */
}el_SlabPoolHead_t;

/* slab������ */
//typedef EL_CHAR (*fELOS_SlabColorConvIdxTake)(struct EL_SLABDispenserStruct *);
//typedef struct EL_SLABDispenserStruct
//{
//    struct list_head SlabNode;
//    void * obj_pool_data;/* ����س����ַ */
//    EL_UINT SlabObjCnt;/* obj����Ŀ */
//    EL_UINT SlabObjUsed;/* ��ʹ�õ�obj��Ŀ */
//    SlabBitMapBaseType bitmap[BIT_CONV_TYPE_NUM(SlabBitMapBaseType,ELOS_SLABZONE_OBJ_MAXNUM)];/* �����������λͼ */
//    el_SlabColor_t cr_type;/* ��ɫ������ */
//    void * SlabOwner;/* ������slab���ƿ�ڵ� */
//    fELOS_NotifySlabCtrEvt NotifySlabCtrEvt;
////    void * (* NotifyControlModifyCr)(el_SlabDispr_t *,
////                fELOS_SlabColorConvIdxTake );
//}el_SlabDispr_t;

/* Slab���ƿ��ʼ�� */
#define SLAB_POOL_HEAD_INIT(SLAB_POOLHEAD_PTR) do{\
    LIST_HEAD_INIT(&(*SLAB_POOLHEAD_PTR).ckGreenListHead);\
    LIST_HEAD_INIT(&(*SLAB_POOLHEAD_PTR).ckYellowListHead);\
    LIST_HEAD_INIT(&(*SLAB_POOLHEAD_PTR).ckRedListHead);\
    (*SLAB_POOLHEAD_PTR).GreenSlabCnt = 0;\
    (*SLAB_POOLHEAD_PTR).YellowSlabCnt = 0;\
    (*SLAB_POOLHEAD_PTR).RedSlabCnt = 0;\
}while(0);

/* Slab��������ʼ�� */
#define SLAB_DISPENSER_INIT(SLAB_DISPENSER_PTR) do{\
    LIST_HEAD_INIT(&(*SLAB_DISPENSER_PTR).SlabNode);\
}while(0);

#define OS_SLAB_DIPSR_OWNER_SET( P_SLAB_DISPR,P_SLAB_HEAD ) (*P_SLAB_DISPR).SlabOwner=(void *)P_SLAB_HEAD
#define OS_SLAB_DIPSR_OWNER_GET( P_SLAB_DISPR ) ((*P_SLAB_DISPR).SlabOwner)
/**************************************************
 * >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
            s l a b ϵ ͳ �� ��
����������������������������       _______________
|slab���ƿ�2   |       | slab������   |
| ��2^n��      |������������>|������������   |��������������>........
_______________      |���ڴ�ռ�     |
                     �������������������������������� 
����������������������������       _______________
|slab���ƿ�2   |       | slab������   |
| ��2^n+1��    |������������>|������������   |��������������>........
_______________      |���ڴ�ռ�     |
                     �������������������������������� 
 * <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
��������������������������������������������������������������������������������������������������������
Ϊslab����������һ��zoneʱ����slab���ƿ����Ϊ׼
***************************************************/
#endif