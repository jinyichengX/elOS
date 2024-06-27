#ifndef EL_KTMR_H
#define EL_KTMR_H

#include "el_klist.h"
#include "el_avl.h"
#include "el_type.h"
#include "el_pthread.h"
#include "elightOS_config.h"
#include "el_kobj.h"
#include "el_kpool_static.h"
#include "el_kheap.h"
#include "el_sem.h"

#if EL_USE_KTIMER
/* 放置定时器对象的容器类型 */
#define ktmr_mng_container_t avl_t

typedef void (*pTmrCallBackEntry)(void *args);

typedef enum
{
	period,	/* 周期定时器 */
	count,	/* 计时定时器 */
}ktmr_type;

typedef struct stKernelTimer{
	ktmr_type type;						/* 定时器类型 */
	char period_timeout_type;		/* 活动开始周期型，活动结束周期型 */
	uint32_t timeout_tick;				/* 周期/计时周期 */
	uint32_t sched_cnt;					/* 周期触发/单次触发（对连续计时型无效） */
	EL_G_SYSTICK_TYPE schdline; 		/* 定时器活动开始时限 */
	EL_G_SYSTICK_TYPE deadline; 		/* 计时型定时器活动截止时限 */
	pTmrCallBackEntry CallBackFunEntry;	/* 定时器回调函数 */
	void * callback_args;				/* 用户参数 */
	avl_node_t hook;
	char activing;						/* 活动标志 */
#if KTMR_THREAD_SHARE == 0
	EL_PTCB_T * tmr_thread;
#endif
}kTimer_t;

#ifndef IDX_ZERO 
#define IDX_ZERO 0
#endif

#ifndef KTMR_SCHED_THREAD_STACK_SIZE
#define KTMR_SCHED_THREAD_STACK_SIZE 256
#endif

#define KTMR_NACTIVING 0
#define KTMR_YACTIVING 1

extern EL_RESULT_T ktmr_module_init(void);
extern kTimer_t * kTimerCreate(const char * name,\
				EL_UINT stack_size,\
				ktmr_type type,\
				char sub_type,\
				uint32_t timeout_tick,\
				uint32_t time_cnt,\
				pTmrCallBackEntry usr_callback,\
				void * args);
				extern EL_RESULT_T kTimerStart(kTimer_t * tmr,sys_tick_t count_down);
extern EL_RESULT_T kTimerStart(kTimer_t * tmr,sys_tick_t count_down);
extern void kTimerStop(kTimer_t * tmr);
#endif
#endif




