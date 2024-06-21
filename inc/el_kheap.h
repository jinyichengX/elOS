#ifndef EL_KHEAP_H
#define EL_KHEAP_H

#include <stdint.h>
#include "el_mutex_lock.h"
#include "el_klist.h"
#include "el_type.h"

typedef struct stHeapControlBlock hcb_t;

/*配置相关*/
#define KHEAP_NAME_EN 0
#define KHEAP_USAGE_STATISTICS_EN 1
/* 堆内存对齐单元 */
#define KHEAP_ALIGNMENT_UINT 4
#if KHEAP_NAME_EN == 1
/* 堆内存名最大长度*/
	#define HEAP_NAME_LEN 10
#endif
/* 定义堆内存互斥访问锁类型 */
#define HEAPLOCK_TYPE mutex_lock_t //定义一个互斥锁，可以改成自旋锁
/* 智能拟合模式 */
#define KHEAP_ALLOC_PATTERN_AUTO_SWITCH 0

#define HEAP_TYPE(top_level,next_level) HEAP_TYPE_##top_level_##next_level

//// 可以这样使用特化的宏：
//#define HEAP_TYPE(KERNEL,KOBJ_POOL) HEAP_TYPE_##KERNEL##KOBJ_POOL

/* 拟合策略 */
typedef enum {
	BEST_FIT,							/* 最佳拟合 */
	FIRST_FIT,							/* 首次拟合 */
	WORST_FIT,							/* 最差拟合 */
	MAX_PATTERN,						/* 拟合计数 */
}AllocStrategy_t;

/* 分配策略 */
typedef struct stAlloStrategy{
	AllocStrategy_t alloc_pttn; 							/* 分配策略 */
	void * (*php_alloc[MAX_PATTERN])( hcb_t *, uint32_t);	/* 实现策略 */
}alloc_stg_t;

typedef struct stHeapControlBlock{
	struct list_head hook;				/* 链接多个堆 */
	struct list_head free_entry;		/* 指向首个空闲块 */
	uint32_t heap_size;					/* 堆内存总大小 */
	uint32_t valid_size;				/* 堆内存有效大小 */
	uint32_t start_addr;				/* 起始地址 */
#if KHEAP_NAME_EN == 1
	char heap_name[HEAP_NAME_LEN + 1];	/* 堆内存名 */
#endif
	alloc_stg_t strat;					/* 分配策略 */
	HEAPLOCK_TYPE hp_lock;				/* 访问锁 */
#if KHEAP_USAGE_STATISTICS_EN == 1
	/* 例1：usage_int = 1 , usage_flo = 9 --------->1.09%  */
	/* 例2：usage_int = 23, usage_flo = 68--------->23.68% */
	char usage_int;						/* 内存使用率（整数部分）*/
	char usage_flo;						/* 内存使用率（两位小数）*/ 
#endif
}hcb_t;

/* 空闲节点信息 */
typedef struct stLinkNode{ 
	struct list_head hpnode;			/* 空闲块节点 */
	uint32_t size; 						/* 块大小 */
}linknode_t;

/* 已用节点信息 */
struct usdinfo{
	uint32_t usdsz; 					/* 块大小 */
};		

extern void Kheap_name_set( hcb_t * hcb,const char * n);
extern hcb_t * Kheap_Initialise(void * surf,void * bottom);	/* 堆初始化 */
extern void * hp_alloc(hcb_t * hcb, uint32_t size);			/* 内存分配（阻塞方式） */
extern void * hp_calloc( hcb_t * hcb, uint32_t size );
extern void hp_free( hcb_t * hcb, void * fst_addr );		/* 内存释放（阻塞方式） */
extern EL_RESULT_T hp_StatisticsTake( hcb_t * hcb, uint32_t *heap_size, uint32_t * start_addr,
								uint8_t * strat, uint8_t * usg1, uint8_t * usg2);
#endif