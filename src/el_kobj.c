#include "el_kobj.h"
#include "el_kheap.h"
#include "el_kpool.h"

#define KOBJ_PPOOL_BASE_INIT() 0

/* 内核对象初始化 */
#define KOBJ_BASE_INFO_INIT( kobj_type, kobj_struct, kobj_ppool, pool_size)\
		{\
		  .Kobj_type = (EL_KOBJTYPE_T)kobj_type, .Kobj_size = sizeof(kobj_struct),\
		  .Kobj_Basepool = (void *)kobj_ppool  , .Kobj_PoolSize = pool_size }

/* 内核对象基本信息 */
kobj_info_t EL_Kobj_BasicInfoTable_t[EL_KOBJ_TYPE_MAX] = {
#if 1
	KOBJ_BASE_INFO_INIT( EL_KOBJ_PTCB,		 	EL_PTCB_T,		KOBJ_PPOOL_BASE_INIT() , THREAD_POOLSIZE 	    ),
#endif
	
#if EL_USE_THREAD_PENDING && THREAD_PENDING_OBJ_STATIC
	KOBJ_BASE_INFO_INIT( EL_KOBJ_TICKPENDING,	TickPending_t,	KOBJ_PPOOL_BASE_INIT() , THREAD_PENDING_POOLSIZE),
#endif
	
#if EL_USE_THREAD_SUSPEND && THREAD_SUSPEND_OBJ_STATIC
	KOBJ_BASE_INFO_INIT( EL_KOBJ_SUSPEND,	 	Suspend_t,		KOBJ_PPOOL_BASE_INIT() , THREAD_SUSPEND_POOLSIZE),
#endif
	
#if EL_USE_MUTEXLOCK   	  && MUTEXLOCK_OBJ_STATIC
	KOBJ_BASE_INFO_INIT( EL_KOBJ_MUTEXLOCK,		mutex_lock_t,	KOBJ_PPOOL_BASE_INIT() , MUTEXLOCK_POOLSIZE 	),
#endif
	
#if EL_USE_MESSAGE_QUEUE  && MESSAGE_QUEUE_OBJ_STATIC
	KOBJ_BASE_INFO_INIT( EL_KOBJ_MESSAGE_QUEUE, msg_q_t, 		KOBJ_PPOOL_BASE_INIT() , MESSAGE_QUEUE_POOLSIZE ),
#endif
	
#if EL_USE_KTIMER 		  && KTIMER_OBJ_STATIC
	KOBJ_BASE_INFO_INIT( EL_KOBJ_kTIMER,	 	kTimer_t,		KOBJ_PPOOL_BASE_INIT() , KTIMER_POOLSIZE 		),
#endif

#if EL_USE_SEM 		  	  && SEM_OBJ_STATIC
	KOBJ_BASE_INFO_INIT( EL_KOBJ_SEM,	 	 	el_sem_t,		KOBJ_PPOOL_BASE_INIT() , SEM_POOLSIZE 		    ),
#endif

#if EL_USE_EVENTFLAG 	  && EVENTFLAG_OBJ_STATIC
	KOBJ_BASE_INFO_INIT( EL_KOBJ_EVTFLG,	 	evtflg_t,		KOBJ_PPOOL_BASE_INIT() , EVENTFLAG_POOLSIZE 	),
#endif
};

/* 获取内核对象基本信息 */
EL_RESULT_T kobj_check( EL_KOBJTYPE_T obj_type, kobj_info_t * pobj )
{
	kobj_info_t * kobj_info;
	if((pobj == (kobj_info_t *)0)||((EL_KOBJTYPE_T)obj_type >= EL_KOBJ_TYPE_MAX))
			return EL_RESULT_ERR;
	
	kobj_info = &(EL_Kobj_BasicInfoTable_t[ (EL_KOBJTYPE_T)obj_type ]);
	/* 匹配表 */
	if( kobj_info->Kobj_type == obj_type){
		pobj->Kobj_Basepool = kobj_info->Kobj_Basepool;
		pobj->Kobj_size = kobj_info->Kobj_size;
		pobj->Kobj_type = kobj_info->Kobj_type;
	}
	else return EL_RESULT_ERR;
	
	return EL_RESULT_OK;
}

/* 为内核对象分配对象池 */
void * kobj_pool_request(EL_KOBJTYPE_T kobj_type,EL_UINT ticks)
{
	kobj_info_t * kobj_info;
	if( kobj_type >= EL_KOBJ_TYPE_MAX ) return (void *)0;
	
	kobj_info = &(EL_Kobj_BasicInfoTable_t[ (EL_KOBJTYPE_T)kobj_type ]);
	if( kobj_info->Kobj_Basepool != NULL ){
		return (void *)kobj_info->Kobj_Basepool;
	}
	kobj_info->Kobj_Basepool = (hp_alloc( sysheap_hcb, kobj_info->Kobj_PoolSize ));
	/* 初始化对象池 */
	EL_stKpoolInitialise(kobj_info->Kobj_Basepool,kobj_info->Kobj_PoolSize,kobj_info->Kobj_size);
	return kobj_info->Kobj_Basepool;
}

/* 为内核对象分配一个块 */
void * kobj_alloc(EL_KOBJTYPE_T kobj_type)
{
	void * pool;
	if((EL_KOBJTYPE_T)kobj_type >= EL_KOBJ_TYPE_MAX)
			return NULL;
	
	pool = EL_Kobj_BasicInfoTable_t[kobj_type].Kobj_Basepool;
	if(pool == NULL) return NULL;
	return EL_stKpoolBlockTryAlloc(pool);
}

/* 从内核对象池中释放一个块 */
void kobj_free(EL_KOBJTYPE_T kobj_type,void * pobj_blk)
{
	void * pool;
	if(((EL_KOBJTYPE_T)kobj_type >= EL_KOBJ_TYPE_MAX) || (pobj_blk == NULL)) return;
	pool = EL_Kobj_BasicInfoTable_t[kobj_type].Kobj_Basepool;
	
	if(pool == NULL) return;
	EL_stKpoolBlockFree(pool,pobj_blk);
	
}