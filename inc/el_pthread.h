#ifndef EL_PTHREAD_H
#define EL_PTHREAD_H

#include "elightOS_config.h"
#include "el_klist.h"
#include "el_type.h"
#include "port.h"

#ifndef CPU_MAX_USAGE_RATE
#define CPU_MAX_USAGE_RATE 100
#endif

#ifndef MIN_PTHREAD_STACK_SIZE
#define MIN_PTHREAD_STACK_SIZE 64
#endif

#if ((EL_MAX_LEVEL_OF_PTHREAD_PRIO <= 0)||\
	(EL_MAX_LEVEL_OF_PTHREAD_PRIO > 256))
    #error "EL_MAX_LEVEL_OF_PTHREAD_PRIO macro invalid"
#endif

#if EL_PTHREAD_NAME_MAX_LEN <= 0
    #error "pthread name is too short"
#endif

typedef void (*pthread_entry)(void *);

/* 线程状态类型 */
typedef enum {
	EL_PTHREAD_READY = 0, 							/* 线程就绪 */
	EL_PTHREAD_PENDING,								/* 线程阻塞 */
	EL_PTHREAD_SUSPEND,								/* 线程挂起 */
	EL_PTHREAD_RUNNING,								/* 线程运行 */
	EL_PTHREAD_SLEEPING,							/* 线程延时 */
	EL_PTHREAD_COMPOUND_BLOCK,						/* 线程复合阻塞 */
	EL_PTHREAD_STATUS_COUNT,						/* 线程状态计数 */
	EL_PTHREAD_DEAD									/* 死亡线程，不能恢复 */
}EL_PTHREAD_STATUS_T;

/* 延时阻塞类型 */
typedef enum{
	EVENT_TYPE_NONE_WAIT,	/* 未等待同步量 */
	EVENT_TYPE_SLEEP,		/* 等待睡眠唤醒 */
	EVENT_TYPE_SEM_WAIT,	/* 等待信号量 */
	EVENT_TYPE_MUTEX_WAIT,	/* 等待互斥锁 */
	EVENT_TYPE_QUEUE_WAIT,	/* 等待消息队列 */
	EVENT_TYPE_EVTFLAG_WAIT /* 等待事件 */
}PendType_t;

/* 线程同步互斥组占用标志 */
#define EL_PTHREAD_NONE_WAIT 0x0000	  				/* 无等待 */
#define EL_PTHREAD_SEMAP_WAIT 0x00001 				/* 等待信号量释放 */
#define EL_PTHREAD_MUTEX_WAIT 0x00002 				/* 等待互斥锁释放 */
#define EL_PTHREAD_SPEEDPIPE_WAIT 0x0004 			/* 等待高速队列可用 */

/* 线程控制块 */
typedef struct EL_PTHREAD_CONTROL_BLOCK
{
	struct list_head pthread_node;					/* 钩子节点 */
    EL_CHAR pthread_name[EL_PTHREAD_NAME_MAX_LEN];	/* 线程名 */
    EL_PTHREAD_PRIO_TYPE pthread_prio;				/* 线程优先级 */
    EL_PORT_STACK_TYPE *pthread_sp;					/* 动态线程栈 */
    EL_PTHREAD_STATUS_T pthread_state;				/* 线程状态 */
	EL_PORT_STACK_TYPE *pthread_stack_top;			/* 线程栈栈顶 */
#if EL_CALC_PTHREAD_STACK_USAGE_RATE
	EL_STACK_TYPE pthread_stack_size;				/* 线程栈大小 */
	EL_FLOAT pthread_stack_usage;					/* 线程栈使用率 */
#endif
	EL_UINT pthread_id;								/* 线程id */
	void * block_holder;							/* 被持有对象的节点 */
	EL_UINT PendFlags;								/* 同步互斥组阻塞标志 */

#if EL_USE_EVENTFLAG
	/* 需要满足的事件标志 */
	uint32_t event_wait;						
	bool andor;
#endif
}EL_PTCB_T;

