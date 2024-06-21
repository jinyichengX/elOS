/* 内核定时器 */

/* 内核定时器任务调度策略
1.多定时任务共享一个调度周期，由中央调度线程统一管理（占用内存小但定时器响应度低，执行周期精度较低）
2.每个定时任务分散调度，方式①：每个定时任务都分配调度周期自行调度（定时器响应度高但占用内存大，执行周期精度较高）
 						 方式②：由中央调度线程统一管理每个定时线程的调度（定时器响应度高但占用内存大，执行周期精度较高）
*/


/* 软件定时器类型 */
/* 1.连续计时型 （1）首尾单次触发执行/结束型 （2）连续运行型①带返回②不带返回 */
/* 2.周期计数型 （1）周期内必须执行完 （2）执行完等待固定周期 */

/* 内核定时器使用链表/跳表/AVL树/红黑树管理 */
#include "el_ktmr.h"

extern EL_G_SYSTICK_TYPE systick_get(void);

el_sem_t g_kmtr_sem;

#define os_realloc	realloc
#define os_malloc 	malloc
#define os_free 	free

/* 全局的定时器容器 */
static ktmr_mng_container_t ktmr_container;
#define KTMR_CONTAINER_INIT do{avl_Initialise(&ktmr_container,\
											NULL,0);\
										}while(0);

#define KTMR_CONTAINER_ADD(ktmr)  
#define KTMR_CONTAINER_REMOVE(ktmr)  

/* 查找已经超时的定时器 */
#define KTMR_TIMEOUT_SEARCH()
/* 查找即将超时的定时器 */
#define KTMR_TIMEOUT_SOON_SEARCH()

/* 定时器活动标志查询 */
#define KTMR_ACTIVING(ktmr) 

#if KTMR_THREAD_SHARE == 0
/* 等待通知 */
#define KTMR_SCHED_THREAD_NOTIFY()
#endif

void kTimerInitialise( kTimer_t * ptmr_inst,\
						ktmr_type type,\
						pTmrCallBackEntry usr_callback );
										
#if KTMR_THREAD_SHARE == 1
void kTimerManageThreadCreate(EL_UINT);
#endif
										
/* 定时器容器初始化 */
void ktmr_module_init(void)
{
	KTMR_CONTAINER_INIT;
#if KTMR_THREAD_SHARE == 1
	kTimerManageThreadCreate();
#endif
}

/* 创建定时器错误处理 */
static inline void ktmr_create_err_handle(void * kobj_pool,kTimer_t *tmr)
{
	EL_stKpoolBlockFree((void *)kobj_pool,(void *)tmr);
}

