/* 内核定时器 */

/* 内核定时器任务调度策略
1.多定时任务共享一个调度周期，由中央调度线程统一管理（占用内存小但定时器响应度低，执行周期精度较低）
2.每个定时任务分散调度，方式①：每个定时任务都分配调度周期自行调度（定时器响应度高但占用内存大，执行周期精度较高）
 						 方式②：由中央调度线程统一管理每个定时线程的调度（定时器响应度高但占用内存大，执行周期精度较高）
*/

/* 内核定时器使用链表/跳表/AVL树/红黑树管理 */
#include "el_ktmr.h"

extern EL_G_SYSTICK_TYPE systick_get(void);
#if KTMR_THREAD_SHARE == 0
el_sem_t g_kmtr_sem;
#endif
/* 钩子节点偏移量 */
#define HOOK_OFFSET ((size_t)&((kTimer_t *)0)->hook)

char compare_timeout_tick(void *tmr1,void * tmr2)
{
	kTimer_t * tmr_cmp1 = (kTimer_t *)tmr1;
	kTimer_t * tmr_cmp2 = (kTimer_t *)tmr2;
	return (tmr_cmp1->schdline < tmr_cmp2->schdline)?-1:\
		   ((tmr_cmp1->schdline == tmr_cmp2->schdline)?0:1);
}

/* 全局的定时器容器 */
static ktmr_mng_container_t ktmr_container;
/* 访问定时器容器的信号量 */
static el_sem_t sem_ktmrCont;
/* 初始化定时器容器 */
#define KTMR_CONTAINER_INIT(pcont) 				avl_Initialise((pcont),compare_timeout_tick,HOOK_OFFSET);
/* 向定时器容器中添加定时器 */
#define KTMR_CONTAINER_ADD(pktmr,pcont)  		g_avl_node_add((void *)(pktmr),(ktmr_mng_container_t *)(pcont))
/* 从定时器容器中删除定时器 */
#define KTMR_CONTAINER_REMOVE(pktmr,pcont)  	avl_node_delete(_AVL_CONTAINER2NODE((pktmr),HOOK_OFFSET),(ktmr_mng_container_t *)(pcont))
/* 查找已经超时的定时器 */
static inline kTimer_t * KTMR_TIMEOUT_SEARCH(ktmr_mng_container_t * pcont)
{
	kTimer_t * tmr;
	avl_node_t * node = avl_find_first_node(pcont);
	if(node == NULL) return NULL;
	tmr = (kTimer_t *)_AVL_NODE2CONTAINER(node,pcont->node_off);
	return (tmr->schdline <= systick_get())?tmr:NULL;
}

/* 查找即将超时的定时器 */
#define KTMR_TIMEOUT_SOON_SEARCH(pcont)			KTMR_TIMEOUT_SEARCH((ktmr_mng_container_t *)(pcont))
/* 定时器活动标志查询 */
#define KTMR_ACTIVING(pktmr) 					(((kTimer_t *)(pktmr))->activing)

#if KTMR_THREAD_SHARE == 0
/* 等待通知 */
#define KTMR_SCHED_THREAD_NOTIFY()
#endif

void pthread_ktmr_sched( void * arg );
void kTimerStop(kTimer_t * tmr);

/* 创建一个线程管理多个定时器 */
static EL_RESULT_T kTimerManageThreadCreate(EL_UINT stack_size)
{
	static EL_PTCB_T ktmr_sched;
	EL_RESULT_T ret;

	ret = EL_Pthread_Create(&ktmr_sched,"ktmr_sched",pthread_ktmr_sched,stack_size,0,NULL);
	return ret;
}		
/* 定时器模块初始化 */
EL_RESULT_T ktmr_module_init(void)
{
	KTMR_CONTAINER_INIT(&ktmr_container);
	el_sem_init(&sem_ktmrCont, 1);
	/* 创建定时器调度线程 */
	return kTimerManageThreadCreate(KTMR_SCHED_THREAD_STACK_SIZE);
}

/* 创建定时器错误处理 */
static inline void ktmr_create_err_handle(void * kobj_pool,kTimer_t *tmr)
{
	EL_stKpoolBlockFree((void *)kobj_pool,(void *)tmr);
}

/* 初始化定时器 */
void kTimerInitialise( kTimer_t * tmr,\
						ktmr_type type,\
						char sub_type,\
						uint32_t timeout_tick,\
						uint32_t time_cnt,\
						pTmrCallBackEntry usr_callback,\
						void * args)
{
	if(tmr == NULL) return;
	if(usr_callback == NULL) return;
	
	tmr->activing = KTMR_NACTIVING;
	tmr->type = type;						/* 周期or计时 */
	tmr->period_timeout_type = sub_type;	/* 周期型子类型 */
	tmr->timeout_tick = timeout_tick;		/* 周期或计时周期 */
	tmr->sched_cnt = time_cnt;				/* 周期计数 */
	tmr->CallBackFunEntry = usr_callback;	/* 回调函数/定时器线程 */
	tmr->schdline = tmr->deadline = 0;
	tmr->callback_args = args;
}

