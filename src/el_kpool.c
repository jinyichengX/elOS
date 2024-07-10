#include "el_kpool.h"
#include "el_pthread.h"
#include "el_debug.h"
#include "kparam.h"
#include "port.h"
#include <stdio.h>
#include <string.h>

#if KPOOL_BYTE_ALIGNMENT%4 != 0
#error "KPOOL_BYTE_ALIGNMENT macro must be divisible by 4"
#endif

/* ����ض��뷽ʽ */
#if KPOOL_BYTE_ALIGNMENT == 16
#define KPOOL_BYTE_ALIGNMENT_MASK    ( 0x000f )
#elif KPOOL_BYTE_ALIGNMENT == 8
#define KPOOL_BYTE_ALIGNMENT_MASK    ( 0x0007 )
#elif KPOOL_BYTE_ALIGNMENT == 4
#define KPOOL_BYTE_ALIGNMENT_MASK    ( 0x0003 )
#endif

#define KPOOL_BLOCK_HEAD_SZ 			sizeof(LIST_HEAD)
/* ��龲̬�ڴ�غϷ��� */
#define IS_POOL_VALID(P) 				P->TotalBlockNum
/* ����ڴ����� */
#define IS_KPOOL_ALIGNED(BASE) 			(((EL_UINT)BASE & KPOOL_BYTE_ALIGNMENT_MASK) == 0)
/* ���ں˶�����С������4�ֽڶ��� */
#define EL_KPOOL_BLKSZ_ALIGNED(BLK_SZ) 	( (KPOOL_BLOCK_HEAD_SZ + BLK_SZ)+(KPOOL_BYTE_ALIGNMENT - 1) )\
											& ~( (EL_UINT)KPOOL_BYTE_ALIGNMENT_MASK )
#define EL_POOL_BLOCK_NODE_ADDR(POBJ)  	((EL_UCHAR *)POBJ-KPOOL_BLOCK_HEAD_SZ)
	
/**********************************************************************
 * �������ƣ� EL_stKpoolInitialise
 * ���������� ��̬�س�ʼ��
 * ��������� PoolSurf ����̬�ڴ�س���ָ��
			 PoolSize ����̬�ڴ�س���
			 PerBlkSz ������ÿ����̬�ڴ���С
 * ��������� ��
 * �� �� ֵ�� EL_RESULT_OK/EL_RESULT_ERR
 * �޸�����        �汾��     �޸���	      �޸�����
 * -----------------------------------------------
 * 2024/02/25	    V1.0	  jinyicheng	      ����
 ***********************************************************************/
__API__ EL_RESULT_T EL_stKpoolInitialise(void * PoolSurf,EL_UINT PoolSize,EL_UINT PerBlkSz)
{
    EL_KPOOL_INFO_T * pKpoolInfo= (EL_KPOOL_INFO_T *)PoolSurf;
    int i;EL_UCHAR * blk_list;
    /* ���μ�� */
    if((PoolSurf == NULL) || (PoolSize <= sizeof(EL_KPOOL_INFO_T))\
	   || (PerBlkSz == 0))
     return EL_RESULT_ERR;
    /* �����룬������Ӧ�û��Զ����� */
    if (!IS_KPOOL_ALIGNED(PoolSurf)) return EL_RESULT_ERR;
	
	/* ������Ҫ�����ٽ�������ΪΪ���û����ں˿ռ�ͳһ���� */
	/* �����û��ڶ��̻߳�����ʹ�þ�̬�ڴ���䷽ʽʱ���µľ�̬���� */
    OS_Enter_Critical_Check();
    /* ��̬�ڴ�ؿ���ͷ��ʼ�� */
    pKpoolInfo->PerBlockSize = EL_KPOOL_BLKSZ_ALIGNED(PerBlkSz);
    pKpoolInfo->TotalBlockNum = (PoolSize - sizeof(EL_KPOOL_INFO_T))/\
								pKpoolInfo->PerBlockSize;
    pKpoolInfo->UsingBlockCnt = (EL_UINT)0;
    
    if(pKpoolInfo->TotalBlockNum == (EL_UINT)0){
	 OS_Exit_Critical_Check();
	 return EL_RESULT_ERR;
    }
    /* ��ʼ����̬�ڴ�ؿ���ͷ������ͷ */
    INIT_LIST_HEAD(&pKpoolInfo->ObjBlockList);
    /* �����п��õ��ڴ��ڵ�ͨ�������������� */
    blk_list = (EL_UCHAR *)PoolSurf+sizeof(EL_KPOOL_INFO_T);
	list_add_tail((LIST_HEAD *)blk_list,&pKpoolInfo->ObjBlockList);
    for(i = 0; i < pKpoolInfo->TotalBlockNum-1; ++i){
	 blk_list += pKpoolInfo->PerBlockSize;
	 list_add_tail((LIST_HEAD *)blk_list,&pKpoolInfo->ObjBlockList);
    }
	/* ��ʼ���ȴ��б� */
	INIT_LIST_HEAD(&pKpoolInfo->WaitersToTakePool);
    OS_Exit_Critical_Check();
	
    return EL_RESULT_OK;
}

