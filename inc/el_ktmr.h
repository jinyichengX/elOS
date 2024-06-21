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
/* 软件定时器类型 */
/* 1.连续计时型 （1）首尾单次触发执行/结束型 （2）连续运行型①带返回②不带返回 */
/* 2.周期计数型 （1）周期内必须执行完 （2）执行完等待固定周期 */

/* 放置定时器对象的容器 */
#define ktmr_mng_container_t avl_t

typedef void (*pTmrCallBackEntry)(void *args);

typedef enum
{
	period,	/* 周期定时器 */
	count,	/* 计时定时器 */
}ktmr_type;

typedef struct stKernelTimer{
	ktmr_type type;						/* 定时器类型 */
	uint32_t period_timeout_type;		
	uint32_t timeout_tick;				/*  */
	EL_G_SYSTICK_TYPE schdline; 	/* 定时器活动开始时限 */
	EL_G_SYSTICK_TYPE deadline; 	/* 连续型定时器活动截止时限 */
	pTmrCallBackEntry CallBackFunEntry;	/* 定时器回调函数 */
	void * callback_args;			/* 用户参数 */
}kTimer_t;

#ifndef IDX_ZERO 
#define IDX_ZERO 0
#endif

extern void ktmr_module_init(void);

#endif




