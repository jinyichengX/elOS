#include "el_event_flag.h"
#include "kparam.h"

#if EL_USE_EVENTFLAG

extern EL_G_SYSTICK_TYPE systick_get(void);
extern EL_G_SYSTICK_TYPE systick_passed(EL_G_SYSTICK_TYPE last_tick);

/* 初始化事件标志 */
void el_EventFlagInitialise(evtflg_t * evtflg)
{
	evtflg->evt_bits = 0;
	INIT_LIST_HEAD(&evtflg->waiters);
}

#if EVENTFLAG_OBJ_STATIC == 1
/* 创建事件标志组 */
evtflg_t * el_EventFlagCreate(void)
{
	EL_kobj_info_t kobj;
	evtflg_t * evtflg = NULL;
	
	OS_Enter_Critical_Check();
	/* 检查对象池是否支持事件标志 */
	if(EL_RESULT_OK != ELOS_KobjStatisticsGet( EL_KOBJ_EVTFLG,&kobj))
		return NULL;
	/* 分配一个内核定时器 */
	evtflg = (void *)kobj_alloc( EL_KOBJ_EVTFLG );
	OS_Exit_Critical_Check();
	if(NULL == evtflg) return NULL;
	/* 初始化事件标志 */
	el_EventFlagInitialise(evtflg);
	
	return evtflg;
}

/* 删除事件标志 */
EL_RESULT_T el_EventFlagrDestroy(evtflg_t * evtflg)
{
	EL_PTCB_T *ptcb = NULL;
	struct list_head *pos;
	if(evtflg == NULL) return EL_RESULT_ERR;
	
	OS_Enter_Critical_Check();
	/* 唤醒所有被阻塞的线程 */
	while(!list_empty(&evtflg->waiters))
	{
		pos = evtflg->waiters.next;
		ptcb = container_of(pos,EL_PTCB_T,pthread_node);
		EL_Pthread_EventWakeup(ptcb);
		list_del(pos);
	}
	/* 内存池回收 */
	kobj_free(EL_KOBJ_EVTFLG,(void * )evtflg);
	OS_Exit_Critical_Check();
	
	return EL_RESULT_OK;
}
#endif

/* 设置事件标志 */
void el_EventFlagSetBits(evtflg_t * evtflg,uint32_t flags)
{
	char need_sched = 0;
	struct list_head *pos,*next;
	EL_PTCB_T *ptcb,*cur_ptcb = NULL;
	if((evtflg == NULL) || (flags == 0))
		return;
	OS_Enter_Critical_Check();
	
	cur_ptcb = EL_GET_CUR_PTHREAD();
	evtflg->evt_bits |= flags;
	/* 唤醒所有等待此事件标志的任务 */
	list_for_each_safe(pos,next,&evtflg->waiters)
	{
		ptcb = container_of(pos,EL_PTCB_T,pthread_node);
		/* 阻塞的线程等等待的标志是否满足 */
		if(
		((WAITBITS_AND == ptcb->andor)&&((evtflg->evt_bits & ptcb->event_wait) == ptcb->event_wait))
		||(((WAITBITS_OR == ptcb->andor)) && (evtflg->evt_bits & ptcb->event_wait))
		){
			list_del(&ptcb->pthread_node);
			EL_Pthread_EventWakeup(ptcb);
			/* 如果被唤醒的线程优先级更高执行一次线程切换 */
			if( cur_ptcb->pthread_prio < ptcb->pthread_prio ) 
				need_sched = 1;
		}
	}
	OS_Exit_Critical_Check();
	if( need_sched )
		PORT_PendSV_Suspend();
}

/* 清除事件标志位 */
void el_EventFlagClearBits(evtflg_t * evtflg, uint32_t flags)
{
	if((evtflg == NULL) || (flags == 0))
		return;
    OS_Enter_Critical_Check();
    evtflg->evt_bits &= ~flags;
    OS_Exit_Critical_Check();
}

/* 等待事件标志 */
uint32_t el_EventFlagWaitBits(evtflg_t * evtflg,uint32_t flags,\
							bool clear,andor_t andor,uint32_t tick)
{
	EL_PTCB_T *cur_ptcb = NULL;
	Suspend_t *SuspendObj;
	uint32_t ret = 0;
	int evt_ret = (int)EL_RESULT_ERR;
	EL_G_SYSTICK_TYPE tick_line = systick_get()+tick;
	EL_G_SYSTICK_TYPE tick_now;
	if((evtflg == NULL) || (flags == 0))
		return 0;
	cur_ptcb = EL_GET_CUR_PTHREAD();

	OS_Enter_Critical_Check();
	if(
		!(((WAITBITS_AND == andor)&&((evtflg->evt_bits & flags) == flags))
		||(((WAITBITS_OR == andor)) && (evtflg->evt_bits & flags)))
	   )
	{
		if(!tick){
			OS_Exit_Critical_Check();
			return ret;
		}
		cur_ptcb->event_wait = flags;
		cur_ptcb->andor = andor;
		if(tick != _MAX_TICKS_TO_WAIT){
			while(!(
			((WAITBITS_AND == andor)&&((evtflg->evt_bits & flags) == flags))
			||(((WAITBITS_OR == andor)) && (evtflg->evt_bits & flags))
			)){
				if(!tick){
					OS_Exit_Critical_Check();
					return ret;
				}
				/* 将线程钩子节点添加到信号量等待队列 */
				list_add_tail(&cur_ptcb->pthread_node,&evtflg->waiters);
				/* 注册等待事件 */
				EL_Pthread_EventWait(cur_ptcb,EVENT_TYPE_EVTFLAG_WAIT,tick,&evt_ret);
				OS_Exit_Critical_Check();
				PORT_PendSV_Suspend();
				OS_Enter_Critical_Check();
				tick_now = systick_get();
				tick = (tick_line < tick_now)?0:(tick_line - tick_now);
			}
		}else{
			while(!(
				((WAITBITS_AND == andor)&&((evtflg->evt_bits & flags) == flags))
				||(((WAITBITS_OR == andor)) && (evtflg->evt_bits & flags))
				))
			{
			   /* 挂起当前线程 */
			   if (NULL == (SuspendObj = (Suspend_t *)kobj_alloc(EL_KOBJ_SUSPEND))){
					OS_Exit_Critical_Check();
					return EL_RESULT_ERR;
			   }
			   /* 将线程放入等待队列 */
			   list_add_tail(&cur_ptcb->pthread_node,&evtflg->waiters);
			   SuspendObj->Pthread = cur_ptcb;
			   SuspendObj->PendingType = (void *)0;
			   cur_ptcb->block_holder = (void *)SuspendObj;
			   EL_Pthread_PushToSuspendList(cur_ptcb,&SuspendObj->Suspend_Node);

			   OS_Exit_Critical_Check();
			   PORT_PendSV_Suspend();
			   OS_Enter_Critical_Check();
			}
		}
	}
	ret = evtflg->evt_bits;
	if(clear == true) el_EventFlagClearBits(evtflg, flags);
	OS_Exit_Critical_Check();
	
	return ret;
}

#endif