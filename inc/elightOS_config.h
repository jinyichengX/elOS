#ifndef ELIGHTOS_CONFIG_H
#define ELIGHTOS_CONFIG_H

/* 线程相关 */
#define EL_MAX_LEVEL_OF_PTHREAD_PRIO 8		/* 定义最多优先级 */
#define EL_PTHREAD_NAME_MAX_LEN 10			/* 定义线程名长度 */
#define EL_CLAC_CPU_USAGE_RATE 0			/* 计算CPU使用率 */
#if EL_CALC_CPU_USAGE_RATE
#define EL_CALC_CPU_USAGE_RATE_STAMP_TICK 50
#endif
#define EL_CALC_PTHREAD_STACK_USAGE_RATE 1	/* 查看线程栈使用率 */
#define EL_CFG_DEFAULT_PTHREAD_STACK_SIZE 64

#ifndef EL_USE_THREAD_PENDING
#define EL_USE_THREAD_PENDING 1 	/* 使用线程阻塞等待 */
#endif
#ifndef EL_USE_THREAD_SUSPEND
#define EL_USE_THREAD_SUSPEND 1 	/* 使用线程挂起 */
#endif
#ifndef EL_USE_MUTEXLOCK
#define EL_USE_MUTEXLOCK 1 		 	/* 使用互斥锁 */
#endif
#ifndef EL_USE_MESSAGE_QUEUE
#define EL_USE_MESSAGE_QUEUE 1 		/* 使用消息队列 */
#endif
#ifndef EL_USE_KTIMER
#define EL_USE_KTIMER 1				/* 使用内核定时器 */
#endif
#ifndef EL_USE_SEM
#define EL_USE_SEM 1				/* 使用信号量 */
#endif
#ifndef EL_USE_EVENTFLAG
#define EL_USE_EVENTFLAG 1			/* 使用事件标志 */
#endif

#if EL_USE_THREAD_PENDING
#define THREAD_PENDING_OBJ_STATIC 1	/* 线程阻塞等待管理器是否静态分配 */
#endif

#if EL_USE_THREAD_SUSPEND
#define THREAD_SUSPEND_OBJ_STATIC 1	/* 线程挂起管理器是否静态分配 */
#endif

#if EL_USE_MUTEXLOCK
#define MUTEXLOCK_OBJ_STATIC 1		/* 互斥锁是否静态分配 */
#endif

#if EL_USE_MESSAGE_QUEUE
#define MESSAGE_QUEUE_OBJ_STATIC 1		/* 消息队列管理器是否静态分配 */
#endif

#if EL_USE_KTIMER
#define KTIMER_OBJ_STATIC 1			/* 内核定时器是否静态分配 */
#endif

#if EL_USE_SEM
#define SEM_OBJ_STATIC 1			/* 信号量是否静态分配 */
#endif

#if EL_USE_EVENTFLAG
#define EVENTFLAG_OBJ_STATIC 1		/* 事件标志是否静态分配 */
#endif

#define THREAD_POOLSIZE	1024		/* 线程控制块对象池大小 */

#if EL_USE_THREAD_PENDING == 1 && THREAD_PENDING_OBJ_STATIC == 1
#define THREAD_PENDING_POOLSIZE 1024/* 线程阻塞等待对象池大小 */
#endif

#if EL_USE_THREAD_SUSPEND == 1 && THREAD_SUSPEND_OBJ_STATIC == 1
#define THREAD_SUSPEND_POOLSIZE 1024/* 线程挂起对象池大小 */
#endif

#if EL_USE_MUTEXLOCK == 1 && MUTEXLOCK_OBJ_STATIC == 1
#define MUTEXLOCK_POOLSIZE 1024 	/* 互斥锁对象池大小 */
#endif

#if EL_USE_MESSAGE_QUEUE == 1 && MESSAGE_QUEUE_OBJ_STATIC == 1
#define MESSAGE_QUEUE_POOLSIZE 1024 	/* 消息队列对象池大小 */
#endif

#if EL_USE_KTIMER == 1 && KTIMER_OBJ_STATIC == 1
#define KTIMER_POOLSIZE 1024 		/* 内核定时器对象池大小 */
#endif

#if EL_USE_SEM == 1 && SEM_OBJ_STATIC == 1
#define SEM_POOLSIZE 1024 			/* 信号量对象池大小 */
#endif

#if EL_USE_EVENTFLAG == 1 && EVENTFLAG_OBJ_STATIC == 1
#define EVENTFLAG_POOLSIZE 64		/* 事件标志对象池大小 */
#endif

/* 内核定时器配置 */
#ifndef KTMR_THREAD_SHARE
#define KTMR_THREAD_SHARE 1 		/* 多定时器共享调度线程。不可设为0，功能待完善 */
#endif
#endif


#define __API__