/* 创建定时器 */
#if KTIMER_OBJ_STATIC == 1
kTimer_t * 
kTimerCreate(const char * name,\
				EL_UINT stack_size,\
				ktmr_type type,\
				char sub_type,\
				uint32_t timeout_tick,\
				uint32_t time_cnt,\
				pTmrCallBackEntry usr_callback,\
				void * args)
{
	int index;
	EL_kobj_info_t kobj;
	EL_PTCB_T * tmr_thread;
	kTimer_t * ktmr = NULL;
	if( usr_callback == NULL ) return NULL;
	
	OS_Enter_Critical_Check();
	/* 检查对象池是否支持内核定时器 */
	if(EL_RESULT_OK != ELOS_KobjStatisticsGet( EL_KOBJ_kTIMER,&kobj))
		return NULL;
	/* 分配一个内核定时器 */
	ktmr = (void *)kobj_alloc( EL_KOBJ_kTIMER );
	if(NULL == ktmr) return NULL;
	/* 初始化定时器 */
	kTimerInitialise(ktmr,type,sub_type,timeout_tick,time_cnt,usr_callback,args);
	
#if KTMR_THREAD_SHARE == 0/* 多定时器共享线程 */

	/* 从内核对象池取线程控制块 */
	tmr_thread = (EL_PTCB_T * )kobj_alloc(EL_KOBJ_PTCB);
	if( tmr_thread == NULL ) {
		ktmr_create_err_handle((void *)kobj.Kobj_Basepool,(void *)ktmr);
		OS_Exit_Critical_Check();
		return NULL;
	}
	/* 创建定时器线程，优先级最高 */
	if(EL_RESULT_ERR ==  EL_Pthread_Create( tmr_thread, name, usr_callback,stack_size,0,ktmr->callback_args))
	{
		/* 释放定时器对象 */
		ktmr_create_err_handle((void *)kobj.Kobj_Basepool,(void *)ktmr);
		/* 释放线程对象 */
		kobj_free( EL_KOBJ_PTCB ,(void *)tmr_thread );
		OS_Exit_Critical_Check();
		return NULL;
	}
	/* 挂起定时器线程 */
	EL_Pthread_Suspend( tmr_thread );
	
	
#endif
	OS_Exit_Critical_Check();
	return ktmr;
}
#endif

struct list_head thread_queue;
mutex_lock_t G_KTMR_NOTIFY_QUEUE_MUT;
EL_RESULT_T kTimerStart(kTimer_t * tmr,sys_tick_t count_down);
/* 如何使线程在睡眠中等待被唤醒 */
/* 解决方案：信号量机制，使用信号量建立生产者-消费者模型。
被创建的定时器线程在定时任务结束后向中央调度线程发送信号量，
由中央调度线程进行消费 */

