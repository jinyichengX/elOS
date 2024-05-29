#include "el_kobj.h"
#include "el_kheap.h"
#include "el_kpool_static.h"

#define KOBJ_PPOOL_BASE() 0

/* 内核对象初始化 */
#define KOBJ_BASE_INFO_INIT( kobj_type, kobj_struct, kobj_ppool, pool_size)\
		{\
		  .Kobj_type = (EL_KOBJTYPE_T)kobj_type, .Kobj_size = sizeof(kobj_struct),\
		  .Kobj_Basepool = (void *)kobj_ppool  , .Kobj_PoolSize = pool_size }

/* 内核对象基本信息 */
static EL_kobj_info_t EL_Kobj_BasicInfoTable_t[] = {
	KOBJ_BASE_INFO_INIT( EL_KOBJ_PTCB,		 EL_PTCB_T,		KOBJ_PPOOL_BASE() , THREAD_POOLSIZE 	),
#if EL_USE_THREAD_PENDING && THREAD_PENDING_OBJ_STATIC
	KOBJ_BASE_INFO_INIT( EL_KOBJ_TICKPENDING,TickPending_t,	KOBJ_PPOOL_BASE() , THREAD_PENDING_POOLSIZE 	),
#endif
#if EL_USE_THREAD_SUSPEND && THREAD_SUSPEND_OBJ_STATIC
	KOBJ_BASE_INFO_INIT( EL_KOBJ_SUSPEND,	 Suspend_t,		KOBJ_PPOOL_BASE() , THREAD_SUSPEND_POOLSIZE ),
#endif
#if EL_USE_MUTEXLOCK   	  && MUTEXLOCK_OBJ_STATIC
	KOBJ_BASE_INFO_INIT( EL_KOBJ_MUTEXLOCK,	 mutex_lock_t,	KOBJ_PPOOL_BASE() , MUTEXLOCK_POOLSIZE 		),
#endif
#if EL_USE_SPEEDPIPE	  && SPEEDPIPE_OBJ_STATIC
	KOBJ_BASE_INFO_INIT( EL_KOBJ_SPEEDPIPE,	 speed_pipe_t,	KOBJ_PPOOL_BASE() , SPEEDPIPE_POOLSIZE 		),
#endif
#if EL_USE_KTIMER 		  && KTIMER_OBJ_STATIC
	KOBJ_BASE_INFO_INIT( EL_KOBJ_kTIMER,	 kTimer_t,		KOBJ_PPOOL_BASE() , KTIMER_POOLSIZE 		),
#endif
};

void * heap_belong = NULL;

/* 获取内核对象基本信息 */
EL_RESULT_T ELOS_KobjStatisticsGet( EL_KOBJTYPE_T obj_type, EL_kobj_info_t * pobj )
{
	EL_kobj_info_t * kobj_info;
	if((pobj == (EL_kobj_info_t *)0)||((EL_KOBJTYPE_T)obj_type >= EL_KOBJ_TYPE_MAX))
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
static void * ELOS_RequestForPoolWait(EL_KOBJTYPE_T kobj_type,EL_UINT ticks)
{
	EL_kobj_info_t * kobj_info;
	if( kobj_type >= EL_KOBJ_TYPE_MAX ) return (void *)0;
	
	kobj_info = &(EL_Kobj_BasicInfoTable_t[ (EL_KOBJTYPE_T)kobj_type ]);
	if( kobj_info->Kobj_Basepool != NULL ){
		return (void *)kobj_info->Kobj_Basepool;
	}
//	return (hp_alloc( (hcb_t *) heap_belong, kobj_info->Kobj_PoolSize ));
	return NULL;
}

