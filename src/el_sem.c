#include "el_sem.h"
#include "kparam.h"

extern EL_G_SYSTICK_TYPE systick_get(void);
extern EL_G_SYSTICK_TYPE systick_passed(EL_G_SYSTICK_TYPE last_tick);

/**********************************************************************
 * 函数名称： el_sem_init
 * 功能描述： 初始化信号量
 * 输入参数： sem : 已创建的信号量对象
             value : 信号量初始计数
 * 输出参数： 无
 * 返 回 值： 无
 * 修改日期        版本号     修改人	      修改内容
 * -----------------------------------------------
 * 2024/06/11	    V1.0	  jinyicheng	      创建
 ***********************************************************************/
void el_sem_init(el_sem_t * sem,uint32_t value)
{
	INIT_LIST_HEAD(&sem->waiters);
	sem->value = value;
}

/**********************************************************************
 * 函数名称： el_sem_create
 * 功能描述： 创建并初始化信号量
 * 输入参数： value : 信号量初始计数
 * 输出参数： 无
 * 返 回 值： 信号量句柄
 * 修改日期        版本号     修改人	      修改内容
 * -----------------------------------------------
 * 2024/06/11	    V1.0	  jinyicheng	      创建
 ***********************************************************************/
#if SEM_OBJ_STATIC == 1
el_sem_t * el_sem_create(uint32_t value)
{
	kobj_info_t kobj;
	el_sem_t * sem = NULL;
	
	OS_Enter_Critical_Check();
	/* 检查对象池是否支持事件标志 */
	if(EL_RESULT_OK != kobj_check( EL_KOBJ_SEM,&kobj)){
		OS_Exit_Critical_Check();
		return NULL;
	}
	sem = (el_sem_t * )kobj_alloc( EL_KOBJ_SEM );
	
	if(NULL == sem){
		OS_Exit_Critical_Check();
		return NULL; 
	}
	OS_Exit_Critical_Check();
	el_sem_init(sem,value);
	
	return sem;
}
#endif

/**********************************************************************
 * 函数名称： el_sem_take
 * 功能描述： 阻塞获取信号量
 * 输入参数： sem : 已创建的信号量对象
             tick : 等待的时基数
 * 输出参数： 无
 * 返 回 值： ok/nok
 * 修改日期        版本号     修改人	      修改内容
 * -----------------------------------------------
 * 2024/06/11	    V1.0	  jinyicheng	      创建
 ***********************************************************************/
EL_RESULT_T el_sem_take(el_sem_t *sem,uint32_t tick)
{
	int ret = (int)EL_RESULT_ERR;
	EL_PTCB_T *ptcb = NULL;
	Suspend_t *SuspendObj;
	EL_G_SYSTICK_TYPE tick_line = systick_get()+tick;
	EL_G_SYSTICK_TYPE tick_now;
	if( sem == NULL ) return EL_RESULT_ERR;
	
	OS_Enter_Critical_Check();
	ptcb = EL_GET_CUR_PTHREAD();
	if(sem->value){
	 sem->value --;
	 OS_Exit_Critical_Check();
	 return EL_RESULT_OK;
	}
	else{
	 if(tick != _MAX_TICKS_TO_WAIT){
	  while(!sem->value){
	   if(!tick){
		OS_Exit_Critical_Check();
		return EL_RESULT_ERR;
	   }
	   /* 将线程钩子节点添加到信号量等待队列 */
	   list_add_tail(&ptcb->pthread_node,&sem->waiters);
	   /* 注册等待事件 */
	   EL_Pthread_EventWait(ptcb,EVENT_TYPE_SEM_WAIT,tick,&ret);
	 
	   OS_Exit_Critical_Check();
	   PORT_PendSV_Suspend();
	   OS_Enter_Critical_Check();
	   if((ret == (int)EL_RESULT_OK) && (sem->value))
		sem->value --;
	   else{
	    tick_now = systick_get();
	    tick = (tick_line < tick_now)?0:(tick_line - tick_now);
	    ret = (int)EL_RESULT_ERR;
	    continue;
	   }
	   OS_Exit_Critical_Check();
	
	   return (EL_RESULT_T)ret;
	  }
	 }
	 else{
	  while(!sem->value)
	  {
	   ASSERT(ptcb->pthread_state != EL_PTHREAD_SUSPEND);

	   /* 挂起当前线程 */
	   if (NULL == (SuspendObj = (Suspend_t *)kobj_alloc(EL_KOBJ_SUSPEND))){
	    OS_Exit_Critical_Check();
		return EL_RESULT_ERR;
	   }
	   /* 将线程放入等待队列 */
	   list_add_tail(&ptcb->pthread_node,&sem->waiters);
	   SuspendObj->Pthread = ptcb;
	   SuspendObj->PendingType = (void *)0;
	   ptcb->block_holder = (void *)SuspendObj;
	   EL_Pthread_PushToSuspendList(ptcb,\
				&SuspendObj->Suspend_Node);

	   OS_Exit_Critical_Check();
	   /* 执行一次线程切换 */
	   PORT_PendSV_Suspend();
	   /* 当线程恢复运行后要先进入临界区 */
	   OS_Enter_Critical_Check();
	  }
	  sem->value --;
	  OS_Exit_Critical_Check();

	  return EL_RESULT_OK;
	 }
	}
}

