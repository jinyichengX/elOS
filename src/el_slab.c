#include "el_slab.h"
#include "port.h"
#include <string.h>

///* ����ÿ��slab zone���ù̶���С�������Զ����� */
//static EL_UINT g_SlabPoolSize[ELOS_SLAB_CHUNK_MAX_LEVEL] = {
//    [ 0 ... ELOS_SLAB_CHUNK_MAX_LEVEL-1 ] = ELOS_PER_SLAB_ZONE_DATA_SIZE;
//};
///* slab���ƿ�����obj��С */
//static EL_UINT g_SlabObjSize[ELOS_SLAB_CHUNK_MAX_LEVEL] = {
//};
///* ��ɫ�ڵ�ת�Ʊ� */
//el_SlabCrRmp_t el_SlabCrRmpTab[4] = {
//    {SLAB_CR_GREEN,SLAB_CR_YELLOW},{SLAB_CR_YELLOW,SLAB_CR_GREEN},
//    {SLAB_CR_YELLOW,SLAB_CR_RED},{SLAB_CR_YELLOW,SLAB_CR_YELLOW},
//    {SLAB_CR_RED,SLAB_CR_YELLOW},
//};
//EL_CHAR ELOS_SlabDisprEventCallBack(el_slab_callevt_t callevt_type);
///* ��ȡ��ɫ������ */
//EL_UINT ELOS_SlabColorCntList(void *Slab_PoolSurf,LIST_HEAD* pList)
//{

//}

//void ELOS_SlabDisprBitmapInitialise(el_SlabDispr_t * pDispr)
//{
//    if( pDispr == NULL ) return;

//    el_SlabDispr_t * pSlabDispr = pDispr;
//    char MapSizeAlign4 = sizeof(pSlabDispr->bitmap);
//    ASSERT( MapSizeAlign4 % 4 == 0 );
//    memset((void *)pSlabDispr->bitmap,0,MapSizeAlign4);
//}

//void ELOS_SlabSDisprBitmapSetBit(  )
//{

//}
//void ELOS_SlabSDisprBitmapClearBit(  )
//{

//}

///* �ҵ���λ��λ���������� */
//EL_UINT ELOS_SlabSDisprBitmapFindBitAndSet(el_SlabDispr_t * pSlabDispr)
//{

//}

///* �ҵ���λ���㲢�������� */
//EL_UINT ELOS_SlabSDisprBitmapFindBitAndClear(el_SlabDispr_t * pSlabDispr,EL_INT index)
//{

//}
///* ת������ɫ�ڵ㣬����+1 */
//EL_CHAR ELOS_SlabColorConvDeepIdxTake(el_SlabDispr_t * pSlabDispr)
//{
//    return pSlabDispr
//}
///* ǳɫ�ڵ㣬����-1 */
//EL_CHAR ELOS_SlabColorConvSallowIdxTake(el_SlabDispr_t * pSlabDispr)
//{
//    return 
//}
//void SlabZoneResize(EL_UINT index,EL_UINT size)
//{
//    g_SlabPoolSize[index] = (index<=ELOS_SLAB_CHUNK_MAX_LEVEL-1)?\
//                                size:ELOS_PER_SLAB_ZONE_DATA_SIZE;
//}
//void SlabZoneResizeAll(EL_UINT size)
//{
//    for(int idx=0; idx<ELOS_SLAB_CHUNK_MAX_LEVEL-1; idx++)
//        SlabZoneResize(size);
//}
///* slab�������ʼ�� */
//void ELOS_SlabPoolClassInitialise(void *Slab_PoolSurf, EL_UINT Slab_PoolSize)
//{
//    el_SlabPoolHead_t *p_SlabPool = (el_SlabPoolHead_t *)0;
//    if(Slab_PoolSurf == NULL) return;

//    if(Slab_PoolSize < ELOS_SLAB_CHUNK_MAX_LEVEL*sizeof(el_SlabPoolHead_t))
//        return;

