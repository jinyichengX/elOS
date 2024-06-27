#ifndef EL_KTMR_H
#define EL_KTMR_H

#include "el_klist.h"
#include "el_avl.h"
#include "el_type.h"
#include "el_pthread.h"
#include "elightOS_config.h"
#include "el_kobj.h"
#include "el_kpool_static.h"
#include "el_kheap.h"
#include "el_sem.h"

#if EL_USE_KTIMER
/* ���ö�ʱ��������������� */
#define ktmr_mng_container_t avl_t

typedef void (*pTmrCallBackEntry)(void *args);

typedef enum
{
	period,	/* ���ڶ�ʱ�� */
	count,	/* ��ʱ��ʱ�� */
}ktmr_type;

typedef struct stKernelTimer{
	ktmr_type type;						/* ��ʱ������ */
	char period_timeout_type;		/* ���ʼ�����ͣ������������ */
	uint32_t timeout_tick;				/* ����/��ʱ���� */
	uint32_t sched_cnt;					/* ���ڴ���/���δ�������������ʱ����Ч�� */
	EL_G_SYSTICK_TYPE schdline; 		/* ��ʱ�����ʼʱ�� */
	EL_G_SYSTICK_TYPE deadline; 		/* ��ʱ�Ͷ�ʱ�����ֹʱ�� */
	pTmrCallBackEntry CallBackFunEntry;	/* ��ʱ���ص����� */
	void * callback_args;				/* �û����� */
	avl_node_t hook;
	char activing;						/* ���־ */
#if KTMR_THREAD_SHARE == 0
	EL_PTCB_T * tmr_thread;
#endif
}kTimer_t;

#ifndef IDX_ZERO 
#define IDX_ZERO 0
#endif

#ifndef KTMR_SCHED_THREAD_STACK_SIZE
#define KTMR_SCHED_THREAD_STACK_SIZE 256
#endif

#define KTMR_NACTIVING 0
#define KTMR_YACTIVING 1

extern EL_RESULT_T ktmr_module_init(void);
extern kTimer_t * kTimerCreate(const char * name,\
				EL_UINT stack_size,\
				ktmr_type type,\
				char sub_type,\
				uint32_t timeout_tick,\
				uint32_t time_cnt,\
				pTmrCallBackEntry usr_callback,\
				void * args);
				extern EL_RESULT_T kTimerStart(kTimer_t * tmr,sys_tick_t count_down);
extern EL_RESULT_T kTimerStart(kTimer_t * tmr,sys_tick_t count_down);
extern void kTimerStop(kTimer_t * tmr);
#endif
#endif




