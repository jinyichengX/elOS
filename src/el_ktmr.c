/* 内核定时器 */

/* 内核定时器任务调度策略 */
/* 1.多定时任务共享一个调度周期，由定时器调度任务统一管理 */
/* 2.每个定时任务分散调度，每个定时任务都分配调度周期 */

/* 软件定时器类型 */
/* 1.连续计时型 （1）首尾单次触发执行/结束型 （2）连续运行型①带返回②不带返回 */
/* 2.周期计数型 （1）周期内必须执行完 （2）执行完等待固定周期 */

/* 内核定时器使用链表/跳表/AVL树/红黑树管理 */
#include "el_ktmr.h"

void kTimerCreate(void)
{
	
}

void kTimerInitialise( kTimer_t * ptmr_inst, pTmrCallBackEntry usr_callback )
{
	if(ptmr_inst == NULL) return;
	
	ptmr_inst -> CallBackFunEntry = usr_callback;
}