//    p_SlabPool = (el_SlabPoolHead_t *)Slab_PoolSurf;
//    OS_Enter_Critical_Check();
//    /* ��ʼ��slab����ͷ */
//    for( int idx = 0; idx < ELOS_SLAB_CHUNK_MAX_LEVEL; ++idx, p_SlabPool++ ){
//        SLAB_POOL_HEAD_INIT(p_SlabPool);
//        (*p_SlabPool).ckColorCnt = NULL;
//        (*p_SlabPool).SlabObjSize = ELOS_SLAB_BASIC_OBJECT_SIZE << 1;
//        (*p_SlabPool).SlabZoneSize = g_SlabPoolSize[idx];
//        (*p_SlabPool).SlabZoneMountedCnt = (EL_UINT)0;
//        ELOS_SlabDispenserAlloc((void *)p_SlabPool);
//        (*p_SlabPool).SlabZoneMountedCnt ++;
//    }

//    SlabZoneResizeAll(ELOS_PER_SLAB_ZONE_DATA_SIZE);
//    OS_Exit_Critical_Check();
//    return;
//}

///* ��slab������µ�slab������ */
//el_SlabDispr_t * ELOS_SlabDispenserAlloc(void *Slab_PoolSurf)
//{
//    el_SlabPoolHead_t *pSlabHead = (el_SlabPoolHead_t*)Slab_PoolSurf;
//    el_SlabDispr_t * SlabDisprToAlloc = NULL;

//    if(Slab_PoolSurf == NULL) return NULL;
//    EL_UINT ZoneSizeNeed = sizeof(el_SlabDispr_t) + ELOS_PER_SLAB_ZONE_DATA_SIZE;

//    OS_Enter_Critical_Check();
//    SlabDisprToAlloc = (el_SlabDispr_t *)malloc(ZoneSizeNeed);
//    if( SlabDisprToAlloc == NULL ) return (SlabDisprToAlloc *)0;

//    memset( (void *)SlabDisprToAlloc, 0, ZoneSizeNeed);
//    /* slab��������ʼ�� */
//    SLAB_DISPENSER_INIT(SlabDisprToAlloc);
//    /* ��ӵ�����ɫ���б� */
//    list_add_tail( &SlabDisprToAlloc->SlabNode, &pSlabHead->ckGreenListHead);
//    /* slab���ݳ���ָ���ʼ�� */
//    SlabDisprToAlloc->obj_pool_data = (void *)( SlabDisprToAlloc + 1 );
//    SlabDisprToAlloc->SlabObjCnt = pSlabHead->SlabZoneSize/pSlabHead->SlabObjSize;
//    SlabDisprToAlloc->SlabObjUsed = 0;
//    SlabDisprToAlloc->cr_type = SLAB_CR_GREEN;
//    SlabDisprToAlloc->NotifySlabCtrEvt = ELOS_SlabDisprEventCallBack;
//    OS_SLAB_DIPSR_OWNER_SET( SlabDisprToAlloc,pSlabHead );
//    //SlabDisprToAlloc->NotifyControlModifyCr = ELOS_SlabMvColorNodeToAnother;
//    ELOS_SlabDisprBitmapInitialise(SlabDisprToAlloc);
//    OS_Exit_Critical_Check();
//    
//    return SlabDisprToAlloc;
//}

///* ��slab���ͷ�һ��slab������ */
//EL_UINT * ELOS_SlabDispenserFree(el_SlabDispr_t * pSlabDispr)
//{
//    el_SlabPoolHead_t *pSlabHead;
//    el_SlabDispr_t * SlabDisprToFree = pSlabDispr;
//    if(pSlabDispr == NULL) return NULL;

//    OS_Enter_Critical_Check();
//    pSlabHead = (el_SlabPoolHead_t *)pSlabDispr->SlabOwner;
//    pSlabHead->SlabZoneMountedCnt --;
//    /* ����ɫ��ɾ�� */
//    list_del( (struct list_head *)&SlabDisprToFree->SlabNode );
//    if(SlabDisprToFree->cr_type == SLAB_CR_GREEN)
//        pSlabHead->GreenSlabCnt --;
//    else if(SlabDisprToFree->cr_type == SLAB_CR_YELLOW)
//        pSlabHead->YellowSlabCnt --;
//    else if (SlabDisprToFree->cr_type == SLAB_CR_RED)
//        pSlabHead->RedSlabCnt --;
//    free((void *)pSlabDispr);
//    OS_Exit_Critical_Check();

//    return (EL_UINT)EL_RESULT_OK;
//}

