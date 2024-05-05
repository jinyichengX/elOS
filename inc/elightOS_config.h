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

#if EL_USE_THREAD_PENDING == 1
#define THREAD_PENDING_POOLSIZE 512 /* 线程阻塞等待对象池大小 */
#endif

#if EL_USE_THREAD_SUSPEND == 1
#define THREAD_SUSPEND_POOLSIZE 512 /* 线程挂起对象池大小 */
#endif

#if EL_USE_MUTEXLOCK == 1
#define MUTEXLOCK_POOLSIZE 512 /* 互斥锁对象池大小 */
#endif

#if EL_USE_SPEEDPIPE == 1
#define SPEEDPIPE_POOLSIZE 512 /* 高速队列对象池大小 */
#endif

#if EL_USE_KTIMER == 1
#define KTIMER_POOLSIZE 512 /* 高速队列对象池大小 */
#endif

#endif