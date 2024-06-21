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
/* �����ʱ������ */
/* 1.������ʱ�� ��1����β���δ���ִ��/������ ��2�����������͢ٴ����آڲ������� */
/* 2.���ڼ����� ��1�������ڱ���ִ���� ��2��ִ����ȴ��̶����� */

/* ���ö�ʱ����������� */
#define ktmr_mng_container_t avl_t

typedef void (*pTmrCallBackEntry)(void *args);

typedef enum
{
	period,	/* ���ڶ�ʱ�� */
	count,	/* ��ʱ��ʱ�� */
}ktmr_type;

typedef struct stKernelTimer{
	ktmr_type type;						/* ��ʱ������ */
	uint32_t period_timeout_type;		
	uint32_t timeout_tick;				/*  */
	EL_G_SYSTICK_TYPE schdline; 	/* ��ʱ�����ʼʱ�� */
	EL_G_SYSTICK_TYPE deadline; 	/* �����Ͷ�ʱ�����ֹʱ�� */
	pTmrCallBackEntry CallBackFunEntry;	/* ��ʱ���ص����� */
	void * callback_args;			/* �û����� */
}kTimer_t;

#ifndef IDX_ZERO 
#define IDX_ZERO 0
#endif

extern void ktmr_module_init(void);

#endif