/**********************************************************************
 * 函数名称： el_sem_trytake
 * 功能描述： 尝试获取信号量
 * 输入参数： sem : 已创建的信号量对象
 * 输出参数： 无
 * 返 回 值： ok/nok
 * 修改日期        版本号     修改人	      修改内容
 * -----------------------------------------------
 * 2024/06/11	    V1.0	  jinyicheng	      创建
 ***********************************************************************/
EL_RESULT_T el_sem_trytake(el_sem_t *sem)
{
	if( sem == NULL ) return EL_RESULT_ERR;
	
	OS_Enter_Critical_Check();
	if(!sem->value){
		OS_Exit_Critical_Check();
		return EL_RESULT_ERR;
	}
	sem->value --;
	OS_Exit_Critical_Check();
	
	return EL_RESULT_OK;
}

/**********************************************************************
 * 函数名称： el_sem_release
 * 功能描述： 释放信号量
 * 输入参数： sem : 已创建的信号量对象
 * 输出参数： 无
 * 返 回 值： 无
 * 修改日期        版本号     修改人	      修改内容
 * -----------------------------------------------
 * 2024/06/11	    V1.0	  jinyicheng	      创建
 ***********************************************************************/
void el_sem_release(el_sem_t *sem)
{
	char need_sched = 0;
	EL_PTCB_T *ptcb,*cur_ptcb = NULL;
	if( sem == NULL ) return;

	OS_Enter_Critical_Check();
	cur_ptcb = EL_GET_CUR_PTHREAD();
	if(sem->value < 0xffffffff) sem->value ++;
	else return;
	/* 是否有线程等待此信号量 */
	if( !list_empty( &sem->waiters ) ){
	 /* 获取第一个等待此信号量的线程 */
	 ptcb = (EL_PTCB_T *)sem->waiters.next;

	 /* 从等待队列删除等待信号量的线程钩子节点 */
	 list_del(&ptcb->pthread_node);
	 /* 唤醒第一个等待此信号量的线程 */
	 EL_Pthread_EventWakeup(ptcb);
	 /* 如果被唤醒的线程优先级更高执行一次线程切换 */
	 if( cur_ptcb->pthread_prio < ptcb->pthread_prio ) 
	  need_sched = 1;
	}
	OS_Exit_Critical_Check();
	
	if( need_sched )
	 PORT_PendSV_Suspend();
}

/**********************************************************************
 * 函数名称： el_sem_StatisticsTake
 * 功能描述： 信号量状态信息统计
 * 输入参数： sem : 已创建的信号量对象
			  val：写入计数值的首地址
 * 输出参数： 无
 * 返 回 值： ok/nok
 * 修改日期        版本号     修改人	      修改内容
 * -----------------------------------------------
 * 2024/06/11	    V1.0	  jinyicheng	      创建
 ***********************************************************************/
EL_RESULT_T el_sem_StatisticsTake(el_sem_t *sem,uint32_t * val)
{
	if( sem == NULL ) return EL_RESULT_ERR;
	
	OS_Enter_Critical_Check();
	if( val != NULL ) *val = sem->value;
	OS_Exit_Critical_Check();
	
	return EL_RESULT_OK;
}