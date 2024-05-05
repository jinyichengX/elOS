#ifndef EL_KOBJ_H
#define EL_KOBJ_H

#include <stddef.h>
#include "el_pthread.h"
#include "el_mutex_lock.h"
#include "el_speedpipe.h"
#include "el_ktmr.h"
#include "elightOS_config.h"
typedef enum{
#if EL_USE_THREAD_PENDING
    EL_KOBJ_TICKPENDING,
#endif
#if EL_USE_THREAD_SUSPEND
    EL_KOBJ_SUSPEND,
#endif
#if EL_USE_MUTEXLOCK
    EL_KOBJ_MUTEXLOCK,
#endif
#if EL_USE_SPEEDPIPE
    EL_KOBJ_SPEEDPIPE,
#endif
#if EL_USE_KTIMER
	EL_KOBJ_kTIMER,
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

#endif