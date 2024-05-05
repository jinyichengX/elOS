#ifndef EL_KTMR_H
#define EL_KTMR_H

#include "el_klist.h"
#include "el_type.h"
#include "el_pthread.h"

/* �����ʱ������ */
/* 1.������ʱ�� ��1����β���δ���ִ��/������ ��2�����������͢ٴ����آڲ������� */
/* 2.���ڼ����� ��1�������ڱ���ִ���� ��2��ִ����ȴ��̶����� */

typedef void (*pTmrCallBackEntry)(void *args);

typedef struct stKernelTimer{
	struct list_head ktmr_node;		/* ��ʱ���б���� */
	EL_G_SYSTICK_TYPE schdline; 	/* ��ʱ�����ʼʱ�� */
	EL_G_SYSTICK_TYPE deadline; 	/* �����Ͷ�ʱ�����ֹʱ�� */
	pTmrCallBackEntry CallBackFunEntry;	/* ��ʱ���ص����� */
	void * callback_args;			/* �û����� */
}kTimer_t;

#endif