///* SLAB���ƿ��¼����� */
//EL_CHAR ELOS_SlabDisprEventCallBack(el_SlabPoolHead_t *pSlabHead,el_slab_callevt_t callevt_type)
//{
//    EL_CHAR ret = 0;
//    switch(callevt_type)
//    {
//        case CURRENT_SLAB_FULL:
//            /* ���������µ��ڴ�� */
//            if(!ELOS_SlabDispenserAlloc((void *)pSlabHead)) ret = -1;
//            break;
//        case MODIFY_SLAB_COLOR:
//            break;
//    }
//    return ret;
//}

///* ��slab��������µ�slab�� */
//EL_UINT ELOS_SlabGroupAdd(el_SlabPoolHead_t *pSlabHead, EL_UINT obj_size)
//{
//    el_SlabDispr_t * SlabDisprToAlloc = NULL;
//    for(int idx = 0 ; idx < ELOS_SLAB_CHUNK_MAX_LEVEL ; ++idx)
//    {
//        if( (pSlabHead ++)->SlabObjSize == obj_size){
//            pSlabHead --;
//            SlabDisprToAlloc = ELOS_SlabDispenserAlloc((void *)pSlabHead);
//        }
//    }
//    return (SlabDisprToAlloc == NULL)?EL_RESULT_ERR:EL_RESULT_OK;
//}

///* ��ɫ���б���� */
//EL_UINT ELOS_SlabMvColorNodeToAnother(el_SlabDispr_t * pSlabDispr,\
//                fELOS_SlabColorConvIdxTake SlabColorConvIdxTake)
//{
//    LIST_HEAD * pListNode;EL_CHAR cvIdx;
//    el_SlabPoolHead_t * pSlabDipsrOwner;
//    if(pSlabDispr == NULL)  return (EL_UINT)EL_RESULT_ERR;

//    pSlabDipsrOwner = OS_SLAB_DIPSR_OWNER_GET(pSlabDispr);/* ��ȡslab�������ĳ����� */

//    pListNode = pSlabDispr->SlabNode;/* slab�������ڵ� */

//    OS_Enter_Critical_Check();
//    /* ��ԭ��ɫ���б�ɾ�� */
//    list_del( pListNode );
//    cvIdx = SlabColorConvIdxTake(pSlabDispr);
//    /* ��ӵ��µ���ɫ���б� */
//    list_add_tail( pListNode, pSlabDipsrOwner->ckYellowListHead );
//    /* ������ɫ����Ŀ */
//    pSlabDispr->SlabObjUsed += cvIdx;
//    OS_Exit_Critical_Check();

//    return (EL_UINT)EL_RESULT_OK; 
//}

///* ��slab������һ��obj */
//void * ELOS_SlabObjAlloc(el_SlabDispr_t * pSlabDispr)
//{
//    void * pObjAlloced;
//    EL_UINT ObjIndex;
//    el_SlabPoolHead_t *pHead = (el_SlabPoolHead_t *)pSlabDispr->SlabOwner;
//    if( pSlabDispr == NULL ) return NULL;

//    OS_Enter_Critical_Check();
//    ASSERT( pSlabDispr->SlabObjCnt >= pSlabDispr->SlabObjUsed );
//    /* ��λͼ�����ҳ� */
//    ObjIndex = (pSlabDispr->SlabObjCnt == pSlabDispr->SlabObjUsed)?0:\
//                ELOS_SlabSDisprBitmapFindBitAndSet(pSlabDispr);
//    if(ObjIndex == (EL_UINT)0){
//        /* ���·���һ��slab���� */
//        //if( EL_RESULT_ERR == ELOS_SlabGroupAdd(el_SlabPoolHead_t *pSlabHead, EL_UINT obj_size)){
//        //    return NULL;
//        //}
//        if(0 == (void)SlabDisprToAlloc->NotifySlabCtrEvt(CURRENT_SLAB_FULL))
//            ELOS_SlabObjAlloc( el_SlabDispr_t * pSlabDispr);
//        else return NULL;
//    }
//    pObjAlloced = pSlabDispr->obj_pool_data + ObjIndex * pHead->SlabObjSize;
//    /* ������ɫ���б� */
//    ELOS_SlabMvColorNodeToAnother(pSlabDispr,ELOS_SlabColorConvDeepIdxTake);
//    OS_Exit_Critical_Check();

