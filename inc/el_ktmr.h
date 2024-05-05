#ifndef EL_KTMR_H
#define EL_KTMR_H

#include "el_klist.h"
#include "el_type.h"
#include "el_pthread.h"

/* 软件定时器类型 */
/* 1.连续计时型 （1）首尾单次触发执行/结束型 （2）连续运行型①带返回②不带返回 */
/* 2.周期计数型 （1）周期内必须执行完 （2）执行完等待固定周期 */

typedef void (*pTmrCallBackEntry)(void *args);

typedef struct stKernelTimer{
	struct list_head ktmr_node;		/* 定时器列表入口 */
	EL_G_SYSTICK_TYPE schdline; 	/* 定时器活动开始时限 */
	EL_G_SYSTICK_TYPE deadline; 	/* 连续型定时器活动截止时限 */
	pTmrCallBackEntry CallBackFunEntry;	/* 定时器回调函数 */
	void * callback_args;			/* 用户参数 */
}kTimer_t;

#endif




