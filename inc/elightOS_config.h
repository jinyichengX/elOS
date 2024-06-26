#ifndef ELIGHTOS_CONFIG_H
#define ELIGHTOS_CONFIG_H

/* 定义最多优先级 */
#define EL_MAX_LEVEL_OF_PTHREAD_PRIO 8

/* 定义线程名长度 */
#define EL_PTHREAD_NAME_MAX_LEN 10

/* 计算CPU使用率 */
#define EL_CLAC_CPU_USAGE_RATE 0
#if EL_CALC_CPU_USAGE_RATE
#define EL_CALC_CPU_USAGE_RATE_STAMP_TICK 50
#endif

/* 查看线程栈使用率 */
#define EL_CALC_PTHREAD_STACK_USAGE_RATE 1

#define EL_CFG_DEFAULT_PTHREAD_STACK_SIZE 64

/* 使用线程阻塞等待 */
#define EL_USE_THREAD_PENDING 1 
/* 使用线程挂起 */
#define EL_USE_THREAD_SUSPEND 1 
/* 使用互斥锁 */
#define EL_USE_MUTEXLOCK 1 		
/* 使用高速队列 */
#define EL_USE_SPEEDPIPE 1 
#if EL_USE_SPEEDPIPE == 1
#define EL_HIGHSPPEDPIPE_SPIN_PEND 1/* 高速队列自旋阻塞模式 */
#endif
/* 使用内核定时器 */
#define EL_USE_KTIMER 1 	
/* 使用信号量 */
#define EL_USE_SEM 1

/* 线程阻塞等待管理器是否静态分配 */
#if EL_USE_THREAD_PENDING
#define THREAD_PENDING_OBJ_STATIC 1
#endif

/* 线程挂起管理器是否静态分配 */
#if EL_USE_THREAD_SUSPEND
#define THREAD_SUSPEND_OBJ_STATIC 1
#endif

/* 互斥锁是否静态分配 */
#if EL_USE_MUTEXLOCK
#define MUTEXLOCK_OBJ_STATIC 1
#endif

/* 高速队列管理器是否静态分配 */
#if EL_USE_SPEEDPIPE
#define SPEEDPIPE_OBJ_STATIC 1
#endif

/* 内核定时器是否静态分配 */
#if EL_USE_KTIMER
#define KTIMER_OBJ_STATIC 1
#endif

/* 信号量是否静态分配 */
#if EL_USE_SEM
#define SEM_OBJ_STATIC 1
#endif


#define THREAD_POOLSIZE	1024			/* 线程控制块对象池大小 */

#if EL_USE_THREAD_PENDING == 1
#define THREAD_PENDING_POOLSIZE 1024 /* 线程阻塞等待对象池大小 */
#endif

#if EL_USE_THREAD_SUSPEND == 1
#define THREAD_SUSPEND_POOLSIZE 1024 /* 线程挂起对象池大小 */
#endif

#if EL_USE_MUTEXLOCK == 1
#define MUTEXLOCK_POOLSIZE 1024 /* 互斥锁对象池大小 */
#endif

#if EL_USE_SPEEDPIPE == 1
#define SPEEDPIPE_POOLSIZE 1024 /* 高速队列对象池大小 */
#endif

#if EL_USE_KTIMER == 1
#define KTIMER_POOLSIZE 1024 /* 高速队列对象池大小 */
#endif

#if EL_USE_SEM == 1
#define SEM_POOLSIZE 1024 /* 信号量对象池大小 */
#endif

/* 内核定时器配置 */
#ifndef KTMR_THREAD_SHARE
#define KTMR_THREAD_SHARE 1 /* 多定时器共享调度线程。不可设为0，功能待完善 */
#endif
#endif