/**********************************************************************
 * �������ƣ� EL_stKpoolBlockTryAlloc
 * ���������� ��ָ����̬�ط����ڴ��
 * ��������� PoolSurf ����̬�ڴ��
 * ��������� ��
 * �� �� ֵ�� ���þ�̬�ڴ���ַ
 * �޸�����        �汾��     �޸���	      �޸�����
 * -----------------------------------------------
 * 2024/02/25	    V1.0	  jinyicheng	      ����
 ***********************************************************************/
__API__ void * EL_stKpoolBlockTryAlloc(void *PoolSurf)
{
    if(PoolSurf == NULL) return NULL;
    EL_KPOOL_INFO_T * pKpoolInfo= (EL_KPOOL_INFO_T *)PoolSurf;
    EL_UCHAR *pBlkToAlloc;
	ASSERT(IS_POOL_VALID(pKpoolInfo));
	
    OS_Enter_Critical_Check();
    if(pKpoolInfo->TotalBlockNum == pKpoolInfo->UsingBlockCnt){
	 ASSERT(list_empty(&pKpoolInfo->ObjBlockList));
	 OS_Exit_Critical_Check();
	 return NULL;
    }
    /* �ͷŵ�һ���ڵ� */
	pBlkToAlloc = (EL_UCHAR *)(pKpoolInfo->ObjBlockList.next + 1); 
    list_del(pKpoolInfo->ObjBlockList.next);
    pKpoolInfo->UsingBlockCnt ++;
    OS_Exit_Critical_Check();
	
    return (void *)(pBlkToAlloc);
}

/**********************************************************************
 * �������ƣ� EL_stKpoolBlockAllocWait
 * ���������� ��ָ����̬�س�ʱ��æ���ȴ������ڴ��
 * ��������� PoolSurf ����̬�ڴ��
             TicksToWait ����ʱ�ȴ�ʱ��
 * ��������� ��
 * �� �� ֵ�� ���þ�̬�ڴ���ַ
 * �޸�����        �汾��     �޸���	      �޸�����
 * -----------------------------------------------
 * 2024/03/02	    V1.0	  jinyicheng	      ����
 ***********************************************************************/
__API__ void * EL_stKpoolBlockAllocWaitSpin(void *PoolSurf,EL_UINT TicksToWait)
{
    if(PoolSurf == NULL) return NULL;
	EL_KPOOL_INFO_T * pKpoolInfo= (EL_KPOOL_INFO_T *)PoolSurf;
    ASSERT(IS_POOL_VALID(pKpoolInfo));
    EL_PTCB_T *Cur_ptcb = EL_GET_CUR_PTHREAD();
    EL_UCHAR *pBlkToAlloc;
    EL_UINT MaxUpperLimitTick = g_TickSuspend_Count + TicksToWait;
    EL_UINT MaxUpperLimitTick_OverFlow = g_TickSuspend_OverFlowCount ;
    if(MaxUpperLimitTick < g_TickSuspend_Count) MaxUpperLimitTick_OverFlow ++;

    OS_Enter_Critical_Check();
    /* �ȴ������ڴ�أ�����ֻ�������ȴ�����Ϊ����ڴ������ٶ��в�ͬ�����ɶ��̹߳���� */
    while(pKpoolInfo->TotalBlockNum == pKpoolInfo->UsingBlockCnt){
        if(TicksToWait == (EL_UINT)0){
            ASSERT(list_empty(&pKpoolInfo->ObjBlockList));
            OS_Exit_Critical_Check();
            return NULL;
        }
        else if(TicksToWait != _MAX_TICKS_TO_WAIT){
            OS_Exit_Critical_Check();
            /* ���ѵ����� */
            PORT_PendSV_Suspend();
            OS_Enter_Critical_Check();
            /* ������Ҫ�����ȴ���ʱ��Ƭ���� */
            TicksToWait = ((((g_TickSuspend_OverFlowCount == MaxUpperLimitTick_OverFlow)&&\
		    (g_TickSuspend_Count >= MaxUpperLimitTick))\
		    ||(g_TickSuspend_OverFlowCount > MaxUpperLimitTick_OverFlow)))?((EL_UINT)0):\
            ((EL_UINT)(MaxUpperLimitTick - g_TickSuspend_Count));
            continue;
        }
        /* ���̷߳���ȴ��б����ù���̬ */
        list_add_tail(&Cur_ptcb->pthread_node,&pKpoolInfo->WaitersToTakePool);
        PTHREAD_STATE_SET(Cur_ptcb,EL_PTHREAD_SUSPEND);

        OS_Exit_Critical_Check();
        PORT_PendSV_Suspend();
        OS_Enter_Critical_Check();
    }
    /* �п��þ�̬�ڴ�飬�ͷŵ�һ���ڵ� */
	pBlkToAlloc = (EL_UCHAR *)(pKpoolInfo->ObjBlockList.next + 1); 
    list_del(pKpoolInfo->ObjBlockList.next);
    pKpoolInfo->UsingBlockCnt ++;
    OS_Exit_Critical_Check();

    return (void *)pBlkToAlloc;
}

