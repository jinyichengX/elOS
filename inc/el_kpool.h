#ifndef EL_KPOOL_H
#define EL_KPOOL_H

#include "el_type.h"
#include "el_klist.h"

/* 以N字节对齐 */
#define KPOOL_BYTE_ALIGNMENT 4

/* 静态内存池控制头 */
typedef struct EL_KERNEL_POOL_STATIC_STRUCT
{
    struct list_head ObjBlockList; /* 列表引导头 */
    EL_UINT PerBlockSize;   /* 该内存池中分块大小 */
    EL_UINT TotalBlockNum;   /* 该内存池中总分块数目 */
    EL_UINT UsingBlockCnt;   /* 该内存池中已使用分块计数 */
	struct list_head WaitersToTakePool;/* 等待对象池的线程 */
}EL_KPOOL_INFO_T;

extern EL_RESULT_T EL_stKpoolInitialise(void * PoolSurf,EL_UINT PoolSize,EL_UINT PerBlkSz);
extern void * EL_stKpoolBlockTryAlloc(void *PoolSurf);
extern void EL_stKpoolBlockFree(void *PoolSurf,void *pBlkToFree);
extern void * EL_stKpoolBlockAllocWaitSpin(void *PoolSurf,EL_UINT TicksToWait);
extern void EL_stKpoolClear(void * PoolSurf, void * pBlk);

#endif