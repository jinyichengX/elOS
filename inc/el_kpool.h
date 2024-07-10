#ifndef EL_KPOOL_H
#define EL_KPOOL_H

#include "el_type.h"
#include "el_klist.h"

/* ��N�ֽڶ��� */
#define KPOOL_BYTE_ALIGNMENT 4

/* ��̬�ڴ�ؿ���ͷ */
typedef struct EL_KERNEL_POOL_STATIC_STRUCT
{
    struct list_head ObjBlockList; /* �б�����ͷ */
    EL_UINT PerBlockSize;   /* ���ڴ���зֿ��С */
    EL_UINT TotalBlockNum;   /* ���ڴ�����ֿܷ���Ŀ */
    EL_UINT UsingBlockCnt;   /* ���ڴ������ʹ�÷ֿ���� */
	struct list_head WaitersToTakePool;/* �ȴ�����ص��߳� */
}EL_KPOOL_INFO_T;

extern EL_RESULT_T EL_stKpoolInitialise(void * PoolSurf,EL_UINT PoolSize,EL_UINT PerBlkSz);
extern void * EL_stKpoolBlockTryAlloc(void *PoolSurf);
extern void EL_stKpoolBlockFree(void *PoolSurf,void *pBlkToFree);
extern void * EL_stKpoolBlockAllocWaitSpin(void *PoolSurf,EL_UINT TicksToWait);
extern void EL_stKpoolClear(void * PoolSurf, void * pBlk);

#endif