#ifndef EL_KOBJ_H
#define EL_KOBJ_H

#include <stddef.h>
#include "el_pthread.h"
#include "el_mutex_lock.h"
#include "el_speedpipe.h"
#include "el_ktmr.h"
#include "el_sem.h"
#include "el_event_flag.h"
#include "elightOS_config.h"

typedef enum{
#if 1
	EL_KOBJ_PTCB,
#endif
#if EL_USE_THREAD_PENDING && THREAD_PENDING_OBJ_STATIC
    EL_KOBJ_TICKPENDING,
#endif
#if EL_USE_THREAD_SUSPEND && THREAD_SUSPEND_OBJ_STATIC
    EL_KOBJ_SUSPEND,
#endif
#if EL_USE_MUTEXLOCK 	  && MUTEXLOCK_OBJ_STATIC
    EL_KOBJ_MUTEXLOCK,
#endif
#if EL_USE_SPEEDPIPE	  && SPEEDPIPE_OBJ_STATIC
    EL_KOBJ_SPEEDPIPE,
#endif
#if EL_USE_KTIMER 		  && KTIMER_OBJ_STATIC
	EL_KOBJ_kTIMER,
#endif
#if EL_USE_SEM 		  	  && SEM_OBJ_STATIC
	EL_KOBJ_SEM,
#endif
#if EL_USE_EVENTFLAG 	  && EVENTFLAG_OBJ_STATIC
	EL_KOBJ_EVTFLG,
#endif
#if 1
    EL_KOBJ_TYPE_MAX
#endif
}EL_KOBJTYPE_T;

typedef struct stKobjSelfBaseInfo{
//	struct list_head kobj_entry;/* 内核对象列表入口 */
	struct{
		EL_KOBJTYPE_T Kobj_type;/* 内核对象类型 */
		EL_UCHAR Kobj_size;		/* 内核对象大小 */
	};
	struct{
		void * Kobj_Basepool;	/* 内核对象所在对象池 */
		EL_UINT Kobj_PoolSize;	/* 内核对象所需要的对象池大小 */
	};
}EL_kobj_info_t;

extern EL_RESULT_T ELOS_KobjStatisticsGet( EL_KOBJTYPE_T obj_type, EL_kobj_info_t * pobj );
extern void * ELOS_RequestForPoolWait(EL_KOBJTYPE_T kobj_type,EL_UINT ticks);
extern void * kobj_alloc(EL_KOBJTYPE_T kobj_type);
extern void kobj_free(EL_KOBJTYPE_T kobj_type,void * pobj_blk);
extern EL_kobj_info_t EL_Kobj_BasicInfoTable_t[];

#define KOBJ_INFO_MATCH(idx,start,end) for(idx = start; idx < end; idx++)

#endif