#ifndef EL_SLAB_H
#define EL_SLAB_H

#include "el_klist.h"
#include "el_type.h"

#define SLAB_CHUNK_MINSZ	64		
#define SLAB_PAGE_SZ 		(4*1024)	
#define SLAB_GROUP_LEVEL 	5 		

/* slab页的分配方式，从页分配器/堆中分配 */
#define SLAB_CACHE_ALLOC_FROM_HEAP_OR_PAGE 0/* 宏为0是从堆分配，为1从页分配器分配，slab页释放功能没写完 */

typedef struct stSlabDispensor slab_disp_t;
typedef struct stSlabContrlBlock slab_ctrl_t;
typedef struct stPageControlHead page_t;

#if SLAB_CACHE_ALLOC_FROM_HEAP_OR_PAGE
/* 页分配器 */
typedef struct stPageControlHead
{
	page_t * next;
	size_t page_num;
}page_t;
#endif

/* slab分配器 */
typedef struct stSlabDispensor
{
    slab_disp_t * next;
    void * plist;		
    uint32_t chunk_num;	
    uint32_t chunk_used;
	slab_ctrl_t * owner;
	uint32_t * punaligned;		//这个成员在使用page分配器时可以不用
}slab_disp_t;

/* slab控制块 */
typedef struct stSlabContrlBlock
{  
    uint32_t zone_size;			/* 一个slab区块大小 */
    uint32_t zone_mounted;		/* 挂载的slab区块数 */
    uint32_t chunk_size;		/* 单个obj大小 */
	
    slab_disp_t * full;			/* 红区块链表头 */
    slab_disp_t * partical;		/* 黄区块链表头 */
    slab_disp_t * empty;		/* 绿区块链表头 */
}slab_ctrl_t;
#if SLAB_CACHE_ALLOC_FROM_HEAP_OR_PAGE
extern page_t * sys_page_init(page_t **page,void *surf, void *bottom);
extern void * _slab_pages_alloc(page_t **page, int page_num);
extern void _slab_pages_free(page_t ** page,void * addr,int page_num);
#endif
extern void slab_mem_init(void *surf, int size);
extern slab_disp_t * slab_cache_new(slab_ctrl_t *slab_ctrl);
extern void slab_cache_destroy(slab_disp_t * slab);
extern void * slab_cache_alloc(slab_disp_t * slab);
extern slab_disp_t * slab_cache_free(void * p);
extern void * slab_mem_alloc(slab_ctrl_t *slab_ctrl,uint32_t size);
extern void slab_mem_free(void * p);
extern void slab_blk_remove(slab_disp_t *slab,void * p);
/**************************************************
 * >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
            s l a b 系 统 结 构
——————————————       _______________
|slab控制块2   |       | slab分配器   |
| （2^n）      |——————>|及分配器管理   |———————>........
_______________      |的内存空间     |
                     ———————————————— 
——————————————       _______________
|slab控制块2   |       | slab分配器   |
| （2^n+1）    |——————>|及分配器管理   |———————>........
_______________      |的内存空间     |
                     ———————————————— 
 * <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
————————————————————————————————————————————————————
为slab分配器分配一个zone时，以slab控制块参数为准
***************************************************/
#endif