static inline void EL_stKpoolBlockNodeReInitialise(void * ObjNodeSurf)
{
	((struct list_head *)ObjNodeSurf)->next = NULL;
	((struct list_head *)ObjNodeSurf)->prev = NULL;
}

/**********************************************************************
 * �������ƣ� EL_stKpoolBlockFree
 * ���������� ��ָ����̬�ػ����ڴ��
 * ��������� PoolSurf ����̬�ڴ��
			 pBlkToFree ����Ҫ�ͷŵľ�̬�ڴ���׵�ַ
 * ��������� ��
 * �� �� ֵ�� ��
 * �޸�����        �汾��     �޸���	      �޸�����
 * -----------------------------------------------
 * 2024/02/25	    V1.0	  jinyicheng	      ����
 ***********************************************************************/
__API__ void EL_stKpoolBlockFree(void *PoolSurf,void *pBlkToFree)
{
    EL_KPOOL_INFO_T * pKpoolInfo = (EL_KPOOL_INFO_T *)PoolSurf;
	EL_PTCB_T *Cur_ptcb = EL_GET_CUR_PTHREAD();
    if((PoolSurf == NULL) || (pBlkToFree == NULL)) return;
    LIST_HEAD * pblk;
	ASSERT(IS_POOL_VALID(pKpoolInfo));
	
    OS_Enter_Critical_Check();
    pblk = (LIST_HEAD *)EL_POOL_BLOCK_NODE_ADDR(pBlkToFree);
    list_add_tail(pblk,&pKpoolInfo->ObjBlockList);
    pKpoolInfo->UsingBlockCnt --;
	
	/* ���ѵ�һ���������ڴ�ض����������߳� */
//    if(!list_empty(&pKpoolInfo->WaitersToTakePool)){
//        list_del(pKpoolInfo->WaitersToTakePool.next);
//        /* ����������б� */
//        list_add_tail(&Cur_ptcb->pthread_node,\
//            KERNEL_LIST_HEAD[EL_PTHREAD_READY]+Cur_ptcb->pthread_prio);
//        PTHREAD_STATE_SET(Cur_ptcb,EL_PTHREAD_READY);
//    }
    OS_Exit_Critical_Check();
}

/**********************************************************************
 * �������ƣ� EL_stKpoolClear
 * ���������� ���㾲̬���е�ĳһ�ڴ��
 * ��������� PoolSurf ����̬�ڴ��
			 pBlk ����Ҫ�����ڴ�ľ�̬�ڴ���׵�ַ
 * ��������� ��
 * �� �� ֵ�� ��
 * �޸�����        �汾��     �޸���	      �޸�����
 * -----------------------------------------------
 * 2024/02/25	    V1.0	  jinyicheng	      ����
 ***********************************************************************/
__API__ void EL_stKpoolClear(void * PoolSurf, void * pBlk)
{
    EL_KPOOL_INFO_T *pKpoolInfo = (EL_KPOOL_INFO_T *)PoolSurf;

    if (PoolSurf == NULL || pBlk == NULL) return;

	OS_Enter_Critical_Check();
    memset(pBlk, 0, pKpoolInfo->PerBlockSize - sizeof(LIST_HEAD));
	OS_Exit_Critical_Check();
}

__API__ void EL_stKpoolStatisticsTake(void * PoolSurf,\
								EL_UINT * blksz,\
								EL_UINT * blknum,\
								EL_UINT * blkused)
{
	EL_KPOOL_INFO_T * pKpoolInfo;
	if(PoolSurf == NULL) return;
	
	pKpoolInfo = (EL_KPOOL_INFO_T *)PoolSurf;
	if(blksz) *blksz = pKpoolInfo->PerBlockSize;
	if(blksz) *blknum = pKpoolInfo->TotalBlockNum;
	if(blksz) *blkused = pKpoolInfo->UsingBlockCnt;
}