typedef struct EL_OS_AbsolutePendingTickCount
{
	struct list_head TickPending_Node;				/* 钩子节点 */
	EL_UINT TickSuspend_OverFlowCount;				/* 系统tick溢出单位 */
	EL_UINT TickSuspend_Count;						/* 一单位内系统tick */
	EL_PTCB_T * Pthread;							/* 被延时阻塞的线程 */
	PendType_t PendType;							/* 阻塞类型 */
	int * result;									/* 阻塞等待同步量结果 */
}TickPending_t;

typedef struct EL_OS_SuspendStruct
{
	struct list_head Suspend_Node;					/* 钩子节点 */
	EL_PTCB_T * Pthread;							/* 被挂起的线程 */
	EL_UCHAR Suspend_nesting;						/* 挂起嵌套计数 */
	void * PendingType;								/* 是否正处于阻塞状态被挂起 */
}Suspend_t;

#define SZ_Suspend_t sizeof(Suspend_t)
#define SZ_TickPend_t sizeof(TickPending_t)
	
#define FPOS(type,field) 							( (EL_PORT_STACK_TYPE)&((type *)0)->field )
#define STATICSP_POS FPOS(EL_PTCB_T,pthread_sp) 		/* 线程私有sp在线程控制块中的偏移 */
#define PTCB_BASE(PRIV_PSP) 						( (EL_PTCB_T *)((EL_PORT_STACK_TYPE)PRIV_PSP-STATICSP_POS) )/* 线程控制块基地址（指针） */
#define EL_GET_CUR_PTHREAD() 						PTCB_BASE(g_pthread_sp)/* 获取当前线程控制块（指针） */

#define PTHREAD_STATE_SET(PPTCB,STATE) 				(PPTCB->pthread_state = STATE)/* 设置线程状态(由ptcb*) */
#define PTHREAD_STATE_GET(PPTCB) 					(PPTCB->pthread_state)/* 读取线程状态(由ptcb*) */

extern EL_RESULT_T EL_Pthread_Initialise(EL_PTCB_T *ptcb,\
									const char * name,\
									pthread_entry pthread_entry,\
									EL_UINT pthread_stackSz,\
									EL_PTHREAD_PRIO_TYPE pthread_prio,\
									void *args);
extern EL_RESULT_T EL_Pthread_Create(const char * name,pthread_entry entry,\
	EL_UINT pthread_stackSz,EL_PTHREAD_PRIO_TYPE prio,void * args);
extern void EL_OS_Start_Scheduler(void);
extern void EL_PrioListInitialise(void);
extern void EL_OS_Initialise(void);
extern void EL_Pthread_Sleep(EL_UINT TickToDelay);
extern EL_RESULT_T EL_Pthread_Suspend(EL_PTCB_T *PthreadToSuspend);
extern void EL_Pthread_Resume(EL_PTCB_T *PthreadToResume);
extern void EL_Pthread_PushToSuspendList(EL_PTCB_T * ptcb,\
										struct list_head * SuspendList);
extern void EL_Pthread_Priority_Set(EL_PTCB_T *PthreadToModify,\
									EL_PTHREAD_PRIO_TYPE new_prio);
extern void EL_Pthread_EventWait(EL_PTCB_T * thread,\
						  PendType_t type,\
                          EL_UINT tick,\
						  int * result);
extern void EL_Pthread_EventWakeup(EL_PTCB_T * thread);
extern void EL_TickIncCount(void);
extern void EL_Pthread_Pend_Release(void);
extern EL_PTCB_T* EL_Pthread_DelSelf(EL_PTCB_T *PthreadToDel);

extern EL_PORT_STACK_TYPE g_psp;
extern EL_PORT_STACK_TYPE **g_pthread_sp;/* 线程私有psp */
extern EL_UINT g_TickSuspend_OverFlowCount;/* 系统tick溢出单位 */
extern EL_UINT g_TickSuspend_Count;/* 一单位内系统tick */

#if EL_CLAC_CPU_USAGE_RATE
extern EL_FLOAT CPU_UsageRate;
#endif
#endif