//    return pObjAlloced?pObjAlloced:NULL;
//}
///* ��slab���ͷ�һ��obj */
//void ELOS_SlabFreeObject(el_SlabDispr_t * pSlabDispr, void *pObj)
//{
//    EL_UINT ObjIndex;
//    el_SlabPoolHead_t *pHead;
//    if((pSlabDispr == NULL)||(pObj == NULL)) return;
//    pHead = (el_SlabPoolHead_t *)pSlabDispr->SlabOwner;
//    ObjIndex = ((EL_UINT)pObj - (EL_UINT)pSlabDispr->obj_pool_data)/pHead->SlabObjSize;

//    OS_Enter_Critical_Check();
//    ELOS_SlabSDisprBitmapFindBitAndClear( pSlabDispr,ObjIndex );
//    pSlabDispr->SlabObjUsed --;
//    /* ��ɫ���б���� */
//    pSlabDispr->NotifySlabCtrEvt(pHead,MODIFY_SLAB_COLOR);
//    OS_Exit_Critical_Check();
//}

///* ����һ��obj,����Ϊslab�����׵�ַ */
//void *ELOS_SlabMemAlloc(void * SlabPoolSurf,EL_UINT NeedAllocSize)
//{
//    el_SlabPoolHead_t *SlabHead = (el_SlabPoolHead_t *)SlabPoolSurf;
//    LIST_HEAD_CREAT( *pSlabZoneToAllocHeadNode );
//    void * pObj = NULL;
//    if( (SlabPoolSurf== NULL) || (NeedAllocSize == 0) ) return NULL;

//    /* ������С�����㷨 */
//    for(int s_idx = 0 ; s_idx < ELOS_SLAB_CHUNK_MAX_LEVEL; s_idx++){
//        SlabHead += s_idx;
//        /* ��֤�ڴ�ʹ���ʴ���50% */
//        if( (SlabHead->SlabObjSize >= NeedAllocSize) &&\
//             (SlabHead->SlabObjSize <= 2*NeedAllocSize) ){
//            /* ���ȴӻ������䣬�����������������ͷ����µ�slab���� */
//            pSlabZoneToAllocHeadNode = &SlabHead->ckYellowListHead;
//            if( list_empty( pSlabZoneToAllocHeadNode ) ){
//                /* ����û�оʹ��������� */
//                pSlabZoneToAllocHeadNode = &SlabHead->ckGreenListHead;
//                if( list_empty( pSlabZoneToAllocHeadNode ) ){
//                    /* û�������ʹ�ϵͳ�ڴ����� */
//                    if((EL_UINT)EL_RESULT_ERR ==  ELOS_SlabGroupAdd(SlabHead, SlabHead->SlabObjSize)){
//                        return (void *)0;
//                    }else{
//                        pObj = ELOS_SlabObjAlloc( SlabHead->ckGreenListHead );
//                        ASSERT( pObj != (void *)0 );
//                        return (void *)pObj;
//                    }
//                }
//            }
//            pObj = ELOS_SlabObjAlloc( SlabHead->ckYellowListHead );
//        }else continue;
//    }
//    return (void *)pObj;
//}

///* �ͷ�һ��obj */
//void ELOS_SlabMemFree(void *pObj)
//{
//    EL_UINT itemIndex;EL_UCHAR ** SlabDisprAddr;
//    if(pObj == NULL) return;
//    EL_UINT ObjIdx;
//    /* ��ȡobj����slab���ƿ飬slab�������׵�ַ */
//    ELOS_GetObjSlabDisprAddr(EL_UCHAR *plabDisprAddr,&ObjIdx);

//    //EL_UINT itemSize = pSlabHead->SlabObjSize;/* ��ȡobj��С */
//    //itemIndex = pSlabHead->SlabZoneSize/itemSize;/* ��ȡ��obj����λͼ������ */
//    ELOS_SlabMvColorNodeToAnother(pSlabDispr);
//}

///* ��obj��ȡ����slab��������ַ */
//void * ELOS_GetObjSlabDisprAddr(EL_UCHAR *plabDisprAddr,EL_UINT *pObjIdx)
//{
//    
//    *pObjIdx = 0;/* obj���ڵ�slab���� */
//}