/* 定时器调度线程 */
void pthread_ktmr_sched( void * arg )
{	
	EL_RESULT_T ret;
	sys_tick_t unsched_tick = 0;
	kTimer_t * ktmr_timeout = NULL;
	EL_PTCB_T * thread_tmr = NULL;
#if KTMR_THREAD_SHARE == 0
	/* 初始化同步机制 */
	el_sem_init(&g_kmtr_sem,0);
	EL_Mutex_Lock_Init(&G_KTMR_NOTIFY_QUEUE_MUT,MUTEX_LOCK_NESTING);
	/* 初始化通知队列 */
	INIT_LIST_HEAD( &thread_queue );
#endif
	while(1)
	{
#if KTMR_THREAD_SHARE == 0
		/* 查找超时的定时器 */
		while(NULL != (ktmr_timeout = KTMR_TIMEOUT_SEARCH(&ktmr_container))){

			KTMR_CONTAINER_REMOVE(ktmr_timeout,&ktmr_container);
			/* 唤醒这个线程 */
			EL_Pthread_Resume(ktmr_timeout->tmr_thread);
		}
		/* 确定下一个定时器的超时时间 */
		unsched_tick = (KTMR_TIMEOUT_SOON_SEARCH(pcont)->schdline);

		unsched_tick = unsched_tick - systick_get();
#else
		/* 查找超时的定时器 */
		while(NULL != (ktmr_timeout = KTMR_TIMEOUT_SEARCH(&ktmr_container))){
			el_sem_take(&sem_ktmrCont,0xffffffff);
			KTMR_CONTAINER_REMOVE(ktmr_timeout,&ktmr_container);
			el_sem_release(&sem_ktmrCont);
			/* 执行定时任务 */
			ktmr_timeout->activing = KTMR_YACTIVING;
			ktmr_timeout->CallBackFunEntry(ktmr_timeout->callback_args);
			ktmr_timeout->activing = KTMR_NACTIVING;
			if(ktmr_timeout->sched_cnt != 0){
				ktmr_timeout->sched_cnt --;
				if(ktmr_timeout->sched_cnt <= 0){
					kTimerStop(ktmr_timeout);
					continue;
				}
			}
			if(ktmr_timeout->period_timeout_type == 1)
				ktmr_timeout->schdline = ktmr_timeout->schdline + ktmr_timeout->timeout_tick;//不建议选这个模式
			else
				ktmr_timeout->schdline = systick_get() + ktmr_timeout->timeout_tick;//建议选这个模式
			el_sem_take(&sem_ktmrCont,0xffffffff);
			KTMR_CONTAINER_ADD(ktmr_timeout,&ktmr_container);
			el_sem_release(&sem_ktmrCont);
		}
#endif
#if KTMR_THREAD_SHARE == 0
		/* 休眠调度线程直至某个定时器超时或定时器任务通知唤醒调度线程 */
		ret = el_sem_take(&g_kmtr_sem,unsched_tick);
		if(ret == EL_RESULT_ERR)
			continue;
		else{
			EL_MutexLock_Take(&G_KTMR_NOTIFY_QUEUE_MUT);
			/* 读取队列中的定时器线程 */
			thread_tmr = (EL_PTCB_T *)thread_queue.next;
			/* 删除和释放第一个节点 */
			list_del(thread_queue.next);
			free((void *)thread_tmr);
			EL_MutexLock_Release(&G_KTMR_NOTIFY_QUEUE_MUT);

			/* 后处理，计算超时时间，重新将定时器插入到容器 */
			ktmr_timeout->schdline = systick_get() + ktmr_timeout->timeout_tick;
			KTMR_CONTAINER_REMOVE(thread_tmr,&ktmr_container);
			KTMR_CONTAINER_ADD(ktmr_timeout,&ktmr_container);
		}
#endif
	}
}

#if KTMR_THREAD_SHARE == 0
/* 唤醒中央调度线程 */
void timer_thread_notify(void)
{
	EL_PTCB_T queue_node;
	EL_PTCB_T * cur_thread;
	cur_thread = EL_GET_CUR_PTHREAD();
	
	EL_MutexLock_Take(&G_KTMR_NOTIFY_QUEUE_MUT);
	//queue_node = (EL_PTCB_T *)malloc( sizeof(EL_PTCB_T) );
	/* 将当前线程实例送入队列 */
	list_add_tail( &queue_node.pthread_node,&thread_queue );
	EL_MutexLock_Release(&G_KTMR_NOTIFY_QUEUE_MUT);
	el_sem_release(&g_kmtr_sem);
}

/* 每个定时器线程结束时必须调用的宏（待放入头文件中） */
#def#endifine KTMR_SCHED_NOTIFY() timer_thread_notify()
#endif
/* 启动定时器 */
EL_RESULT_T kTimerStart(kTimer_t * tmr,sys_tick_t count_down)
{
	if( tmr == NULL ) return EL_RESULT_ERR;
	
	tmr->schdline = systick_get() + count_down;
	/* 放入容器 */
	el_sem_take(&sem_ktmrCont,0xffffffff);
	KTMR_CONTAINER_ADD(tmr,&ktmr_container);
	el_sem_release(&sem_ktmrCont);
#if KTMR_THREAD_SHARE == 0
	/* 恢复定时器线程 */
	EL_Pthread_Resume(tmr->tmr_thread);
#endif
	
	return EL_RESULT_OK;
}

/* 停止定时器 */
void kTimerStop(kTimer_t * tmr)
{	
	if( tmr == NULL ) return;
	
	/* 从容器删除 */
	el_sem_take(&sem_ktmrCont,0xffffffff);
	KTMR_CONTAINER_REMOVE(tmr,&ktmr_container);
	el_sem_release(&sem_ktmrCont);
}

/* 释放定时器 */
void kTimerDestroy(kTimer_t * tmr)
{
}

///*测试用例*/
//void ledtrl(void *arg)
//{
//	//LED1_TOGGLE();
//}
//kTimer_t * timer1;
//void kmtr20240626_test(void){
//	timer1 = kTimerCreate(NULL,\
//				0,\
//				period,\
//				1,\
//				30,\
//				0,\
//				ledtrl,\
//				NULL);
//	
//	kTimerStart(timer1,0);
//}