/* 创建定时器 */
#if KTIMER_OBJ_STATIC == 1
kTimer_t * 
kTimerCreate(const char * name,\
			 void * tmr_thread_entry,\
			 EL_UINT stack_size,\
			 ktmr_type type,\
			 
			 pTmrCallBackEntry usr_callback)
{
	int index;
	EL_kobj_info_t kobj;
	EL_PTCB_T * tmr_thread;
	kTimer_t * ktmr = NULL;
	if( usr_callback == NULL ) return NULL;
	
	/* 从对象池中申请对象 */
	if(EL_RESULT_OK != ELOS_KobjStatisticsGet( EL_KOBJ_kTIMER,&kobj))
		return NULL;
	ktmr = (void *)EL_stKpoolBlockTryAlloc( (void *)kobj.Kobj_Basepool );
	if(NULL == ktmr) return NULL;
	/* 初始化定时器 */
	kTimerInitialise(ktmr,type,usr_callback);

#if KTMR_THREAD_SHARE == 0/* 多定时器共享线程 */
	if( tmr_thread_entry == NULL ) {
		ktmr_create_err_handle((void *)kobj.Kobj_Basepool,(void *)ktmr);
		return NULL;
	}
	#if 1
	/* 从内核对象池取线程控制块 */
	tmr_thread = (EL_PTCB_T * )ELOS_RequestForPoolWait(EL_KOBJ_PTCB,0);
	#else
	/* 从堆中分配 */
	tmr_thread = (EL_PTCB_T * )os_malloc(sizeof(EL_PTCB_T));
	#endif
	if( tmr_thread == NULL ) {
		ktmr_create_err_handle((void *)kobj.Kobj_Basepool,(void *)ktmr);
		return NULL;
	}
	/* 创建定时器线程，优先级最高 */
	if(EL_RESULT_ERR ==  EL_Pthread_Create( tmr_thread, name, tmr_thread_entry,stack_size,0))
	{
		/* 释放定时器对象 */
		ktmr_create_err_handle((void *)kobj.Kobj_Basepool,(void *)ktmr);
		/* 释放线程对象 */
		os_free( (void *)tmr_thread );
		return NULL;
	}
	/* 挂起定时器线程 */
	EL_Pthread_Suspend( tmr_thread );
#endif
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
	/* 初始化同步机制 */
	el_sem_init(&g_kmtr_sem,0);
	EL_Mutex_Lock_Init(&G_KTMR_NOTIFY_QUEUE_MUT,MUTEX_LOCK_NESTING);
	
	/* 初始化生产队列 */
	INIT_LIST_HEAD( &thread_queue );
	while(1)
	{
#if KTMR_THREAD_SHARE == 0
		while(ktmr_timeout == NULL){
//			/* 查找第一个超时的定时器，从容器删除 */
//			ktmr_timeout = KTMR_TIMEOUT_SEARCH();
//			KTMR_CONTAINER_REMOVE(ktmr_timeout);
//			/* 唤醒这个线程 */
//			thread_tmr = ktmr_timeout->ptcb;
//			EL_Pthread_Resume(thread_tmr);
		}
		/* 确定下一个定时器的超时时间 */
//		unsched_tick = (KTMR_TIMEOUT_SOON_SEARCH()->schdline);
		ASSERT(unsched_tick >= systick_get());
		unsched_tick = unsched_tick - systick_get();
#else

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
			KTMR_CONTAINER_REMOVE(thread_tmr);
			KTMR_CONTAINER_ADD(ktmr_timeout);
		}
#endif
	}
}

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
#define KTMR_SCHED_NOTIFY() timer_thread_notify()

EL_PTCB_T ktmr_sched;
/* 创建一个线程管理多个定时器 */
void kTimerManageThreadCreate(EL_UINT stack_size)
{
#if 1
	
#endif
	EL_Pthread_Create(&ktmr_sched,"ktmr_sched",pthread_ktmr_sched,stack_size,0);
	/* 挂起定时器线程 */
}

/* 初始化定时器 */
void kTimerInitialise( kTimer_t * tmr,\
						ktmr_type type,\
					   pTmrCallBackEntry usr_callback )
{
	if(tmr == NULL) return;
	if(usr_callback == NULL) return;
	tmr->CallBackFunEntry = usr_callback;
	tmr->callback_args = NULL;
	tmr->schdline = 0;
	tmr->deadline = 0;
}

/* 启动定时器 */
EL_RESULT_T kTimerStart(kTimer_t * tmr,sys_tick_t count_down)
{
	if( tmr == NULL ) return EL_RESULT_ERR;
	
	OS_Enter_Critical_Check();
	tmr->schdline = systick_get() + count_down;
	/* 放入容器 */
	KTMR_CONTAINER_ADD(ktmr);
	
	OS_Exit_Critical_Check();
}

///* 停止定时器 */
//void kTimerStop(kTimer_t * tmr)
//{	
//	if( tmr == NULL ) return;
//	OS_Enter_Critical_Check();
//	/* 定时器是否活动中 */
//	if(KTMR_ACTIVING(ktmr) == 1){
//		OS_Exit_Critical_Check();
//		return;
//	}
//	/* 从容器删除 */
//	KTMR_CONTAINER_REMOVE(ktmr);
//	OS_Exit_Critical_Check();
//}

///* 释放定时器 */
//void kTimerDestroy(kTimer_t * tmr)
//{
//#if 
//	/* 内核对象释放 */
//	
//#endif
//}

