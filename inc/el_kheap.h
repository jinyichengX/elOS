#ifndef EL_KHEAP_H
#define EL_KHEAP_H

#include <stdint.h>
#include "el_mutex_lock.h"
#include "el_klist.h"
#include "el_type.h"

typedef struct stHeapControlBlock hcb_t;

/*�������*/
#define KHEAP_NAME_EN 0
#define KHEAP_USAGE_STATISTICS_EN 1
/* ���ڴ���뵥Ԫ */
#define KHEAP_ALIGNMENT_UINT 4
#if KHEAP_NAME_EN == 1
/* ���ڴ�����󳤶�*/
	#define HEAP_NAME_LEN 10
#endif
/* ������ڴ滥����������� */
#define HEAPLOCK_TYPE mutex_lock_t //����һ�������������Ըĳ�������
/* �������ģʽ */
#define KHEAP_ALLOC_PATTERN_AUTO_SWITCH 0

#define HEAP_TYPE(top_level,next_level) HEAP_TYPE_##top_level_##next_level

//// ��������ʹ���ػ��ĺ꣺
//#define HEAP_TYPE(KERNEL,KOBJ_POOL) HEAP_TYPE_##KERNEL##KOBJ_POOL

/* ��ϲ��� */
typedef enum {
	BEST_FIT,							/* ������ */
	FIRST_FIT,							/* �״���� */
	WORST_FIT,							/* ������ */
	MAX_PATTERN,						/* ��ϼ��� */
}AllocStrategy_t;

/* ������� */
typedef struct stAlloStrategy{
	AllocStrategy_t alloc_pttn; 							/* ������� */
	void * (*php_alloc[MAX_PATTERN])( hcb_t *, uint32_t);	/* ʵ�ֲ��� */
}alloc_stg_t;

typedef struct stHeapControlBlock{
	struct list_head hook;				/* ���Ӷ���� */
	struct list_head free_entry;		/* ָ���׸����п� */
	uint32_t heap_size;					/* ���ڴ��ܴ�С */
	uint32_t valid_size;				/* ���ڴ���Ч��С */
	uint32_t start_addr;				/* ��ʼ��ַ */
#if KHEAP_NAME_EN == 1
	char heap_name[HEAP_NAME_LEN + 1];	/* ���ڴ��� */
#endif
	alloc_stg_t strat;					/* ������� */
	HEAPLOCK_TYPE hp_lock;				/* ������ */
#if KHEAP_USAGE_STATISTICS_EN == 1
	/* ��1��usage_int = 1 , usage_flo = 9 --------->1.09%  */
	/* ��2��usage_int = 23, usage_flo = 68--------->23.68% */
	char usage_int;						/* �ڴ�ʹ���ʣ��������֣�*/
	char usage_flo;						/* �ڴ�ʹ���ʣ���λС����*/ 
#endif
}hcb_t;

/* ���нڵ���Ϣ */
typedef struct stLinkNode{ 
	struct list_head hpnode;			/* ���п�ڵ� */
	uint32_t size; 						/* ���С */
}linknode_t;

/* ���ýڵ���Ϣ */
struct usdinfo{
	uint32_t usdsz; 					/* ���С */
};		

extern void Kheap_name_set( hcb_t * hcb,const char * n);
extern hcb_t * Kheap_Initialise(void * surf,void * bottom);	/* �ѳ�ʼ�� */
extern void * hp_alloc(hcb_t * hcb, uint32_t size);			/* �ڴ���䣨������ʽ�� */
extern void * hp_calloc( hcb_t * hcb, uint32_t size );
extern void hp_free( hcb_t * hcb, void * fst_addr );		/* �ڴ��ͷţ�������ʽ�� */
extern EL_RESULT_T hp_StatisticsTake( hcb_t * hcb, uint32_t *heap_size, uint32_t * start_addr,
								uint8_t * strat, uint8_t * usg1, uint8_t * usg2);
#endif